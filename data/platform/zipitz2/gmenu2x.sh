#!/bin/sh

source /etc/profile

#setfont2 /usr/share/setfont2/un-fuzzy-6x10-font.pnm
#loadunimap /usr/share/setfont2/ben_uni.trans

/usr/sbin/loadfont < /etc/console-setup/Uni2-Terminus12x6.psf

trap "" hup

clear
exec /usr/bin/gmenu2x.bin
