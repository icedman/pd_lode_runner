#!/bin/bash

# Usage: ./adjust_exposure.sh input.png output.png 0.05 0.95 0.8
#          ^infile      ^outfile  ^black ^white ^gamma

INPUT="$1"
OUTPUT="$2"
BLACK_LEVEL="$3"  # e.g. 0.05
WHITE_LEVEL="$4"  # e.g. 0.95
GAMMA="$5"        # e.g. 0.8

# Convert black and white levels to percentages for ImageMagick
BLACK_PERCENT=$(echo "$BLACK_LEVEL * 100" | bc -l)
WHITE_PERCENT=$(echo "$WHITE_LEVEL * 100" | bc -l)

# Run the command
magick "$INPUT" \
  -channel RGB \
  -level ${BLACK_PERCENT}%x${WHITE_PERCENT}%${GAMMA} \
  +channel \
  "$OUTPUT"
