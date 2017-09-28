Utilities for the build system
==============================

These C sources are compiled to the host platform, and not for the platform it's generating images for.

- *drivers.sh* shell script to generate `devices` database
- *elftool.c* quick and dirty tool to generate C header files for system services
- *mkfs.c* utility to create [FS/Z](https://github.com/bztsrc/osz/tree/master/docs/fs.md) and disk images
