OS/Z Core
=========

This directory holds the source of the code running in supervisor mode.
It's splitted into two parts:

 - platform independent part
 - platform dependent part

The C source files in this directory are the independent ones. Platform
dependent code are organized in sub-directories, and may have .S assembler
sources as well, at least start.S .
