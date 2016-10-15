#include "raycast.h"

// Writes P3 formatted data to a file
// Takes in the file handler of the file to be written to
void writeP3(FILE* fh) {
  fprintf(fh, "P%c\n%i %i\n%i\n", format, N, M, maxColor); // Write out header
  for (int i = 0; i < numPixels; i++) { // Write out Pixel data
    fprintf(fh, "%i %i %i\n", pixmap[i].R, pixmap[i].G, pixmap[i].B);
  }
  fclose(fh);
}

// Calculate if the ray Ro->Rd will intersect with a sphere of center C and radius R
// Return distance to intersection
double sphere_intersection(double* Ro, double* Rd, double* C, double r) {
  double a = sqr(Rd[0]) + sqr(Rd[1]) + sqr(Rd[2]);
  double b = 2 * (Rd[0] * (Ro[0] - C[0]) + Rd[1] * (Ro[1] - C[1]) + Rd[2] * (Ro[2] - C[2]));
  double c = sqr(Ro[0] - C[0]) + sqr(Ro[1] - C[1]) + sqr(Ro[2] - C[2]) - sqr(r);

  double det = sqr(b) - 4 * a * c;
  if (det < 0) return -1; // no intersection

  det = sqrt(det);

  double t0 = (-b - det) / (2 * a);
  if (t0 > 0) return t0;

  double t1 = (-b + det) / (2 * a);
  if (t1 > 0) return t1;

  return -1;
}

// Calculate if the ray Ro->Rd will intersect with a plane of position P and normal N
// Return distance to intersection
double plane_intersection(double* Ro, double* Rd, double* P, double* N) {
  double D = -(N[0] * P[0] + N[1] * P[1] + N[2] * P[2]); // distance from origin to plane
  double t = -(N[0] * Ro[0] + N[1] * Ro[1] + N[2] * Ro[2] + D) /
  (N[0] * Rd[0] + N[1] * Rd[1] + N[2] * Rd[2]);

  if (t > 0) return t;

  return -1; // no intersection
}

// helper function to convert a percentage double into a valid value for a color channel
unsigned char double_to_color(double color) {
  if (color > 1.0) {
    color = 1.0;
  }
  else if (color < 0) {
    color = 0.0;
  }
  return (unsigned char)(maxColor * color);
}

// Cast the objects in the scene and write the output image to the file
void raycast(char* filename) {

  // default camera position
  double cx = cameraObject->position[0];
  double cy = cameraObject->position[1];
  double cz = cameraObject->position[2];

  double ch = cameraObject->camera.height;
  double cw = cameraObject->camera.width;

  double pixheight = ch / M;
  double pixwidth = cw / N;

  // initialize pixmap based on the number of pixels
  pixmap = malloc(sizeof(RGBpixel) * numPixels);
  int pixIndex = 0; // position in pixmap array

  for (int y = 0; y < M; y++) { // for each row
    double y_coord = -(cy - (ch/2) + pixheight * (y + 0.5)); // y coord of the row

    for (int x = 0; x < N; x++) { // for each column
      double x_coord = cx - (cw/2) + pixwidth * (x + 0.5); // x coord of the column
      double Ro[3] = {cx, cy, cz}; // position of camera
      double Rd[3] = {x_coord, y_coord, 1}; // position of pixel
      normalize(Rd); // normalize (P - Ro)

      double closestT = INFINITY;
      Object* closestObject;
      for (int i = 0; i < numPhysicalObjects; i++) { // loop through the array of objects in the scene
        double t = 0;
        if (physicalObjects[i]->kind == 0) { // plane
          t = plane_intersection(Ro, Rd, physicalObjects[i]->position, physicalObjects[i]->plane.normal);
        }
        else if (physicalObjects[i]->kind == 1) { // sphere
          t = sphere_intersection(Ro, Rd, physicalObjects[i]->position, physicalObjects[i]->sphere.radius);
        }
        else { // ???
          fprintf(stderr, "Unrecognized object.\n");
          exit(1);
        }
        if (t > 0 && t < closestT) { // found a closer t value, save the object data
          closestT = t;
          closestObject = physicalObjects[i];
        }
      }
      // place the pixel into the pixmap array, with illumination
      if (closestT > 0 && closestT != INFINITY) {
        illuminate(closestT, closestObject, Rd, Ro, pixIndex);
        /*pixmap[pixIndex].R = double_to_color(color[0]);
        pixmap[pixIndex].G = double_to_color(color[1]);
        pixmap[pixIndex].B = double_to_color(color[2]);*/
        //free(object); // deallocate mem for object
      }
      else { // make background pixels black
        pixmap[pixIndex].R = 0;
        pixmap[pixIndex].G = 0;
        pixmap[pixIndex].B = 0;
      }
      pixIndex++;
    }
  }
  // finished created image data, write out
  FILE* fh = fopen(filename, "w");
  writeP3(fh);
  // free the pixmap from memory
  free(pixmap);
}

static inline void v3_scale(double* a, double s, double* c) {
  c[0] = s * a[0];
  c[1] = s * a[1];
  c[2] = s * a[2];
}

static inline void v3_add(double* a, double* b, double* c) {
  c[0] = a[0] + b[0];
  c[1] = a[1] + b[1];
  c[2] = a[2] + b[2];
}

static inline void v3_subtract(double* a, double* b, double* c) {
  c[0] = a[0] - b[0];
  c[1] = a[1] - b[1];
  c[2] = a[2] - b[2];
}

static inline double p3_distance(double* a, double* b) {
  return sqrt(sqr(b[0] - a[0]) + sqr(b[1] - a[1]) + sqr(b[2] - a[2]));
}

void illuminate(double colorObjT, Object* colorObj, double* Rd, double* Ro, int pixIndex) {
  // initialize values for color, would be where ambient color goes
  double color[3];
  color[0] = 0;
  color[1] = 0;
  color[2] = 0;

  double closestT = INFINITY;
  Object* closestShadowObj;

  // loop through all the lights in the lights array
  for (int i = 0; i < numLightObjects; i++) {
    double Ron[3]; // position of object
    v3_scale(Rd, colorObjT, Ron);
    v3_add(Ron, Ro, Ron);
    double Rdn[3]; // direction from object to light
    v3_subtract(lightObjects[i]->position, Ron, Rdn);
    normalize(Rdn);

    //printf("Ron: [%lf, %lf, %lf]; Rdn: [%lf, %lf, %lf]\n", Ron[0], Ron[1], Ron[2], Rdn[0], Rdn[1], Rd[2]);

    for (int j = 0; j < numPhysicalObjects; j++) { // loop through all the objects in the array
      double currentT = 0;
      Object* currentObj = physicalObjects[j];

      if (currentObj == colorObj) continue; // skip over the object we are coloring

      if (currentObj->kind == 0) { // plane
        currentT = plane_intersection(Ron, Rdn, currentObj->position, currentObj->plane.normal);
      }
      else if (currentObj->kind == 1) { // sphere
        currentT = sphere_intersection(Ron, Rdn, currentObj->position, currentObj->sphere.radius);
      }
      else { // ???
        fprintf(stderr, "Unrecognized object.\n");
        exit(1);
      }
      double distanceToLight = p3_distance(Ron, lightObjects[i]->position); // distance from object to the light
      if (currentT >= distanceToLight) {
      }
      if (currentT <= distanceToLight && currentT > 0 && currentT < closestT) { // found a closer t value, save the object data
        closestT = currentT; // the current t value is the new closest t value
        closestShadowObj = currentObj;
      }
    }
    if (closestT == INFINITY) { // no shadow
      color[0] = colorObj->specularColor[0];
      color[1] = colorObj->specularColor[1];
      color[2] = colorObj->specularColor[2];
    }
    else {
      color[0] = colorObj->diffuseColor[0];
      color[1] = colorObj->diffuseColor[1];
      color[2] = colorObj->diffuseColor[2];
    }
  }

  pixmap[pixIndex].R = double_to_color(color[0]);
  pixmap[pixIndex].G = double_to_color(color[1]);
  pixmap[pixIndex].B = double_to_color(color[2]);
}


// next_c() wraps the getc() function and provides error checking and line
// number maintenance
int next_c(FILE* json) {
  int c = fgetc(json);
  #ifdef DEBUG
  printf("next_c: '%c'\n", c);
  #endif
  if (c == '\n') {
    line += 1;
  }
  if (c == EOF) {
    fprintf(stderr, "Error: Unexpected end of file on line number %d.\n", line);
    exit(1);
  }
  return c;
}

// expect_c() checks that the next character is d.  If it is not it emits
// an error.
void expect_c(FILE* json, int d) {
  int c = next_c(json);
  if (c == d) return;
  fprintf(stderr, "Error: Expected '%c' on line %d.\n", d, line);
  exit(1);
}

// skip_ws() skips white space in the file.
void skip_ws(FILE* json) {
  int c = next_c(json);
  while (isspace(c)) {
    c = next_c(json);
  }
  ungetc(c, json);
}

// next_string() gets the next string from the file handle and emits an error
// if a string can not be obtained.
char* next_string(FILE* json) {
  char buffer[129];
  int c = next_c(json);
  if (c != '"') {
    fprintf(stderr, "Error: Expected string on line %d.\n", line);
    exit(1);
  }
  c = next_c(json);
  int i = 0;
  while (c != '"') {
    if (i >= 128) {
      fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported.\n");
      exit(1);
    }
    if (c == '\\') {
      fprintf(stderr, "Error: Strings with escape codes are not supported.\n");
      exit(1);
    }
    if (c < 32 || c > 126) {
      fprintf(stderr, "Error: Strings may contain only ascii characters.\n");
      exit(1);
    }
    buffer[i] = c;
    i += 1;
    c = next_c(json);
  }
  buffer[i] = 0;
  return strdup(buffer);
}

// parse the next number in the json file
double next_number(FILE* json) {
  double value;
  fscanf(json, "%lf", &value);
  return value;
}

// parse the next vector in the json file (array of 3 doubles)
double* next_vector(FILE* json) {
  double* v = malloc(3*sizeof(double));
  expect_c(json, '[');
  skip_ws(json);
  v[0] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  v[1] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  v[2] = next_number(json);
  skip_ws(json);
  expect_c(json, ']');
  return v;
}

// parse a json file based on filename and place any objects into object array
void read_scene(char* filename) {

  int c;
  FILE* json = fopen(filename, "r");
  if (json == NULL) {
    fprintf(stderr, "Error: Could not open file \"%s\"\n", filename);
    exit(1);
  }
  int camFlag = 0; // boolean to see if we have a camera obj yet

  // initialize counters
  numPhysicalObjects = 0;
  numLightObjects = 0;

  Object* object; // temp object variable

  skip_ws(json);
  expect_c(json, '['); // Find the beginning of the list
  skip_ws(json);

  // Find the objects
  while (1) {
    c = fgetc(json);
    if (c == ']') {
      fprintf(stderr, "Error: Empty object at line %d.\n", line);
      fclose(json);
      free(object);
      return;
    }
    if (c == '{') {
      object = malloc(sizeof(Object)); // assign memory to hold the new object
      skip_ws(json);
      // Parse the object
      char* key = next_string(json);
      if (strcmp(key, "type") != 0) {
        fprintf(stderr, "Error: Expected \"type\" key on line number %d.\n", line);
        exit(1);
      }
      skip_ws(json);
      expect_c(json, ':');
      skip_ws(json);
      char* value = next_string(json);

      if (strcmp(value, "plane") == 0) {
        object->kind = 0;
      }
      else if (strcmp(value, "sphere") == 0) {
        object->kind = 1;
      }
      else if (strcmp(value, "light") == 0) {
        object->kind = 2;
      }
      else if (strcmp(value, "camera") == 0) {
        if (camFlag == 1) {
          fprintf(stderr, "Error: Too many camera objects, see line: %d.\n", line);
          exit(1);
        }
        object->kind = 3;
        camFlag = 1;
      }
      else {
        fprintf(stderr, "Error: Unknown type, \"%s\", on line number %d.\n", value, line);
        exit(1);
      }
      skip_ws(json);
      int kind = object->kind; // check what kind of object we are parsing
      while (1) { // parse the current object

        c = next_c(json);
        if (c == '}') {
          break; // stop parsing this object
        }
        else if (c == ',') {
          // read another field
          skip_ws(json);
          char* key = next_string(json);
          skip_ws(json);
          expect_c(json, ':');
          skip_ws(json);
          if (strcmp(key, "width") == 0) {
            double value = next_number(json);
            if (kind == 3) {
              object->camera.width = value;
            }
            else {
              fprintf(stderr, "Error: Unexpected 'width' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "height") == 0) {
            double value = next_number(json);
            if (kind == 3) {
              object->camera.height = value;
            }
            else {
              fprintf(stderr, "Error: Unexpected 'height' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "radius") == 0) {
            double value = next_number(json);
            if (kind == 1) {
              object->sphere.radius = value;
            }
            else {
              fprintf(stderr, "Error: Unexpected 'radius' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "color") == 0) {
            double* value = next_vector(json);
            if (kind == 0 || kind == 1 || kind == 2) {
              memcpy(object->color, value, sizeof(double) * 3);
            }
            else {
              fprintf(stderr, "Error: Unexpected 'color' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "diffuse_color") == 0) {
            double* value = next_vector(json);
            if (kind == 0 || kind == 1) {
              memcpy(object->diffuseColor, value, sizeof(double) * 3);
            }
            else {
              fprintf(stderr, "Error: Unexpected 'diffuse_color' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "specular_color") == 0) {
            double* value = next_vector(json);
            if (kind == 0 || kind == 1) {
              memcpy(object->specularColor, value, sizeof(double) * 3);
            }
            else {
              fprintf(stderr, "Error: Unexpected 'specular_color' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "position") == 0) {
            double* value = next_vector(json);
            if (kind == 0 || kind == 1 || kind == 2 || kind == 3) {
              memcpy(object->position, value, sizeof(double) * 3);
            }
            else {
              fprintf(stderr, "Error: Unexpected 'position' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "normal") == 0) {
            double* value = next_vector(json);
            if (kind == 0) {
              memcpy(object->plane.normal, value, sizeof(double) * 3);
            }
            else {
              fprintf(stderr, "Error: Unexpected 'normal' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "direction") == 0) {
            double* value = next_vector(json);
            if (kind == 2) {
              memcpy(object->light.direction, value, sizeof(double) * 3);
            }
            else {
              fprintf(stderr, "Error: Unexpected 'direction' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "radial-a0") == 0) {
            double value = next_number(json);
            if (kind == 2) {
              object->light.radialA0 = value;
            }
            else {
              fprintf(stderr, "Error: Unexpected 'radial-a0' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "radial-a1") == 0) {
            double value = next_number(json);
            if (kind == 2) {
              object->light.radialA1 = value;
            }
            else {
              fprintf(stderr, "Error: Unexpected 'radial-a1' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "radial-a2") == 0) {
            double value = next_number(json);
            if (kind == 2) {
              object->light.radialA2 = value;
            }
            else {
              fprintf(stderr, "Error: Unexpected 'radial-a2' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "angular-a0") == 0) {
            double value = next_number(json);
            if (kind == 2) {
              object->light.angularA0 = value;
            }
            else {
              fprintf(stderr, "Error: Unexpected 'angular-a0' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else {
            fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n", key, line);
            exit(1);
          }
          skip_ws(json);
        }
        else {
          fprintf(stderr, "Error: Unexpected value on line %d\n", line);
          exit(1);
        }
      } // end loop through object fields

      if (camFlag == 0) { // ensure that we parsed a camera
        fprintf(stderr, "Error: The JSON file does not contain a camera object.\n");
        exit(1);
      }

      // place object into the appropriate data structure
      if (kind == 0 || kind == 1) {
        physicalObjects[numPhysicalObjects] = object;
        numPhysicalObjects++;
      }
      else if (kind == 2) {
        lightObjects[numLightObjects] = object;
        numLightObjects++;
      }
      else {
        // set camera position
        object->position[0] = 0;
        object->position[1] = 0;
        object->position[2] = 0;
        cameraObject = object;
      }

      skip_ws(json);
      c = next_c(json);
      if (c == ',') {
        skip_ws(json);
      }
      else if (c == ']') {
        fclose(json);
        free(object);
        return;
      }
      else {
        fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
        exit(1);
      }
    }
  } // end loop through all objects in scene
}

// function to print out all the objects to stdout, for debugging
void printObjs() {
  for (int i = 0; i < numPhysicalObjects; i++) {
    printf("Object %i: type = %i; position = [%lf, %lf, %lf]\n", i, physicalObjects[i]->kind,
    physicalObjects[i]->position[0],
    physicalObjects[i]->position[1],
    physicalObjects[i]->position[2]);
  }
  for (int i = 0; i < numLightObjects; i++) {
    printf("Light Object %i: type = %i; position = [%lf, %lf, %lf]\n", i, lightObjects[i]->kind,
    lightObjects[i]->position[0],
    lightObjects[i]->position[1],
    lightObjects[i]->position[2]);
  }
  printf("Camera: type = %i\n", cameraObject->kind);
}

// function to print out the contents of pixmap to stdout, for debugging
void printPixMap() {
  int i = 0;
  for (int y = 0; y < M; y++) {
    for (int x = 0; x < N; x++) {
      printf("[%i, %i, %i] ", pixmap[i].R, pixmap[i].G, pixmap[i].B); // print the pixel
      i++;
    }
    printf("\n");
  }
}

int main(int args, char** argv) {

  if (args != 5) {
    fprintf(stderr, "Usage: raycast width height input.json output.ppm\n");
    exit(1);
  }

  M = atoi(argv[2]); // save height
  N = atoi(argv[1]); // save width
  numPixels = M * N; // total pixels for output image
  read_scene(argv[3]);
  raycast(argv[4]);
  return 0; // exit success
}
