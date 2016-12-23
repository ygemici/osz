#!/bin/bash

a=$1
a="${a##*/}"
readelf --dyn-syms $1 | grep 'FUNC' | grep -v ': 0000000000000000     0 FUNC' | while read line
do
	idx=`echo "$line" | cut -d ':' -f 1`
	fnc=`echo "$line" | cut -d ':' -f 2 | cut -b 53- | cut -d ' ' -f 1 | grep -ve "^_"`
	if [ "$fnc" != "" ]; then
		echo "#define SYS_$fnc		($idx)"
	fi
done >$2/${a}.h
