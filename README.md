# TextureProject
Recoloring of a texture file based on a map file that splits the image into regions.

Example run of the provided main function:

```cmd
textureProject.exe <texture_path> <map_path> <output_path> <num_regions>
```
The texture file should be a 24-bit-per-pixel RGB image and the map file should be a grayscale image. Currently, compression
of the output file is not supported.

The Recolor class performs the main recoloring tasks. The colors provided to an instance of that class can be set by the user 
or be randomly generated. Almost all of the tasks involved in the recoloring utilize GPU parallelization by OpenCL. Therefore, 
OpenCL must be installed and VS must be correctly configured in order to build the solution. 

---

Planned features for upcoming versions:
* A GUI for the application that supports bulk recoloring of multiple textures
* Option to preview the texture on its corresponding model.
* Generating the map file by some specific algortihm rather than having the user make it themselves by hand.
* PNG compression for writing the output file.

---

Format of the map file:
* Region of a pixel is indexed through the value of the grayscale pixel, where the region index can be obtained by dividing the value by 10.
* A value of 0 indicates that the region will not be recolored.
* Each region has a sample point inside, signified by value 255. The color of the corresponding pixel in the texture will be mapped
to the user-provided color for that region, and the pixels in the same region will be recolored based on this mapping. These sample
points should not be put on the edges of the file or of the region they belong to.
* Removing the sample point in the map file, changing the values of a region to zero, or providing an "empty" color for that region
to the Recolor class (see the implementation of ColorRGB for the meaning of "empty") all mean that the region will not be recolored.
