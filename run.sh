#!/bin/bash
gcc $1 -o ${1%.*} -lGL -lGLU -lglut -lm && ./${1%.*}