#include <stdio.h>
#include <stdlib.h>
#include "image_handling.h"
#include "image_properties.h"
#include "lodepng.h"

typedef struct {
	char image_data[64][8]; //the current state of each bit in a pixel is stored as a single char
	//and the resultant extra data extends the width of each image chunk.
	//though possible to extract bit-information on the fly, 8x the data storage on this level
	//is preferable to the resulting complexity and overhead.
} image_chunk;

typedef struct {
	char image_data[64][8]; //for tracking just sub-arrays
} image_chunk_data;

typedef struct {
	short network_weights[512][512]; /*store every bit's relationships in a single line
	64*8 = 512 bits per block.
	a short allows for storing a constant correlation across ~32,000 images.*/
} network_chunk;

/* loads in a given image and populates the image chunks.*/
void
array_loader(image_chunk ***image_grid, char* file_location)
{
	int array_length;
	unsigned char pix_mask[8]; //bitmask array for selecting bits from a char, MSB to LSB
	for (int i = 0; i < 8; i++)
	{
		pix_mask[i] = (1 << (7 - i));
	}
	unsigned char *image = load_image(file_location, &array_length);
	/* Move through the image in blocks of eight lines at a time, filling out
	the blocks on each row together.*/
	for (int colour = 0; colour < COLOUR_CHANNELS; colour++)
	{
		int channel_offset = colour*NUMBER_PIXELS; //how offset in the loaded array each colour channel is
		//i.e., no offset for red, a number of pixels for green, 2x number of pixels for blue.
		for (int chunk_row = 0; chunk_row < VER_ARRAYS; chunk_row++)
		{
			int chunk_row_offset = chunk_row * IMAGE_WIDTH * 8; //number of pixels read in per row of the chunk grid
			for (int image_row = 0; image_row < 8; image_row++) //which row on the actual image is it? In relationship 
				//to the chunk
			{
				int image_row_offset = image_row * IMAGE_WIDTH; 
				for (int pixel_num = 0; pixel_num < IMAGE_WIDTH; pixel_num++) //which pixel on the row is it?
				{
					unsigned char pixel = image[channel_offset + chunk_row_offset + image_row_offset + pixel_num];
					int chunk_x = pixel_num / 8; //for every 8 pixels, move onto the next chunk.
					int pixel_in_block = pixel_num % 8;
					for (int bit = 0; bit < 8; bit++) {
						int bit_offset = pixel_in_block * 8 + bit;
						if ((pixel & pix_mask[bit]) != 0)
						{
							image_grid[chunk_x][chunk_row][colour].image_data[bit_offset][image_row] = 1;
						}
						else
						{
							image_grid[chunk_x][chunk_row][colour].image_data[bit_offset][image_row] = 0;
						}
					}
				}
			}
		}
	}
	return;
}

void
array_saver(image_chunk ***image_grid, char* file_location)
{
	unsigned char* image_data = malloc(sizeof(char)*NUMBER_PIXELS * COLOUR_CHANNELS);

	for (int colour = 0; colour < COLOUR_CHANNELS; colour++)
	{
		int channel_offset = colour*NUMBER_PIXELS; //how much to offset the pixels by to arrange colour channels correctly;
		for (int chunk_x = 0; chunk_x < HOR_ARRAYS; chunk_x++) 
		{
			int chunk_offset = chunk_x * 8; //additional shifting in the array to keep in line with the chunks
			for (int chunk_y = 0; chunk_y < VER_ARRAYS; chunk_y++)
			{
				int chunk_row_offset = chunk_y * IMAGE_WIDTH * 8;
				for (int image_row = 0; image_row < 8; image_row++)
				{
					int image_row_offset = image_row * IMAGE_WIDTH;
					for (int pixel = 0; pixel < 8; pixel++)
					{
						unsigned char pixel_value = 0;
						for (int bit = 0; bit < 8; bit++)
						{
							pixel_value = pixel_value << 1; //shift bits 1 closer to original position.
							int pixel_offset = pixel * 8 + bit;
							if (image_grid[chunk_x][chunk_y][colour].image_data[pixel_offset][image_row] == 1)
							{
								pixel_value = pixel_value++;
							}
						}
						image_data[channel_offset + chunk_row_offset + image_row_offset + chunk_offset + pixel ] = pixel_value;
					}
				}
			}
		}
	}
	save_image(file_location, image_data);
	free(image_data);
	return;
}

void
network_saver(char* file_location, network_chunk*** network_grid)
{
	FILE *network_file;

	network_file = fopen(file_location, "w");
	for (int i = 0; i < HOR_ARRAYS; i++)
	{
		for (int j = 0; j < VER_ARRAYS; j++)
		{
			for (int k = 0; k < COLOUR_CHANNELS; k++)
			{
				fwrite(network_grid[i][j][k].network_weights, sizeof(short), (512 * 512), network_file);
			}
		}
	}

	fclose(network_file);

	return;
}

void
train_network(image_chunk*** image_grid, network_chunk*** network_grid)
{
	for (int colour = 0; colour < COLOUR_CHANNELS; colour++)
	{
		for (int chunk_x = 0; chunk_x < HOR_ARRAYS; chunk_x++)
		{
			for (int chunk_y = 0; chunk_y < VER_ARRAYS; chunk_y++)
			{
				for (int bit_relating_to = 0; bit_relating_to < 512; bit_relating_to++)
				{
					char bit = image_grid[chunk_x][chunk_y][colour].image_data[bit_relating_to % 64][bit_relating_to / 64];
					for (int bit_relating_from = 0; bit_relating_from < 512; bit_relating_from++)
					{
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
	for (int i = 0; i < 512; i++)
	{
		int weighted_value = (image_grid[chunk_x][chunk_y][colour].image_data[i % 64][i / 64]) * 2 - 1; //converts an off pixel into a negative value 
		//but leaves an on pixel alone
		sum_correlations += network_grid[chunk_x][chunk_y][colour].network_weights[bit_num][i] * weighted_value;
	}
	if (sum_correlations > 0)
		return_value = 1;

	return return_value;

}

void
recall_chunk(image_chunk*** image_grid, network_chunk*** network_grid, int chunk_x, int chunk_y, int colour)
{
	int sequence[512];
	for (int i = 0; i < 512; i++)
	{
		sequence[i] = i;
	}
	shuffle_int(sequence, 512);

	for (int i = 0; i < 512; i++) {
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

void
generate_cue_image(image_chunk*** image_grid, int noise) 
{
	for (int colour = 0; colour < COLOUR_CHANNELS; colour++)
	{
		for (int chunk_x = 0; chunk_x < HOR_ARRAYS; chunk_x++)
		{
			for (int chunk_y = 0; chunk_y < VER_ARRAYS; chunk_y++)
			{
				for (int bit_x = 0; bit_x < 64; bit_x++)
				{
					for (int bit_y = 0; bit_y < 8; bit_y++)
					{
						if (rand() % 101 < noise)
							image_grid[chunk_x][chunk_y][colour].image_data[bit_x][bit_y] = 1 - image_grid[chunk_x][chunk_y][colour].image_data[bit_x][bit_y];
					}
				}
			}
		}
	}
}

int
main(int argc, char *argv[])
{
	srand(time(NULL));
	/*Declares a three-dimensional matrix large enough to hold every 8x8 pixel chunk
	in the image, with the third dimension representing the different colour
	channels. Allocated with malloc to make sure it ends up on the heap.*/
	image_chunk ***image_grid = (image_chunk***) malloc(HOR_ARRAYS * sizeof(image_chunk **));
	for (int i = 0; i < HOR_ARRAYS; i++)
	{
		image_grid[i] = (image_chunk**) malloc(VER_ARRAYS * sizeof(image_chunk*));
		for (int j = 0; j < VER_ARRAYS; j++)
		{
			image_grid[i][j] = (image_chunk*) malloc(COLOUR_CHANNELS * sizeof(image_chunk));
		}
	}	
	
	network_chunk ***network_grid = (network_chunk***)malloc(HOR_ARRAYS * sizeof(network_chunk **));
	for (int i = 0; i < HOR_ARRAYS; i++)
	{
		network_grid[i] = (network_chunk**)malloc(VER_ARRAYS * sizeof(network_chunk*));
		for (int j = 0; j < VER_ARRAYS; j++)
		{
			network_grid[i][j] = (network_chunk*)malloc(COLOUR_CHANNELS * sizeof(network_chunk));
		}
	}

	for (int z = 0; z < COLOUR_CHANNELS; z++)
	{
		for (int x = 0; x < HOR_ARRAYS; x++) 
		{
			for (int y = 0; y < VER_ARRAYS; y++) 
			{
				for (int i = 0; i < 512; i++)
				{
					for (int j = 0; j < 512; j++)
					{
						network_grid[x][y][z].network_weights[i][j] = 0;
					}
				}
			}
		}
	}

	array_loader(image_grid, "image.png");
	train_network(image_grid, network_grid);
	array_loader(image_grid, "foldericon.png");
	train_network(image_grid, network_grid);
	generate_cue_image(image_grid, 25);
	array_saver(image_grid, "image3.png");
	recall_image(image_grid, network_grid, 100);
	array_saver(image_grid, "image4.png");

	for (int i = 0; i < HOR_ARRAYS; i++)
	{
		for (int j = 0; j < VER_ARRAYS; j++)
		{
			free(image_grid[i][j]);
		}
		free(image_grid[i]);
	}
	free(image_grid);


	return 0;

}