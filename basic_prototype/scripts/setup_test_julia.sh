#!/bin/bash

USER='conor'
SERVER_HOST='ackermann.bio.stonybrook.edu'

echo "Copying code and data to $SERVER_HOST..."
scp julia/server.jl julia/client.jl data/processed/r11_spks.h5 $USER@$SERVER_HOST:~/realtime
