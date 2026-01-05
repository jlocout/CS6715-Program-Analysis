#!/bin/bash

CFILE=$1
clang -E $CFILE -o $CFILE.i