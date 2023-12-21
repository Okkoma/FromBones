#!/usr/bin/env bash

cd /media/DATA/Dev/Projets/FromBones/exe/gl

./FromBonesUbuntu22.04Gl-static -gamecfg engine_config-net.xml -netmode server -xp 0 -yp 10 -logfile FromBonesLinuxServer.log&

./FromBonesUbuntu22.04Gl-static -gamecfg engine_config-net.xml -netmode client -xp 1920 -yp 10 -logfile FromBonesLinuxClient1.log&

echo "1" > numclients.txt
