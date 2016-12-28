#ifndef _SYSCALL_H
#define _SYSCALL_H 1

// system services
#define SRV_core		 0
#define SRV_system  	-1
#define SRV_fs			-2
#define SRV_ui			-3
#define SRV_LASTCRIT	SRV_ui
#define SRV_syslog		-4
#define SRV_net			-5
#define SRV_sound		-6
#define SRV_init		-7
#define SRV_usrfirst	-8
#define SRV_LAST		-NRSRV_MAX
// get function indeces
#include <sys/core.h>
#include <sys/syslog.h>
#include <sys/fs.h>
#include <sys/ui.h>
#include <sys/net.h>

#ifndef _AS
/* async, send a message (non-blocking, except dest queue is full) */
bool_t mq_send(pid_t dst, uint64_t func, uint64_t arg0, uint64_t arg1, uint64_t arg2);
/* sync, send a message and receive result (blocking) */
msg_t *mq_call(pid_t dst, uint64_t func, uint64_t arg0, uint64_t arg1, uint64_t arg2);
/* async, is there a message? (non-blocking) */
bool_t mq_ismsg();
/* sync, wait until there's a message (blocking) */
msg_t *mq_recv();
/* sync, dispatch events (blocking, noreturn) */
void mq_dispatch();
#endif

#endif
