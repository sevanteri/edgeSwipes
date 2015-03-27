#!/bin/bash

./edgeSwipes -n synaptics | while read line; do
    activeWindow=$(xdotool getwindowname `xdotool getactivewindow`)

    if [[ $activeWindow == *"Mozilla Firefox"* ]]
    then
        if [ "$line" = "Right" ]; then
            xdotool key ctrl+Page_Down
        elif [ "$line" = "Left" ]; then
            xdotool key ctrl+Page_Up
        fi
    fi
done

