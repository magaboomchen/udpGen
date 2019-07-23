#!/bin/sh
set +e

sudo clang -O3 -Wall -Wextra -Wno-unused-parameter \
    -ggdb -g -lm -pthread \
    -o udpreceiver1 udpreceiver1.c \
    net.c

sudo clang -O3 -Wall -Wextra -Wno-unused-parameter \
    -ggdb -g -lm -pthread \
    -o udpsender udpsender.c \
    net.c
