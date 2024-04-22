#!/bin/sh
# this file is derived by the ldd3 load_scull by Rubini
#
# Author    :   Leonardo Suriano <leonardo.suriano@live.it>
#
# $Id: scull_load,v 1.4 2004/11/03 06:19:49 rubini Exp $
# $Id: hello_load,v 1.4 2017/12/21 14:45:49 leonardo Exp $
module="hello"
device="hello"
mode="666"
# Group: since distributions do it differently, look for wheel or use staff
if grep -q '^staff:' /etc/group; then
    group="staff"
else
    group="wheel"
fi

# invoke insmod with all arguments we got
# and use a pathname, as insmod doesn't look in . by default

sudo /sbin/insmod ./$module.ko $*
# retrieve major number
major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)

# Remove stale nodes and replace them, then give gid and perms
# Usually the script is shorter, it's scull that has several devices in it.

echo $major 
#sudo rm -f /dev/${device}
#sudo mknod /dev/${device} c $major 0 # create device file
#sudo chgrp $group /dev/${device}
#sudo chmod $mode  /dev/${device}
