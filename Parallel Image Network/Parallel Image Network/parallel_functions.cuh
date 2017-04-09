#ifndef parallel_functions_cuh
#define parallel_functions_cuh

/* Host code to set up data to run on the GPU, then train the network in parallel*/
void
parallel_trainer(image_chunk ***image_grid, network_chunk *** network_grid);

/* Host code to set up data to run on the GPU, then recall the image in parallel.*/
void
parallel_recall(image_chunk ***image_grid, network_chunk *** network_grid, int loops);

#endif