#! /bin/sh
module="pipe_simple_sleep"
device="pipe_simple_sleep"
mode="666"
group=0

sudo rm -f /dev/${device}[0-3]
sudo rmmod $module 