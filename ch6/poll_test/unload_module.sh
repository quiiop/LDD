#!/bin/sh
module="poll"
device="poll"
mode="666"
group=0

sudo rm -f /dev/${device}
sudo rmmod $module || exit 1