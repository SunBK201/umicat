#!/bin/bash
wrk -t2 -c200 -d10s http://127.0.0.1:10201/