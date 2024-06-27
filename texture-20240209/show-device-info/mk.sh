#!/bin/bash

echo "gcc -g -o show-device-info show-device-info.c -lgbm -lEGL -lGLESv2"
gcc -g -o show-device-info show-device-info.c -lgbm -lEGL -lGLESv2
