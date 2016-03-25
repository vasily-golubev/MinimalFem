#!/bin/sh
./MinimalFEM rect_test3.inp rect_test3.out
python scripts/PostProcessRect.py rect_test3.inp rect_test3.out
