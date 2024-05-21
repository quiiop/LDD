#!/bin/sh
module="poll"
device="poll" #這裡要和alloc_chrdev_region(&devno, poll_minor, POLL_DEV_NR, MODULE_NAME)，MODULE_NAME相同
mode="666"
group=0

sudo insmod ./$module.ko $* || exit 1
sudo rm -f /dev/${device}
major=$(awk -v device="$device" '$2==device {print $1}' /proc/devices)
echo "Major number is: $major"

sudo mknod /dev/${device} c $major 0
sudo chgrp $group /dev/$device
sudo chmod $mode /dev/$device