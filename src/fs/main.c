#include <osZ.h>

public size_t read(fid_t fid, void *buf, size_t size) { return 0; }
public fid_t dup2(fid_t oldfd, fid_t newfd) { return 0; }
public size_t write(void *buf, size_t size, fid_t fid) { return 0; }
public fpos_t seek(fid_t fid, fpos_t offset, int whence) { return 0; }
public fid_t dup(fid_t oldfd) { return 0; }
public int stat(fid_t fd, stat_t *buf) { return 0; }

public void pipe(){}
public void ioctl(){}
