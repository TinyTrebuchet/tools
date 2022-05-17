#!/bin/sh

# If tablet attached
if [[ -n $(/usr/bin/xsetwacom --list) ]]; then
  /bin/sh /home/tiny/.config/scripts/HS611.sh
fi

