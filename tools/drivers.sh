#!/bin/bash

out="../../../../bin/initrd/sys/drivers"
a=$1
[ "${a%/*}" == "fsdrv" ] && echo "*	$1" >>$out
[ ! -f devices -o "`grep "$1" $out`" != "" ] && exit
cat devices | while read line; do echo "$line	$1"; done >>$out

