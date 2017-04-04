#ifndef image_handling_h
#define image_handling_h

/*
Takes in a PNG file location and an integer to hold the eventual length of the array as input
and returns an array of colour values, organised in three blocks (one per colour channel).
Any alpha channel will be discarded in the process.
*/

unsigned char
*load_image(char* file_location, int *array_length);

/*
Takes in a file location to save to and three separate arrays for each
colour channel and saves them as a PNG file in the specified location.
*/
void
save_image(char* file_location, char* image_data);

#endif