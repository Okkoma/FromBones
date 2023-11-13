#!/usr/bin/env bash

echo "0" > numclients.txt

./FromBonesUbuntu22.04Gl-static -gamecfg engine_config-net.xml -netmode server -xp 0 -yp 10 -logfile FromBonesLinuxServer.log&
