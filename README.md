# Parallel-Image-Network

Utilises LodePNG (available at http://lodev.org/lodepng/ ) for encoding and decoding PNG files.

This program is a relatively simple utility for demonstrating associative memory in a serial and parallel implementation. It has 
the ability to read in any 64x64 .png file (though is easily altered to accommodate any image size, so long as its dimensions are a multiple 
of 8) and train an array of simple neural networks to recall the image.

Though it will by default load an input image called input.png from the same directory and run the serial implementation on it,
saving the file as output.png, the following command line arguments can be used to alter program behaviour (all lowercase):

i [filepath] -- change the input file ((i)nput)
o [filename] -- change the output destination ((o)utput)
n [filename] -- change the file location for loading and saving the neural networks ((n)etwork location)
s -- set the program to save the trained network ((s)ave network)
l -- set the program to load a trained network before training it on the current image ((l)oad network)
g -- run the parallel implementation ((g)pu)
t -- only train the network, don't distort and recall the image ((t)rain)
k -- skip training the network, meaningless unless used in conjunction with loading (s(k)ip)
d [number] -- the % distortion to be applied to the network ((d)istortion)
r [number] -- the number of iterations to run when recalling the image ((r)epetitions)
