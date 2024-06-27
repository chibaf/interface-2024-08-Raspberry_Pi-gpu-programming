#!/bin/bash

echo "gcc -g -o texture-copy texture.c -include shader-copy.h -lgbm -lEGL -lGLESv2"
gcc -g -o texture-copy texture.c -include shader-copy.h -lgbm -lEGL -lGLESv2

echo "gcc -g -o texture-invert texture.c -include shader-invert.h -lgbm -lEGL -lGLESv2"
gcc -g -o texture-invert texture.c -include shader-invert.h -lgbm -lEGL -lGLESv2

echo "gcc -g -o texture-gamma texture.c -include shader-gamma.h -lgbm -lEGL -lGLESv2"
gcc -g -o texture-gamma texture.c -include shader-gamma.h -lgbm -lEGL -lGLESv2
