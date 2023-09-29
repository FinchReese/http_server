#!/bin/bash

build_path="./build"
if [ ! -d "$build_path" ]; then
    mkdir $build_path
else
    rm -rf $build_path/*
fi
cd ${build_path}
cmake ..
make
