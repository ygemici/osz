#ifndef _SYSCALL_H
#define _SYSCALL_H 1

// subsystems
#define SRV_init	0
#define SRV_syslog	1
#define SRV_fs		2
#define SRV_ui		3
#define SRV_net		4
#define SRV_sound	5
#define SRV_system  7
#define SRV_NUM     8

#include <sys/core.h>
#include <sys/syslog.h>
#include <sys/fs.h>
#include <sys/ui.h>
#include <sys/net.h>

#endif
