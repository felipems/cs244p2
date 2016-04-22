#!/bin/bash
sudo ./install_dependencies.sh

cd mahimahi && ./autogen.sh && ./configure && make
sudo make install

cd ../sourdough-master && ./autogen.sh && ./configure && make
sudo sysctl -w net.ipv4.ip_forward=1
cd datagrump