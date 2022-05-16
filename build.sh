#!/bin/bash 
make clean
make 
mv -f test_cgi wwwroot
rm -rf publish
mkdir publish 
cp -rf wwwroot publish
cp -rf srv publish
./srv 8081
