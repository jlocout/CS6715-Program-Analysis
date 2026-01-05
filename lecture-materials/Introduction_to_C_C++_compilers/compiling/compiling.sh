#!/bin/bash

IFILE=$1
clang -S $IFILE -o $IFILE.s