#!/bin/sh
set -eu
SLOT=$(echo $1 | sed 's/-.*//')
echo $(expr $SLOT \* 2 - 1)
