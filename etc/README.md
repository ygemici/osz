Additional assets
=================

Skeletons for directories

- *etc* classic UNIX etc directory
- *include* C header files, will be mounted as /usr/include
- *root* the root user's home

Files

- *CONFIG* the default boot configuration. Must be 4096 bytes long.
  Will be copied to FS0:\BOOTBOOT\CONFIG on disk assembly.
- *bochs.rc* bochs configuration file
- *system.8x16.psf* PC Screen Font, unicode font for kprintf
- *system.8x16.txt* font source, use [writepsf](https://github.com/talamus/solarize-12x29-psf/blob/master/writepsf) to compile
