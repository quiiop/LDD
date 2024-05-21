#! /bin/sh
module="pipe_simple_sleep"
device="pipe_dev"
mode="666"
group=0

sudo /sbin/insmod ./$module.ko $*
sudo rm -f /dev/${device}[0-3]

# 別忘了，這裡我們一口氣申請多個設備
major=$(awk -v device="$device" '$2==device {print $1}' /proc/devices)
echo "Major number is: $major"

sudo mknod /dev/${device}0 c $major 0
sudo mknod /dev/${device}1 c $major 1
sudo mknod /dev/${device}2 c $major 2
sudo mknod /dev/${device}3 c $major 3

# chgrp $group /dev/$device[0-3]
# chmod $mode /dev/$device[0-3]