#include <osZ.h>

public uint64_t open(unsigned char *fn) { return 0; }
public void close(uint64_t fid) { }
public uint64_t read(uint64_t fid, unsigned char *buff, uint64_t size) { return 0; }
public uint64_t write(unsigned char *buff, uint64_t fid, uint64_t size) { return 0; }
public void stat(){}
public void seek(){}
public void pipe(){}
public void dup(){}
public void dup2(){}
public void ioctl(){}
