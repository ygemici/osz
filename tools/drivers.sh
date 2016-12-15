#!/bin/bash

out="../../../../bin/root/etc/sys/drivers"
a=$1
if [ "${a%/*}" == "fs" ]
then
    echo "*	$1" >>$out
fi

[ ! -f devices ] && exit

cat devices | while read line
do
	echo "$line	$1"
done >>$out

