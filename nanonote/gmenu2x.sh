#!/bin/sh

source /etc/profile

setfont2 /usr/share/setfont2/un-fuzzy-6x10-font.pnm
loadunimap /usr/share/setfont2/ben_uni.trans
clear

cd /usr/share/gmenu2x
./gmenu2x 
