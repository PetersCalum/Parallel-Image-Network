#ifndef shared_functions_h
#define shared_functions_h

/* Loads in the image at the supplied file locaiton and breaks it down into
8x8 chunks stored in the supplied image_chunk array*/
int array_loader(image_chunk *** image_grid, char * file_location);

/* Takes the given array of image chunks and restores it to a single complete image,
before saving the image in the supplied location. */
void array_saver(image_chunk *** image_grid, char * file_location);

/* Saves the component sub-networks in the given location*/
void network_saver(network_chunk *** network_grid, char * file_location);

/* Loads the network in the given location, overwriting any data already
in the sub-networks in argument 1.*/
int network_loader(network_chunk *** network_grid, char * file_location);

/* Distorts all of the subimages by % amount. */
void generate_cue_image(image_chunk *** image_grid, int noise);

#endif