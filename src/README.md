==========================
Getting started
==========================

build the project
    make

clean the project 
    make clean


==========================
Description of Files
==========================

After make, there is 4 executable file
./Benchmark
    ./Benchmark -f /dev/sdb1 -s 10000 -n 10000 (-f filepath, -s hashtable_size, -n insert_record_num)

./CreateDb
    temporary test file for create database on ssd device (the path is hardcode in CreateDb.cc !!!)

./ExampleKV
    temporary test file for insert, get and delete 3000 pairs of data on ssd device (the path is hardcode in ExampleKV.cc !!!)

./DbTest
    temporary test file for insert same key with different value.(the path is hardcode in DbTest.cc !!!) 
