#!/bin/bash

killall player
echo "!!!!! START !!!!!"
sleep 1
for i in {1..50}
do
    valgrind ./player 127.0.0.1 1234&
done