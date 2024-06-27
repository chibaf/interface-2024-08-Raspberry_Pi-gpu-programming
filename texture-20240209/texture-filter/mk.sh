#!/bin/bash

echo "gcc -O2 -g -o texture-gb-filter-gpu texture-filter.c -include shader-gb-filter.h -lgbm -lEGL -lGLESv2"
gcc -O2 -g -o texture-gb-filter-gpu texture-filter.c -include shader-gb-filter.h -lgbm -lEGL -lGLESv2

echo "gcc -O2 -g -o texture-usm-gpu texture-filter.c -include shader-usm.h -lgbm -lEGL -lGLESv2"
gcc -O2 -g -o texture-usm-gpu texture-filter.c -include shader-usm.h -lgbm -lEGL -lGLESv2

echo "gcc -O2 -g -o texture-gb-filter-cpu texture-gb-filter-cpu.c"
gcc -O2 -g -o texture-gb-filter-cpu texture-gb-filter-cpu.c

echo "gcc -O2 -g -o texture-usm-cpu texture-usm-cpu.c"
gcc -O2 -g -o texture-usm-cpu texture-usm-cpu.c
