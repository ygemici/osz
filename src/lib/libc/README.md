OS/Z Lib C
==========

The most basic library that provides a standardized way to interact with
the operating system and hides the details of low level syscalls.
It's splitted into two parts:

 - platform independent part
 - platform dependent part

The C source files in this directory are the independent ones. Platform
dependent code are organized in sub-directories, and may have .S assembler
sources as well, at least crt0.S .
