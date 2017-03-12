#include <stdio.h>
#include <stdlib.h>
#include "lodepng.h"
#include "image_properties.h"

/*Uses lodepng to read in a given image and strip it down to a numerical RGB array. */
int
load_image(char* file_location, size_t array_size)
{
	char* load_output;
	unsigned error_digit;
	error_digit = lodepng_decode24_file(load_output, IMAGE_WIDTH, IMAGE_HEIGHT, file_location);
	if (error_digit == 0)
	{
		return 0;
	}
	else
	{
		printf("Error in loading image. Error Code: %d", error_digit);
		return 0;
	}
}

void 
save_image(char* file_location, int* image_data, size_t array_size)
{

}