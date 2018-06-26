#!/bin/bash

if [ -d "datasets" ]; then
rm -rf datasets
fi

mkdir datasets
cd datasets
wget https://www.dropbox.com/s/fdakfnj8xoknzuc/type_repo.txt 
cd ..

if [ -d "build" ]; then
rm -rf build
fi

if [ -f *speedups.txt ]; then
rm *speedups.txt
fi

mkdir build
cd build
cmake ../src
make

printf "   \t1\t2\t4\t8\t16\t32\t64\n" >> speedups.txt

for obj in *.out; do
echo "=============$obj================"
./$obj
done

mv speedups.txt ../
