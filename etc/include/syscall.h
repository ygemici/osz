#ifndef _SYSCALL_H
#define _SYSCALL_H 1

// subsystems
#define SRV_core	0
#define SRV_system  1
#define SRV_fs		2
#define SRV_ui		3
#define SRV_net		4
#define SRV_sound	5
#define SRV_syslog	6
#define SRV_init	7
#define SRV_NUM     8

#include <sys/core.h>
#include <sys/syslog.h>
#include <sys/fs.h>
#include <sys/ui.h>
#include <sys/net.h>

bool_t clsend(pid_t dst, uint64_t func, uint64_t arg0, uint64_t arg1, uint64_t arg2);
msg_t *clcall(pid_t dst, uint64_t func, uint64_t arg0, uint64_t arg1, uint64_t arg2);
msg_t *clrecv();
bool_t clismsg();

#endif
