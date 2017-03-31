#include <stdio.h>
#include <stdlib.h>
#include "image_handling.h"
#include "image_properties.h"

typedef struct {
	char image_data[64][8]; //the current state of each bit in a pixel is stored as a single char
	//and the resultant extra data extends the width of each image chunk.
	//though possible to extract bit-information on the fly, 8x the data storage on this level
	//is preferable to the resulting complexity and overhead.
	short network_weights[512][512]; //store every bit's relationships in a single line
	//64*8 = 512 bits per block.
	//a short allows for storing a constan correlation across ~32,000 images.
} image_chunk;

/* loads in a given image and populates the image chunks, including initialising 
the relationship arrays.*/
void
array_loader(image_chunk ***image_grid, char* file_location)
{
	int array_length;
	unsigned char pix_mask[] = { 128, 64, 32, 16, 8, 4, 2, 1 }; //bitmask arrays for selecting bits from a char, MSB to LSB
	unsigned char *image = load_image(file_location, &array_length);
	/* Move through the image in blocks of eight lines at a time, filling out
	the blocks on each row together.*/
	for (int colour = 0; colour < 3; colour++)
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
						if (pixel & pix_mask[bit] != 0)
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

	/*Now initialising the relationships to a blank array*/
	for (int colour = 0; colour < 3; colour++)
	{
		for (int chunk_x = 0; chunk_x < HOR_ARRAYS; chunk_x++)
		{
			for (int chunk_y = 0; chunk_y < VER_ARRAYS; chunk_y++)
			{
				for (int weights_x = 0; weights_x < 512; weights_x++)
				{
					for (int weights_y = 0; weights_y < 512; weights_y++)
					{
						image_grid[chunk_x][chunk_y][colour].network_weights[weights_x][weights_y] = 0;
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
	unsigned char* image_data = malloc(sizeof(char)*NUMBER_PIXELS * 3);

	for (int colour = 0; colour < 3; colour++)
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
network_saver(char* file_location, image_chunk*** image_grid)
{
	FILE *network_file;

	network_file = fopen(file_location, "w");
	for (int i = 0; i < HOR_ARRAYS; i++)
	{
		for (int j = 0; j < VER_ARRAYS; j++)
		{
			for (int k = 0; k < 3; k++)
			{
				fwrite(image_grid[i][j][k].network_weights, sizeof(short), (512 * 512), network_file);
			}
		}
	}

	fclose(network_file);

	return;
}

int
main(int argc, char *argv[])
{
	/*Declares a three-dimensional matrix large enough to hold every 8x8 pixel chunk
	in the image, with the third dimension representing the different colour
	channels. Allocated with malloc to make sure it ends up on the heap.*/
	image_chunk ***image_grid = (image_chunk***) malloc(HOR_ARRAYS * sizeof(image_chunk **));
	for (int i = 0; i < HOR_ARRAYS; i++)
	{
		image_grid[i] = (image_chunk**) malloc(VER_ARRAYS * sizeof(image_chunk*));
		for (int j = 0; j < VER_ARRAYS; j++)
		{
			image_grid[i][j] = (image_chunk*) malloc(3 * sizeof(image_chunk));
		}
	}

	array_loader(image_grid, "TestImage.png");
	network_saver("relationships.bin", image_grid);

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