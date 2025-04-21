#!/bin/bash

# bourbon 
./read_cold -m 7 -w -u -c -n 100000000 -f ../../../datasets/libio  -d /mnt/svashi/tmp/bdb -k 16 -v 100 -t 16 --seed 1234
./read_cold -m 7 -w -u -c -n 100000000 -f ../../../datasets/genome -d /mnt/svashi/tmp/bdb -k 16 -v 100 -t 16 --seed 1234
./read_cold -m 7 -w -u -c -n 100000000 -f ../../../datasets/covid  -d /mnt/svashi/tmp/bdb -k 19 -v 100 -t 16 --seed 1234
#./read_cold -m 7 -w -u -c -n 100000000 -f ../../../datasets/osm    -d /mnt/svashi/tmp/bdb -k 19 -v 100 -t 16 --seed 1234

# wisckey
#./read_cold -m 8 -w -u -c -n 100000000 -f ../../../datasets/libio  -d /mnt/svashi/tmp/bdb -k 16 -v 100 -t 16 --seed 1234
#./read_cold -m 8 -w -u -c -n 100000000 -f ../../../datasets/genome -d /mnt/svashi/tmp/bdb -k 16 -v 100 -t 16 --seed 1234
#./read_cold -m 8 -w -u -c -n 100000000 -f ../../../datasets/covid  -d /mnt/svashi/tmp/bdb -k 19 -v 100 -t 16 --seed 1234
#./read_cold -m 8 -w -u -c -n 100000000 -f ../../../datasets/osm    -d /mnt/svashi/tmp/bdb -k 19 -v 100 -t 16 --seed 1234
