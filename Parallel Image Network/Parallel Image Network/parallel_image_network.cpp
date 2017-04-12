#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cuda_runtime.h> 
#include <device_launch_parameters.h>
#include "image_properties.h"
#include "array_properties.h"
#include "serial_functions.h"
#include "shared_functions.h"
#include "parallel_functions.cuh"

int
main(int argc, char *argv[])
{
	/*default values without command line arguments*/
	char * image_location = "image.png";
	char * image_output = "output.png";
	char * network_location = "network.bin";
	int network_save = 0;
	int network_load = 0;
	int train_only = 0;
	int skip_training = 0;
	int parallel_run = 0;
	int distort = 20;
	int repeats = 100;

	srand(time(NULL));

	/*Declares a three-dimensional matrix large enough to hold every 8x8 pixel chunk
	in the image, with the third dimension representing the different colour
	channels. Allocated with malloc to make sure it ends up on the heap.
	Finally, set all array values to 0.*/
	image_chunk ***image_grid = (image_chunk***)malloc(HOR_ARRAYS * sizeof(image_chunk **));
	for (int i = 0; i < HOR_ARRAYS; i++)
	{
		image_grid[i] = (image_chunk**)malloc(VER_ARRAYS * sizeof(image_chunk*));
		for (int j = 0; j < VER_ARRAYS; j++)
		{
			image_grid[i][j] = (image_chunk*)malloc(COLOUR_CHANNELS * sizeof(image_chunk));
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
				for (int i = 0; i < BITS_PER_SUBIMAGE; i++)
				{
					for (int j = 0; j < BITS_PER_SUBIMAGE; j++)
					{
						network_grid[x][y][z].network_weights[i][j] = 0;
					}
				}
			}
		}
	}

	/*Parse command line arguments*/
	int current_arg = 1; //argv 0 is the program name
	while (current_arg < argc)
	{
		switch (argv[current_arg][0])
		{
		case 'i' :	 //(I)nput
			image_location = argv[current_arg + 1];
			current_arg += 2; /*read in location then increment past it*/
			break;
		case 'o' : //(O)utput
			image_output = argv[current_arg + 1];
			current_arg += 2; /*read in location then increment past it*/
			break;
		case 'n' : //(N)etwork Location
			network_location = argv[current_arg + 1];
			current_arg += 2;
			break;
		case 's' : //Network (S)ave
			network_save = 1;
			current_arg += 1;
			break;
		case 'l' : //Network (L)oad
			network_load = 1;
			current_arg += 1;
			break;
		case 'g' : //(G)pu
			parallel_run = 1;
			current_arg += 1;
			break;
		case 't' : //(T)rain
			train_only = 1;
			current_arg += 1;
			break;
		case 'k' : //s(K)ip Training
			skip_training = 1;
			current_arg += 1;
			break;
		case 'd': //(D)istortion
			distort = strtol(argv[current_arg + 1], NULL, 10);
			current_arg += 2;
			break;
		case 'r' : //(R)epeats
			repeats = strtol(argv[current_arg + 1], NULL, 10);
			current_arg += 2;
			break;
		default :
			printf("Argument %d invalid\n", current_arg);
			current_arg += 1;
		}
	}

	/*Run setup procedures*/
	int err = array_loader(image_grid, image_location);
	if (err != 0) 
		return err;

	if (network_load == 1)
	{
		err = network_loader(network_grid, network_location);
		if (err != 0)
		{
			return err;
		}
	}

	/*Train the network with the supplied image*/
	if (skip_training == 0)
	{
		if (parallel_run == 1)
			parallel_trainer(image_grid, network_grid);
		else
			train_network(image_grid, network_grid);
	}

	/*If doing so, distort and recall the image*/
	if (train_only == 0)
	{
		generate_cue_image(image_grid, distort);
		array_saver(image_grid, "DistortedImage.png");
		if (parallel_run == 1)
		{
			parallel_recall(image_grid, network_grid, repeats);
		}
		else
		{
			recall_image(image_grid, network_grid, repeats);
		}
		array_saver(image_grid, image_output);
	}

	/*Save the network if desired*/
	if (network_save == 1) 
	{
		network_saver(network_grid, network_location);
	}

	for (int i = 0; i < HOR_ARRAYS; i++)
	{
		for (int j = 0; j < VER_ARRAYS; j++)
		{
			free(image_grid[i][j]);
			free(network_grid[i][j]);
		}
		free(image_grid[i]);
		free(network_grid[i]);
	}
	free(image_grid);
	free(network_grid);

	return 0;

}