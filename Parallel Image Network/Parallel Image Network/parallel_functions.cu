#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h> 
#include <device_launch_parameters.h>
#include "image_properties.h"
#include "array_properties.h"
#include "shared_functions.h"
#include "serial_functions.h"

__global__ void
parallel_train_network(image_chunk * image_grid, network_chunk * network_grid)
{
	char bit_value = image_grid[blockIdx.x + (blockIdx.y*VER_ARRAYS) +
		(blockIdx.z*VER_ARRAYS*HOR_ARRAYS)].image_data[threadIdx.x%64][threadIdx.x/64];
	/*In this case, where there are necessarily more variables than can be included in a single launch
	due to needing to specify more dimensions than can be arranged in blocks (limited to three) 
	and adding thread dimensions would exceed the hardware limit (512^2 = 262,144, 
	even the best GPU can't exceed 2048) it's a case of choosing between multiple kernel launches, 
	or including a loop in the device code. In this case, I chose to have just one loop here.*/
	for (int i = 0; i < blockDim.x; i++)
	{
		char relating_bit_value = image_grid[blockIdx.x + (blockIdx.y*VER_ARRAYS) + 
			(blockIdx.z*VER_ARRAYS*HOR_ARRAYS)].image_data[i % 64][i / 64];
		if (relating_bit_value == bit_value)
		{
			network_grid[blockIdx.x + (blockIdx.y*VER_ARRAYS) +
				(blockIdx.z*VER_ARRAYS*HOR_ARRAYS)].network_weights[threadIdx.x][i] += 1;
		}
		else
		{
			network_grid[blockIdx.x + (blockIdx.y*VER_ARRAYS) +
				(blockIdx.z*VER_ARRAYS*HOR_ARRAYS)].network_weights[threadIdx.x][i] += -1;
		}
	}
}

__global__ void
parallel_recall_image(image_chunk * image_grid, network_chunk * network_grid, int colour_channel) {
	//accessible to all threads within a block.
	__shared__ int neuron_output[BITS_PER_SUBIMAGE];
	/*Unlike the above code, here there are separate kernels for each colour channel--otherwise there
	would be nested for loops in the device code. Because of the need to access a shared array, this
	has the potential for leaving lots of threads waiting most of the time, which should be avoided
	as it's poor performance.*/
	char weighted_value = image_grid[blockIdx.x + (blockIdx.y*VER_ARRAYS) +
		(colour_channel*VER_ARRAYS*HOR_ARRAYS)].image_data[threadIdx.x % 64][threadIdx.x / 64] * 2 - 1; 
	neuron_output[threadIdx.x] = network_grid[blockIdx.x + blockIdx.y*VER_ARRAYS +
		colour_channel*VER_ARRAYS*HOR_ARRAYS].network_weights[blockIdx.z][threadIdx.x] * weighted_value;

	__syncthreads();

	for (int i = blockDim.x / 2; i > 0; i >>= 1) { //first half of the array adds the second
		//then first half of THAT adds the second, until everything has been added to the 0th element
		if (threadIdx.x < i) {
			neuron_output[threadIdx.x] += neuron_output[threadIdx.x + i];
		}
		__syncthreads();
	}

	if (threadIdx.x == 0) //only operate this on one thread
		//sets bit values
	{
		if (neuron_output[0] > 0) { 
			image_grid[blockIdx.x + (blockIdx.y*VER_ARRAYS) + (colour_channel*VER_ARRAYS*HOR_ARRAYS)]
				.image_data[blockIdx.z % 64][blockIdx.z / 64] = 1;
		}
		else {
			image_grid[blockIdx.x + (blockIdx.y*VER_ARRAYS) + (colour_channel*VER_ARRAYS*HOR_ARRAYS)]
				.image_data[blockIdx.z % 64][blockIdx.z / 64] = 0;
		}
	}
}

__host__
void
parallel_trainer(image_chunk ***image_grid, network_chunk ***network_grid) {

	//convert the multidimensional array  to a single array--can be transferred in one go to the GPU
	//rather than specifying the dimensions in the method header.
	//as we can see here, using the Cuda version 6.0+ unified memory allows for transferring structs
	//rather than having to allocate memory for each sub-array individually and update the device pointers
	image_chunk * parallel_image;
	cudaMallocManaged(&parallel_image, sizeof(image_chunk)*VER_ARRAYS*HOR_ARRAYS*COLOUR_CHANNELS);
	network_chunk * parallel_network;
	cudaMallocManaged(&parallel_network, sizeof(network_chunk)*VER_ARRAYS*HOR_ARRAYS*COLOUR_CHANNELS);
	for (int chunk_x = 0; chunk_x < VER_ARRAYS; chunk_x++)
	{
		for (int chunk_y = 0; chunk_y < HOR_ARRAYS; chunk_y++)
		{
			for (int colour = 0; colour < COLOUR_CHANNELS; colour++)
			{
				for (int image_y = 0; image_y < 8; image_y++)
				{
					for (int image_x = 0; image_x < 64; image_x++)
					{
						parallel_image[chunk_x + (chunk_y * VER_ARRAYS) + (colour * VER_ARRAYS * HOR_ARRAYS)]
							.image_data[image_x][image_y]
							= image_grid[chunk_x][chunk_y][colour].image_data[image_x][image_y];
					}
				}
				for (int relationship_x = 0; relationship_x < BITS_PER_SUBIMAGE; relationship_x++)
				{
					for (int relationship_y = 0; relationship_y < BITS_PER_SUBIMAGE; relationship_y++)
					{
						parallel_network[chunk_x + (chunk_y * VER_ARRAYS) + (colour * VER_ARRAYS * HOR_ARRAYS)]
							.network_weights[relationship_x][relationship_y]
							= network_grid[chunk_x][chunk_y][colour].network_weights[relationship_x][relationship_y];
					}
				}
			}
		}
	}	

	dim3 grid_dimensions(VER_ARRAYS, HOR_ARRAYS, COLOUR_CHANNELS); //dimensions of the block grid
	//e.g., how many blocks to have
	dim3 block_dimensions(BITS_PER_SUBIMAGE); //number of threads per block. With one value, the Y and Z dimensions
	//are initialised to 1.

	parallel_train_network<<<grid_dimensions, block_dimensions>>>(parallel_image, parallel_network);
	cudaDeviceSynchronize();

	//copy back over to the original arrays
	for (int chunk_x = 0; chunk_x < VER_ARRAYS; chunk_x++)
	{
		for (int chunk_y = 0; chunk_y < HOR_ARRAYS; chunk_y++)
		{
			for (int colour = 0; colour < COLOUR_CHANNELS; colour++)
			{
				for (int image_y = 0; image_y < 8; image_y++)
				{
					for (int image_x = 0; image_x < 64; image_x++)
					{
						image_grid[chunk_x][chunk_y][colour].image_data[image_x][image_y] =
							parallel_image[chunk_x + (chunk_y * VER_ARRAYS) + (colour * VER_ARRAYS * HOR_ARRAYS)]
							.image_data[image_x][image_y];
					}
				}
				for (int relationship_x = 0; relationship_x < BITS_PER_SUBIMAGE; relationship_x++)
				{
					for (int relationship_y = 0; relationship_y < BITS_PER_SUBIMAGE; relationship_y++)
					{
						network_grid[chunk_x][chunk_y][colour].network_weights[relationship_x][relationship_y] =
						parallel_network[chunk_x + (chunk_y * VER_ARRAYS) + (colour * VER_ARRAYS * HOR_ARRAYS)]
							.network_weights[relationship_x][relationship_y];
					}
				}
			}
		}
	}

	cudaFree(parallel_image);
	cudaFree(parallel_network);

}

__host__
void
parallel_recall(image_chunk ***image_grid, network_chunk ***network_grid, int loops) {
	//as above, copy the data into linear arrays
	image_chunk * parallel_image;
	cudaMallocManaged(&parallel_image, sizeof(image_chunk)*VER_ARRAYS*HOR_ARRAYS*COLOUR_CHANNELS);
	network_chunk * parallel_network;
	cudaMallocManaged(&parallel_network, sizeof(network_chunk)*VER_ARRAYS*HOR_ARRAYS*COLOUR_CHANNELS);
	for (int chunk_x = 0; chunk_x < VER_ARRAYS; chunk_x++)
	{
		for (int chunk_y = 0; chunk_y < HOR_ARRAYS; chunk_y++)
		{
			for (int colour = 0; colour < COLOUR_CHANNELS; colour++)
			{
				for (int image_y = 0; image_y < 8; image_y++)
				{
					for (int image_x = 0; image_x < 64; image_x++)
					{
						parallel_image[chunk_x + (chunk_y * VER_ARRAYS) + (colour * VER_ARRAYS * HOR_ARRAYS)]
							.image_data[image_x][image_y]
							= image_grid[chunk_x][chunk_y][colour].image_data[image_x][image_y];
					}
				}
				for (int relationship_x = 0; relationship_x < BITS_PER_SUBIMAGE; relationship_x++)
				{
					for (int relationship_y = 0; relationship_y < BITS_PER_SUBIMAGE; relationship_y++)
					{
						parallel_network[chunk_x + (chunk_y * VER_ARRAYS) + (colour * VER_ARRAYS * HOR_ARRAYS)]
							.network_weights[relationship_x][relationship_y]
							= network_grid[chunk_x][chunk_y][colour].network_weights[relationship_x][relationship_y];
					}
				}
			}
		}
	}

	dim3 grid_dimensions(VER_ARRAYS, HOR_ARRAYS, BITS_PER_SUBIMAGE);
	dim3 block_dimensions(BITS_PER_SUBIMAGE); 
	for (int i = 0; i < loops; i++)
	{
		for (int colour = 0; colour < COLOUR_CHANNELS; colour++)
		{
			parallel_recall_image<<<grid_dimensions, block_dimensions>>>(parallel_image, parallel_network, colour);
			cudaDeviceSynchronize(); //makes sure that the program doesn't proceed until the device code is finished executing
			//all data is already on the device, so it's just a matter of launching another kernel
		}
	}
	//copy back over to the original arrays
	for (int chunk_x = 0; chunk_x < VER_ARRAYS; chunk_x++)
	{
		for (int chunk_y = 0; chunk_y < HOR_ARRAYS; chunk_y++)
		{
			for (int colour = 0; colour < COLOUR_CHANNELS; colour++)
			{
				for (int image_y = 0; image_y < 8; image_y++)
				{
					for (int image_x = 0; image_x < 64; image_x++)
					{
						image_grid[chunk_x][chunk_y][colour].image_data[image_x][image_y] =
							parallel_image[chunk_x + (chunk_y * VER_ARRAYS) + (colour * VER_ARRAYS * HOR_ARRAYS)]
							.image_data[image_x][image_y];
					}
				}
				for (int relationship_x = 0; relationship_x < BITS_PER_SUBIMAGE; relationship_x++)
				{
					for (int relationship_y = 0; relationship_y < BITS_PER_SUBIMAGE; relationship_y++)
					{
						network_grid[chunk_x][chunk_y][colour].network_weights[relationship_x][relationship_y] =
							parallel_network[chunk_x + (chunk_y * VER_ARRAYS) + (colour * VER_ARRAYS * HOR_ARRAYS)]
							.network_weights[relationship_x][relationship_y];
					}
				}
			}
		}
	}

	cudaFree(parallel_image);
	cudaFree(parallel_network);

}