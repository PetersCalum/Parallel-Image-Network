#ifndef shared_functions_h
#define shared_functions_h

void array_loader(image_chunk *** image_grid, char * file_location);

void array_saver(image_chunk *** image_grid, char * file_location);

void network_saver(char * file_location, network_chunk *** network_grid);

void generate_cue_image(image_chunk *** image_grid, int noise);

#endif