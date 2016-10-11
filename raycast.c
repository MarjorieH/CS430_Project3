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

// Cast the objects in the scene and write the output image to the file
void raycast(char* filename) {

  // camera position
  double cx = 0;
  double cy = 0;
  double cz = 0;

  numPixels = M * N; // total pixels for output image

  double pixheight = ch / M;
  double pixwidth = cw / N;

  // initialize pixmap based on the number of pixels
  pixmap = malloc(sizeof(RGBpixel) * numPixels);
  int pixIndex = 0; // place in pixmap array

  for (int y = 0; y < M; y++) { // for each row
    double y_coord = -(cy - (ch/2) + pixheight * (y + 0.5)); // y coord of the row

    for (int x = 0; x < N; x++) { // for each column
      double x_coord = cx - (cw/2) + pixwidth * (x + 0.5); // x coord of the column
      double Ro[3] = {cx, cy, cz}; // position of camera
      double Rd[3] = {x_coord, y_coord, 1}; // position of pixel
      normalize(Rd); // normalize (P - Ro)

      double best_t = INFINITY;
      Object* object;
      for (int i = 0; i < numObjects; i++) { // loop through the array of objects in the scene
        double t = 0;
        if (objects[i]->kind == 0) { // plane
          t = plane_intersection(Ro, Rd, objects[i]->plane.position, objects[i]->plane.normal);
        }
        else if (objects[i]->kind == 1) { // sphere
          t = sphere_intersection(Ro, Rd, objects[i]->sphere.position, objects[i]->sphere.radius);
        }
        else { // ???
          fprintf(stderr, "Unrecognized object.\n");
          exit(1);
        }
        if (t > 0 && t < best_t) { // found a t value, save the object data
          best_t = t;
          object = malloc(sizeof(Object));
          object = objects[i];
        }
      }
      // place the pixel into the pixmap array
      if (best_t > 0 && best_t != INFINITY) {
        pixmap[pixIndex].R = (unsigned char)(object->color[0] * maxColor);
        pixmap[pixIndex].G = (unsigned char)(object->color[1] * maxColor);
        pixmap[pixIndex].B = (unsigned char)(object->color[2] * maxColor);
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

  int camFlag = 0;
  int c;
  FILE* json = fopen(filename, "r");
  if (json == NULL) {
    fprintf(stderr, "Error: Could not open file \"%s\"\n", filename);
    exit(1);
  }
  numObjects = 0;

  skip_ws(json);
  expect_c(json, '['); // Find the beginning of the list
  skip_ws(json);

  // Find the objects
  while (1) {
    c = fgetc(json);
    if (c == ']') {
      fprintf(stderr, "Error: Empty object at line %d.\n", line);
      fclose(json);
      return;
    }
    if (c == '{') {
      Object* object;
      object = malloc(sizeof(Object));
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
      if (strcmp(value, "camera") == 0) {
        if (camFlag == 1) {
          fprintf(stderr, "Error: Too many camera objects, see line: %d.\n", line);
          exit(1);
        }
        object->kind = -1; // dummy kind for camera
        camFlag = 1;
      }
      else if (strcmp(value, "sphere") == 0) {
        object->kind = 1;
      }
      else if (strcmp(value, "plane") == 0) {
        object->kind = 0;
      }
      else {
        fprintf(stderr, "Error: Unknown type, \"%s\", on line number %d.\n", value, line);
        exit(1);
      }

      skip_ws(json);

      while (1) { // loop through object fields
        c = next_c(json);
        if (c == '}') {
          // stop parsing this object
          break;
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
            if (object->kind < 0 && camFlag == 1) {
              cw = value;
            }
            else {
              fprintf(stderr, "Error: Unexpected 'width' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "height") == 0) {
            double value = next_number(json);
            if (object->kind < 0 && camFlag == 1) {
              ch = value;
            }
            else {
              fprintf(stderr, "Error: Unexpected 'height' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "radius") == 0) {
            double value = next_number(json);
            if (object->kind == 1) {
              object->sphere.radius = value;
            }
            else {
              fprintf(stderr, "Error: Unexpected 'radius' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "color") == 0) {
            double* value = next_vector(json);
            if (object->kind >= 0) {
              memcpy(object->color, value, sizeof(double) * 3);
            }
            else {
              fprintf(stderr, "Error: Unexpected 'color' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "position") == 0) {
            double* value = next_vector(json);
            if (object->kind == 0) {
              memcpy(object->plane.position, value, sizeof(double) * 3);
            }
            else if (object->kind == 1) {
              memcpy(object->sphere.position, value, sizeof(double) * 3);
            }
            else {
              fprintf(stderr, "Error: Unexpected 'position' attribute on line %d.\n", line);
              exit(1);
            }
          }
          else if (strcmp(key, "normal") == 0) {
            double* value = next_vector(json);
            if (object->kind == 0) {
              memcpy(object->plane.normal, value, sizeof(double) * 3);
            }
            else {
              fprintf(stderr, "Error: Unexpected 'normal' attribute on line %d.\n", line);
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

      // place object into the array of objects
      if (object->kind >= 0) {
        objects[numObjects] = object;
        numObjects++;
      }

      skip_ws(json);
      c = next_c(json);
      if (c == ',') {
        skip_ws(json);
      }
      else if (c == ']') {
        fclose(json);
        return;
      }
      else {
        fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
        exit(1);
      }
    }
  } // end loop through all objects in scene
}

int main(int args, char** argv) {

  if (args != 5) {
    fprintf(stderr, "Usage: raycast width height input.json output.ppm\n");
    exit(1);
  }

  M = atoi(argv[2]); // save height
  N = atoi(argv[1]); // save width
  read_scene(argv[3]);
  raycast(argv[4]);
  return 0; // exit success
}
