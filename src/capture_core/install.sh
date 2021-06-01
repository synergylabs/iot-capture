#!/usr/bin/env bash

DEPS="libnetfilter-queue-dev libcurl4-openssl-dev lib32stdc++6 libssl-dev zlib1g-dev "
DEPS+=" libconfig++-dev "

sudo apt update -qq
sudo apt install -qq -y $DEPS