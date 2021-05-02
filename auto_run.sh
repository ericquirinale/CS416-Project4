#!/bin/sh

make clean > /dev/null
make  > /dev/null
./tfs -s /tmp/kdt57/mountdir
#echo "Mounted /tmp/kdt57/mountdir"
cd benchmark
make clean  > /dev/null
make  > /dev/null
#echo "Running simple_test.c"
./simple_test >> ../simplelog
make clean  > /dev/null
cd ..
fusermount -u /tmp/kdt57/mountdir
#echo "Unmounted /tmp/kdt57/mountdir"
make clean  > /dev/null
make > /dev/null
./tfs -s /tmp/kdt57/mountdir
#echo "Mounted /tmp/kdt57/mountdir"
cd benchmark
make clean  > /dev/null
make  > /dev/null
#echo "Running test_cases.c"
./test_case >> ../testlog
make clean  > /dev/null
cd ..
fusermount -u /tmp/kdt57/mountdir 