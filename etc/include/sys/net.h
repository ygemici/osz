#ifndef _SYSCALL_H
#define _SYSCALL_H 1

#define SRV_NET 0x4000

#define SYS_socket (SRV_NET | 0)
#define SYS_connect (SRV_NET | 1)
#define SYS_accept (SRV_NET | 2)
#define SYS_sendto (SRV_NET | 3)
#define SYS_recvfrom (SRV_NET | 4)
#define SYS_sendmsg (SRV_NET | 5)
#define SYS_recvmsg (SRV_NET | 6)
#define SYS_shutdown (SRV_NET | 7)
#define SYS_bind (SRV_NET | 8)
#define SYS_listen (SRV_NET | 9)
#define SYS_getsockname (SRV_NET | 10)
#define SYS_getpeername (SRV_NET | 11)
#define SYS_socketpair (SRV_NET | 12)
#define SYS_setsockopt (SRV_NET | 13)
#define SYS_getsockopt (SRV_NET | 14)

#endif
