#!/bin/bash

julia server.jl > server.log &
sleep 10
julia client.jl > client.log &

wait
