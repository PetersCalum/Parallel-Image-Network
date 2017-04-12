#include <stdio.h>
#include <stdlib.h>
#include "image_properties.h"
#include "array_properties.h"

void
train_network(image_chunk*** image_grid, network_chunk*** network_grid)
{
	for (int colour = 0; colour < COLOUR_CHANNELS; colour++)
	{
		for (int chunk_x = 0; chunk_x < HOR_ARRAYS; chunk_x++)
		{
			for (int chunk_y = 0; chunk_y < VER_ARRAYS; chunk_y++)
			{
				for (int bit_relating_to = 0; bit_relating_to < BITS_PER_SUBIMAGE; bit_relating_to++)
				{
					char bit = image_grid[chunk_x][chunk_y][colour].image_data[bit_relating_to % 64][bit_relating_to / 64];
					for (int bit_relating_from = 0; bit_relating_from < BITS_PER_SUBIMAGE; bit_relating_from++)
					{
						//if the two bits match, then increase the weight of their connection--i.e., the more often they match
						//the stronger the positive correlation. 
						char relating_bit = image_grid[chunk_x][chunk_y][colour].image_data[bit_relating_from % 64][bit_relating_from / 64];
						if (bit == relating_bit)
							network_grid[chunk_x][chunk_y][colour].network_weights[bit_relating_to][bit_relating_from] += 1;
						else
							network_grid[chunk_x][chunk_y][colour].network_weights[bit_relating_to][bit_relating_from] += -1;
					}
				}
			}
		}
	}
}

void
shuffle_int(int* array, int array_length)
{
	if (array_length > 1)
	{
		size_t i;
		for (i = 0; i < array_length - 1; i++)
		{
			size_t j = i + rand() / (RAND_MAX / (array_length - i) + 1);
			int t = array[j];
			array[j] = array[i];
			array[i] = t;
		}
	}
}

char
neuron_output(image_chunk*** image_grid, network_chunk*** network_grid, int bit_num, int chunk_x, int chunk_y, int colour)
{
	int sum_correlations = 0;
	char return_value = 0;
	for (int i = 0; i < BITS_PER_SUBIMAGE; i++)
	{
		char weighted_value = (image_grid[chunk_x][chunk_y][colour].image_data[i % 64][i / 64]) * 2 - 1; //converts an off pixel into a negative value 
																										//but leaves an on pixel alone
		sum_correlations += network_grid[chunk_x][chunk_y][colour].network_weights[bit_num][i] * weighted_value;
		//by multiplying the weighted value by the correlation, get the output for each individual neuron.
		//when these are summed, then (based on the current state of the image) a positive or negative value for
		//each bit will be predicted. With a sufficiently recognisable image, then eventually this will return
		//all bits to the correct value.
	}
	if (sum_correlations > 0)
		return_value = 1;

	return return_value;

}

void
recall_chunk(image_chunk*** image_grid, network_chunk*** network_grid, int chunk_x, int chunk_y, int colour)
{
	int sequence[BITS_PER_SUBIMAGE];
	for (int i = 0; i < BITS_PER_SUBIMAGE; i++)
	{
		sequence[i] = i;
	}
	//randomise the order in which bits are updated.
	shuffle_int(sequence, BITS_PER_SUBIMAGE);

	//get the output for the neurons in the order supplied.
	for (int i = 0; i < BITS_PER_SUBIMAGE; i++) {
		int bitnum = sequence[i];
		int bit_x = sequence[i] % 64;
		int bit_y = sequence[i] / 64;
		char bit_value = neuron_output(image_grid, network_grid, bitnum, chunk_x, chunk_y, colour);
		if (bit_value != image_grid[chunk_x][chunk_y][colour].image_data[bit_x][bit_y])
			image_grid[chunk_x][chunk_y][colour].image_data[bit_x][bit_y] = bit_value;
	}


}

void
recall_image(image_chunk*** image_grid, network_chunk*** network_grid, int number_loops)
{
	for (int i = 0; i < number_loops; i++)
	{
		for (int colour = 0; colour < COLOUR_CHANNELS; colour++)
		{
			for (int chunk_x = 0; chunk_x < HOR_ARRAYS; chunk_x++)
			{
				for (int chunk_y = 0; chunk_y < VER_ARRAYS; chunk_y++)
				{
					recall_chunk(image_grid, network_grid, chunk_x, chunk_y, colour);
				}
			}
		}
	}
}