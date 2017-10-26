#include "./lerp.h"

#ifndef COLOR_H
#define COLOR_H

typedef struct Color {
  int r;
  int g;
  int b;
} Color;

Color lerpColor(Color current, Color target);

bool checkColor(Color);

#endif
