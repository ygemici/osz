/*
 * core/x86_64/tcb.h
 * 
 * Copyright 2016 CC-by-nc-sa bztsrc@github
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * 
 * You are free to:
 *
 * - Share — copy and redistribute the material in any medium or format
 * - Adapt — remix, transform, and build upon the material
 *     The licensor cannot revoke these freedoms as long as you follow
 *     the license terms.
 * 
 * Under the following terms:
 *
 * - Attribution — You must give appropriate credit, provide a link to
 *     the license, and indicate if changes were made. You may do so in
 *     any reasonable manner, but not in any way that suggests the
 *     licensor endorses you or your use.
 * - NonCommercial — You may not use the material for commercial purposes.
 * - ShareAlike — If you remix, transform, or build upon the material,
 *     you must distribute your contributions under the same license as
 *     the original.
 *
 * @brief Thread Control Block, architecture specific part
 */

#include <limits.h>

/* WARNING: must match msghdr_t, for asm */
//#define OSZ_mq_start (MQ_ADDRESS)
//#define OSZ_mq_end (MQ_ADDRESS + 8)

/* WARNING: must match sizeof(msg_t), for asm */
#define OSZ_msg_size 32

#ifndef _AS

// structure at MQ_ADDRESS
typedef struct {
    uint64_t mq_start;    // event queue circular buffer start
    uint64_t mq_end;
    uint64_t mq_size;
    uint64_t mq_recvfrom;
} __attribute__((packed)) msghdr_t;

#endif
