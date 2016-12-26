#!/bin/bash

out="../../../../bin/root/etc/sys/drivers"
a=$1
[ "${a%/*}" == "fs" ] && echo "*	$1" >>$out
[ ! -f devices ] && exit
cat devices | while read line; do echo "$line	$1"; done >>$out

