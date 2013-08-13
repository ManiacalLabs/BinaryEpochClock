#!/bin/bash
wget https://pypi.python.org/packages/source/p/pyserial/pyserial-2.6.tar.gz
tar -zxvf pyserial-2.6.tar.gz
cd pyserial-2.6
sudo python setup.py install
