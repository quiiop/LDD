#!/bin/sh
module="seq_file"
device="seq_file"

# invoke rmmod with all arguments we got
sudo /sbin/rmmod $module $*

# Remove stale nodes

sudo rm -f /dev/${device} /dev/${device}[0-3]