#!/bin/bash
g++ kernel.cpp -o kernelout
g++ disk.cpp -o diskout
g++ Process.cpp -o processout
./kernelout ; ./diskout