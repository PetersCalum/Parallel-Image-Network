#include <stdio.h>
#include <stdlib.h>
#include "image_handling.h"
#include "image_properties.h"
#include "array_properties.h"
#include "serial_functions.h"
#include "shared_functions.h"

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
	recall_image(image_grid, network_grid, 10);
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