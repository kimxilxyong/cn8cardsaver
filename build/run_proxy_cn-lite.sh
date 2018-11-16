#!/bin/sh
# mine is fine

#GPU only miners
#sudo killall ccminer
#sleep 5
# --cuda-launch 16x196 --cuda-bfactor=4 --cuda-bsleep=200
#cd ~/source/nvidia_miner --algo=cryptonight-lite
./cn8cardsaver-nvidia --donate-level 0 --algo=cryptonight-lite -o 192.168.0.248:3333 -u Monero7 -p w=Monero7 --variant 1 --max-gpu-temp=75 --gpu-temp-falloff=7





