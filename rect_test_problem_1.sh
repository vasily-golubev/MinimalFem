#!/bin/sh
./MinimalFEM rect_test1.inp rect_test1.out
python scripts/PostProcessRect.py rect_test1.inp rect_test1.out
