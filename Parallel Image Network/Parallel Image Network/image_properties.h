/*To avoid unexpected behaviour, image width and height should be kept as multiples
of 8.*/
#define IMAGE_WIDTH 64
#define IMAGE_HEIGHT 64
#define NUMBER_PIXELS IMAGE_WIDTH * IMAGE_HEIGHT

/*Multiplied together, these give you the total number of sub-arrays the image
is divided into.*/
#define HOR_ARRAYS IMAGE_WIDTH/8
#define VER_ARRAYS IMAGE_HEIGHT/8
#define COLOUR_CHANNELS 4