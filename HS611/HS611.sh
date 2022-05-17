#!/bin/sh

# For tablet configuration

sleep 1

# Stylus buttons map to MB 2,3 respectively. MB 1 is mapped to tip of stylus
# Pad buttons map to MB 1,2,3,8,9,10,11...
# MB 1,2,3 are left, middle and right click respectively
# MB 4,5,6,7 are reserved in X11 for horizontal and vertical scrolling
# Hence, MB 4,5,6,7 are skipped in Pad buttons

PADID=$(/usr/bin/xsetwacom --list | grep "PAD" | grep -Eo "id: [0-9]*" | awk '{print $2}')
/usr/bin/xsetwacom --set $PADID Button 1  10
/usr/bin/xsetwacom --set $PADID Button 2   2
/usr/bin/xsetwacom --set $PADID Button 3   3
/usr/bin/xsetwacom --set $PADID Button 8  13
/usr/bin/xsetwacom --set $PADID Button 9  14
/usr/bin/xsetwacom --set $PADID Button 10 15
/usr/bin/xsetwacom --set $PADID Button 11 16
/usr/bin/xsetwacom --set $PADID Button 12 17
/usr/bin/xsetwacom --set $PADID Button 13 18
/usr/bin/xsetwacom --set $PADID Button 14 19

STYLUSID=$(/usr/bin/xsetwacom --list | grep "STYLUS" | grep -Eo "id: [0-9]*" | awk '{print $2}')
/usr/bin/xsetwacom --set $STYLUSID Button 1  1
/usr/bin/xsetwacom --set $STYLUSID Button 2  21
/usr/bin/xsetwacom --set $STYLUSID Button 3  22

/usr/bin/pkill xbindkeys
/usr/bin/xbindkeys
