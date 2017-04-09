#ifndef serial_functions_h
#define serial_functions_h

/*Takes in an image decomposed into 64x8 bit chunks and trains the provided network, correlating
the state of each bit with every other bit in the chunk. Operates serially on the CPU*/
void train_network(image_chunk *** image_grid, network_chunk *** network_grid);

/*Given an image, the network will attempt to recover an image that has earlier been stored
in the supplied network array. The number of loops controls how many times the entire array will
be processed--it is possible that an equilibrium could never be achieved.*/
void recall_image(image_chunk *** image_grid, network_chunk *** network_grid, int number_loops);

#endif