set architecture i386:x86-64
target remote localhost:1234
symbol-file bin/initrd/sys/core
display/i $pc
