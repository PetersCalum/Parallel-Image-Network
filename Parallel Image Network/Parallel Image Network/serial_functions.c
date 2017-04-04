#include <stdio.h>
#include <stdlib.h>
#include "image_handling.h"
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