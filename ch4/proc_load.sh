#!/bin/sh
# this file is derived by the ldd3 load_scull by Rubini
#
# Author    :   Leonardo Suriano <leonardo.suriano@live.it>
#
# $Id: scull_load,v 1.4 2004/11/03 06:19:49 rubini Exp $
# $Id: proc_load,v 1.5 2017/12/21 14:45:49 leonardo Exp $
module="seq_file"
device="seq_file"
mode="666"


# invoke insmod with all arguments we got
# and use a pathname, as insmod doesn't look in . by default

sudo /sbin/insmod ./$module.ko $*