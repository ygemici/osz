#include <osZ.h>

uint64_t open(unsigned char *fn) { return 0; }
void close(uint64_t fid) { }
uint64_t read(uint64_t fid, unsigned char *buff, uint64_t size) { return 0; }
uint64_t write(unsigned char *buff, uint64_t fid, uint64_t size) { return 0; }
void stat(){}
void seek(){}
void pipe(){}
void dup(){}
void dup2(){}
void ioctl(){}

void _init(int argc, char**argv)
{
	while(1);
}
