/*
 * libc/x86_64/dispatch.c
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
 * @brief Message Queue Event Dispatcher
 */

#include <osZ.h>
#include <elf.h>
#include <sysexits.h>
#include <platform.h>

extern uint64_t mq_dispatchcall(Elf64_Sym *sym, msg_t *msg);

// returns errno or does not return at all
public uint64_t mq_dispatch()
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)TEXT_ADDRESS;
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uint8_t *)ehdr+(uint32_t)ehdr->e_phoff);
    Elf64_Sym *sym;
    uint64_t i;

    /* Program Header */
    for(i = 0; i < ehdr->e_phnum; i++){
        /* we only need the Dynamic header */
        if(phdr->p_type==PT_DYNAMIC) {
            /* Dynamic header */
            Elf64_Dyn *d;
            d = (Elf64_Dyn *)((uint32_t)phdr->p_offset+(uint64_t)ehdr);
            while(d->d_tag != DT_NULL) {
                if(d->d_tag == DT_SYMTAB) {
                    sym = (Elf64_Sym *)((uint8_t *)ehdr + (uint32_t)d->d_un.d_ptr);
                    goto found;
                }
                /* move pointer to next dynamic entry */
                d++;
            }
        }
        /* move pointer to next section header */
        phdr = (Elf64_Phdr *)((uint8_t *)phdr + ehdr->e_phentsize);
    }
    seterr(ENOEXEC);
    return EX_SOFTWARE;

found:
    while(1) {
        /* get work */
        msg_t *msg = mq_recv();
        if(EVT_FUNC(msg->evt)>0 &&
           ELF64_ST_TYPE(sym[EVT_FUNC(msg->evt)].st_info) == STT_FUNC
        ) {
            /* make the call */
            i = mq_dispatchcall(sym, msg);
            /* send positive or negative acknowledgement */
            mq_send(EVT_DEST(msg->evt), tcb->errno == SUCCESS ? SYS_ack : SYS_nack,
                i/*return value*/, tcb->errno, 0, 0, 0, 0);
        }
    }
    return EX_OK;
}

