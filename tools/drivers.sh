#!/bin/bash

[ ! -f devices ] && exit

cat devices | while read line
do
	echo "$line	$1"
done >>../../../../bin/root/etc/sys/drivers

