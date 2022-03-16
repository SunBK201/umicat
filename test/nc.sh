#!/bin/bash
while true; do 
    printf 'HTTP/1.1 200 OK\n\n%s' "$(cat index.html)" | netcat -l 9090; 
done
