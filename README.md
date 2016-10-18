# CS430_Project3
### CS430: Computer Graphics
### Project 3: Illumination
### Marjorie Hahn
### 18 October 2016

This application takes a JSON file containing object data for spheres and
planes within a scene and uses raycasting to create a P3 PPM image that
corresponds to that object data. It also takes data for spot lights and point
lights contained within the scene and illuminates the objects in the scene
accordingly. An example of an appropriate JSON file that this program can work
on can be found in input.json.

Usage: raycast width height input.json output.ppm

Where "width" and "height" set the size in pixels of the output.ppm image.

In order to run the program, after you have downloaded the files off of Github,
make sure that you are sitting in the directory that holds all of the files and
run the command "make all". Then you will be able to run the program using the
usage command mentioned above.

There is one JSON test file included that you can use to test the functionality
of the program: input.json. The expected output image for this JSON file is
output.ppm. You can create the corresponding PPM image for this JSON file using
the command "make all".
