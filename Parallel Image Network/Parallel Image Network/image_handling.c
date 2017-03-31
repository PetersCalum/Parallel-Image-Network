#include <stdio.h>
#include <stdlib.h>
//#include <windows.h>
#include "lodepng.h"
#include "image_properties.h"


/*Uses lodepng to read in a given image and consolidate all the colour channels in contiguous blocks.*/
unsigned char
*load_image(char* file_location, int *array_length)
{
	unsigned error_digit, width, height;
	unsigned char *load_output;
	error_digit = lodepng_decode24_file(&load_output, &width, &height, file_location);
	if (error_digit == 0)
	{
		if (height != IMAGE_HEIGHT || width != IMAGE_WIDTH) 
		{
			printf("Error: Image Sizes Incorrect");
			return NULL;
		}
		else
		{
			*array_length = (NUMBER_PIXELS * 3); //record array size--one value per pixel per colour channel
			unsigned char *loaded_image = malloc(sizeof(char) * *array_length); //allocate array memory
			if (loaded_image != NULL) 
			{
				for (int i = 0; i < NUMBER_PIXELS; i++)
				{
					loaded_image[i] = load_output[i*3]; //red channel
					loaded_image[NUMBER_PIXELS + i] = load_output[1 + (i * 3)]; //green channel
					loaded_image[NUMBER_PIXELS*2 + i] = load_output[2 + (i * 3)]; //blue channel
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
		printf("Error in loading image. Error Code: %d", error_digit);
		return NULL;
	}
}

void 
save_image(char* file_location, char* image_data)
{
	if (image_data != NULL) {
		char *png_data = malloc(sizeof(char) * NUMBER_PIXELS * 3);
		if (png_data != NULL) {
			for (int i = 0; i < NUMBER_PIXELS; i++)
			{
				png_data[i * 3] = image_data[i];
				png_data[1 + i * 3] = image_data[NUMBER_PIXELS + i];
				png_data[2 + i * 3] = image_data[NUMBER_PIXELS*2 +i];
			}
			lodepng_encode24_file(file_location, png_data, IMAGE_WIDTH, IMAGE_HEIGHT);
		}
	}
}

/*int main(void)
{

	int array_length;
	unsigned char *image = load_image("TestImage.png", &array_length);
	if (image != NULL) {
		for (int i = 0; i < array_length; i++)
		{
			printf("%d \n", *(image + i));
		}
		char* red = malloc(sizeof(char) * NUMBER_PIXELS);
		char* green = malloc(sizeof(char) * NUMBER_PIXELS);
		char* blue= malloc(sizeof(char) * NUMBER_PIXELS);
		for (int i = 0; i < NUMBER_PIXELS; i++) 
		{
			*(red + i) = *(image + i);
			*(green + i) = *(image + NUMBER_PIXELS + i);
			*(blue + i) = *(image + (NUMBER_PIXELS * 2) + i);
		}
		save_image("outputimageredgreenblue.png", red, red, red);
		free(red);
		free(green);
		free(blue);
		free(image);
	}
}*/