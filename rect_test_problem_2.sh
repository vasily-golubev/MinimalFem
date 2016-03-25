#!/bin/sh
./MinimalFEM rect_test2.inp rect_test2.out
python scripts/PostProcessRect.py rect_test2.inp rect_test2.out
