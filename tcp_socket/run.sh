#!/bin/bash
killall ringmaster
killall player
echo "!!!!! START !!!!!"
./ringmaster 1234 100 100&
sleep 1
for i in {1..100}
do
    ./player 127.0.0.1 1234&
done