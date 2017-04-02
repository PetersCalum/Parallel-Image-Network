#include <stdio.h>
#include <stdlib.h>
#include "lodepng.h"
#include "image_properties.h"


/*Uses lodepng to read in a given image and consolidate all the colour channels in contiguous blocks.*/
unsigned char
*load_image(char* file_location, int *array_length)
{
	unsigned error_digit, width, height;
	unsigned char *load_output;
	error_digit = lodepng_decode32_file(&load_output, &width, &height, file_location);
	if (error_digit == 0)
	{
		if (height != IMAGE_HEIGHT || width != IMAGE_WIDTH) 
		{
			printf("Error: Image Sizes Incorrect");
			return NULL;
		}
		else
		{
			*array_length = (NUMBER_PIXELS * COLOUR_CHANNELS); //record array size--one value per pixel per colour channel
			unsigned char *loaded_image = malloc(sizeof(char) * *array_length); //allocate array memory
			if (loaded_image != NULL) 
			{
				for (int i = 0; i < NUMBER_PIXELS; i++)
				{
					loaded_image[i] = load_output[i*COLOUR_CHANNELS]; //red channel
					loaded_image[NUMBER_PIXELS + i] = load_output[1 + (i * COLOUR_CHANNELS)]; //green channel
					loaded_image[NUMBER_PIXELS * 2 + i] = load_output[2 + (i * COLOUR_CHANNELS)]; //blue channel
					loaded_image[NUMBER_PIXELS * 3 + i] = load_output[3 + (i * COLOUR_CHANNELS)]; //alpha channel
				}
				free(load_output);
				return loaded_image; //return the array
			}
			else 
			{
				return NULL;
			}
		}
	}
	else
	{
		if (error_digit == 78)
			printf("File not found, check the file name was spelled correctly.\n");
		else
			printf("Error in loading image. Error Code: %d", error_digit); 
		return NULL;
	}
}

void 
save_image(char* file_location, char* image_data)
{
	if (image_data != NULL) {
		char *png_data = malloc(sizeof(char) * NUMBER_PIXELS * COLOUR_CHANNELS);
		if (png_data != NULL) {
			for (int i = 0; i < NUMBER_PIXELS; i++)
			{
				png_data[i * COLOUR_CHANNELS] = image_data[i];
				png_data[1 + i * COLOUR_CHANNELS] = image_data[NUMBER_PIXELS + i];
				png_data[2 + i * COLOUR_CHANNELS] = image_data[NUMBER_PIXELS * 2 + i];
				png_data[3 + i * COLOUR_CHANNELS] = image_data[NUMBER_PIXELS * 3 + i];
			}
			lodepng_encode32_file(file_location, png_data, IMAGE_WIDTH, IMAGE_HEIGHT);
		}
	}
}