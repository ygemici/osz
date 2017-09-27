/*
 * core/AArch64/task.c
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
 * @brief Task functions, platform dependent code
 */

#include "arch.h"

dataseg phy_t idle_mapping;
dataseg phy_t core_mapping;
dataseg phy_t identity_mapping;
dataseg phy_t *stack_ptr;

/**
 * create a task, allocate memory for it and init TCB
 */
tcb_t *task_new(char *cmdline, uint8_t prio)
{
}

/**
 * Check task address space consistency
 */
bool_t task_check(tcb_t *tcb, phy_t *paging)
{
}

/**
 * Map a core buffer into task's memory
 */
virt_t task_mapbuf(void *buf, uint64_t npages)
{
}
