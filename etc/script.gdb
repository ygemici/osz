set architecture i386:x86-64:intel
target remote localhost:1234
symbol-file bin/root/lib/sys/core
add-symbol-file bin/root/lib/sys/input/ps2.so 0x2050e8
add-symbol-file bin/root/lib/sys/display/fb.so 0x2070e8
add-symbol-file bin/root/lib/sys/proc/pitrtc.so 0x2090e8
display/i $pc
