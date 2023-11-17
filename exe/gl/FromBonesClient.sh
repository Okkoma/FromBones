#!/bin/bash

CLIENT=$(<numclients.txt)
(( CLIENT = CLIENT+1 ))
echo "$CLIENT" > numclients.txt

./FromBonesUbuntu22.04Gl-static -gamecfg engine_config-net.xml -netmode client -logfile "FromBonesLinuxClient$CLIENT.log"&
