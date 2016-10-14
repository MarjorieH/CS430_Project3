#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Hard coded Program Constants
#define maxColor 255
#define format '3' // format of output image data
#define maxObjects 128

// Structure to hold RGB pixel data
typedef struct RGBpixel {
  unsigned char R, G, B;
} RGBpixel;

// Structure to hold an object's data in the scene
typedef struct {
  int kind; // 0 = plane, 1 = sphere, 2 = light, 3 = camera
  double color[3];
  double position[3];
  double diffuseColor[3];
  double specularColor[3];
  union {
    struct {
      double normal[3];
    } plane;
    struct {
      double radius;
    } sphere;
    struct {
      double direction[3];
      double radialA2;
      double radialA1;
      double radialA0;
      double angularA0;
    } light;
    struct {
      double width;
      double height;
    } camera;
  };
} Object;

// Global variables to hold image data
RGBpixel *pixmap; // array of pixels to hold the image data
int numPixels; // total number of pixels in image (N * M)
int M; // height of image in pixels
int N; // width of image in pixels

// Global variables to hold general scene data
Object* physicalObjects[maxObjects]; // Global array to keep track of objects in the scene
int numPhysicalObjects; // index to keep track of number of objects in the scene
Object* lightObjects[maxObjects];
int numLightObjects;
Object* cameraObject;

// Miscellaneous Globals
int line = 1; // keep track of the line number inside of the json file

// function prototype declarations
void expect_c(FILE* json, int d);
int next_c(FILE* json);
double next_number(FILE* json);
char* next_string(FILE* json);
double* next_vector(FILE* json);
double plane_intersection(double* Ro, double* Rd, double* P, double* N);
void raycast(char* filename);
void read_scene(char* filename);
void skip_ws(FILE* json);
double sphere_intersection(double* Ro, double* Rd, double* C, double r);
void writeP3(FILE* fh);
void printObjs ();
void printPixMap ();

// static inline functions
static inline double sqr(double v) {
  return v*v;
}
static inline void normalize(double* v) {
  double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
  v[0] /= len;
  v[1] /= len;
  v[2] /= len;
}
