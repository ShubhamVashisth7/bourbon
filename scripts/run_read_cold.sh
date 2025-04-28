#!/bin/bash

# bourbon 
echo "Running Bourbon"
./read_cold -m 7 -w -u -n 10000000 -f ../../../datasets/libio  -d /mnt/svashi/tmp/bdb -k 16 -v 100 --seed 62
./read_cold -m 7 -w -u -n 10000000 -f ../../../datasets/genome -d /mnt/svashi/tmp/bdb -k 16 -v 100 --seed 62
./read_cold -m 7 -w -u -n 10000000 -f ../../../datasets/covid  -d /mnt/svashi/tmp/bdb -k 19 -v 100 --seed 62
#./read_cold -m 7 -w -u -c -n 100000000 -f ../../../datasets/osm    -d /mnt/svashi/tmp/bdb -k 19 -v 100 -t 16 --seed 1234

./read_cold -m 7 -w -u -n 10000000 -f ../../../datasets/libio  -d /mnt/svashi/tmp/bdb -k 16 -v 100 --mix 10 --seed 62
./read_cold -m 7 -w -u -n 10000000 -f ../../../datasets/genome -d /mnt/svashi/tmp/bdb -k 16 -v 100 --mix 10 --seed 62
./read_cold -m 7 -w -u -n 10000000 -f ../../../datasets/covid  -d /mnt/svashi/tmp/bdb -k 19 -v 100 --mix 10 --seed 62
#./read_cold -m 7 -w -u -c -n 100000000 -f ../../../datasets/osm    -d /mnt/svashi/tmp/bdb -k 19 -v 100 -t 16 --mix 10 --seed 1234

# wisckey
echo "Running Wisckey"
./read_cold -m 8 -w -u -n 10000000 -f ../../../datasets/libio  -d /mnt/svashi/tmp/bdb -k 16 -v 100 --seed 62
./read_cold -m 8 -w -u -n 10000000 -f ../../../datasets/genome -d /mnt/svashi/tmp/bdb -k 16 -v 100 --seed 62
./read_cold -m 8 -w -u -n 10000000 -f ../../../datasets/covid  -d /mnt/svashi/tmp/bdb -k 19 -v 100 --seed 62
./read_cold -m 8 -w -u -n 10000000 -f ../../../datasets/osm    -d /mnt/svashi/tmp/bdb -k 19 -v 100 --seed 62

./read_cold -m 8 -w -u -n 10000000 -f ../../../datasets/libio  -d /mnt/svashi/tmp/bdb -k 16 -v 100 --mix 10 --seed 62
./read_cold -m 8 -w -u -n 10000000 -f ../../../datasets/genome -d /mnt/svashi/tmp/bdb -k 16 -v 100 --mix 10 --seed 62
./read_cold -m 8 -w -u -n 10000000 -f ../../../datasets/covid  -d /mnt/svashi/tmp/bdb -k 19 -v 100 --mix 10 --seed 62
./read_cold -m 8 -w -u -n 10000000 -f ../../../datasets/osm    -d /mnt/svashi/tmp/bdb -k 19 -v 100 --mix 10 --seed 62