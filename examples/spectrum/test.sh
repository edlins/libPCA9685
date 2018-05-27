#!/bin/sh

cd /usr/local/src/libPCA9685/examples/spectrum/build
./6735l4 plughw:1,0 | ./6735l3 plughw:1,0
