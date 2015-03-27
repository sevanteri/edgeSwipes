#!/bin/bash

./edgeSwipes | while read line; do
    if [ "$line" = "Left" ]; then
        /home/sevanteri/Copy/Workspace/python/pyqt5/tabletShortcuts/run.sh&
    elif [ "$line" = "Tap_3" ]; then
        if [ `xsetwacom --get 'Wacom ISDv4 E6 Finger touch' Touch` == "off" ]; then
            xsetwacom --set 'Wacom ISDv4 E6 Finger touch' Touch on &
        else
            xsetwacom --set 'Wacom ISDv4 E6 Finger touch' Touch off &
        fi
    elif [ "$line" = "Right" ]; then
        xdotool keydown alt key Tab keyup alt &

    #elif [ "$line" = "Bottom" ]; then
        #cellwriter &

    elif [ "$line" = "Top" ]; then

        monon=`xset q | grep "Monitor is"`
        if [[ "$monon" =~ "Monitor is On" ]]; then
            xset dpms force off & # blank screen
        else
            i3lock
        fi
    fi
done
