#ifndef array_properties
#define array_properties

#define BITS_PER_SUBIMAGE 512

typedef struct {
	char image_data[64][8]; //the current state of each bit in a pixel is stored as a single char
							//and the resultant extra data extends the width of each image chunk.
							//though possible to extract bit-information on the fly, 8x the data storage on this level
							//is preferable to the resulting complexity and overhead of masking pixels constantly.
} image_chunk;

typedef struct {
	short network_weights[BITS_PER_SUBIMAGE][BITS_PER_SUBIMAGE]; /*store every bit's relationships in a single line
									 64*8 = 512 bits per block.
									 a short allows for storing a constant correlation across ~32,000 images.*/
} network_chunk;


#endif