#!/bin/sh

source /etc/profile
trap "" hup
clear
exec /usr/bin/gmenu2x.bin
