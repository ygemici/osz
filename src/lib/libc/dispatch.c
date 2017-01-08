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

extern uint64_t mq_dispatchcall(
    uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5,
    virt_t func);

// returns errno or does not return at all
public uint64_t mq_dispatch()
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)TEXT_ADDRESS;
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uint8_t *)ehdr+(uint32_t)ehdr->e_phoff);
    Elf64_Sym *sym = NULL, *sym_end = NULL;
    uint64_t i, maxfunc, syment = 0;
i/=0;
    /* Program Header */
    for(i = 0; i < ehdr->e_phnum; i++){
        /* we only need the Dynamic header */
        if(phdr->p_type==PT_DYNAMIC) {
            /* Dynamic header */
            Elf64_Dyn *d;
            d = (Elf64_Dyn *)((uint32_t)phdr->p_offset+(uint64_t)ehdr);
            while(d->d_tag != DT_NULL && (sym==NULL || sym_end==NULL || syment==0)) {
                if(d->d_tag == DT_STRTAB) {
                    sym_end = (Elf64_Sym *)((uint8_t *)ehdr + (uint32_t)(d->d_un.d_ptr&0xFFFFFF));
                }
                if(d->d_tag == DT_SYMTAB) {
                    sym = (Elf64_Sym *)((uint8_t *)ehdr + (uint32_t)(d->d_un.d_ptr&0xFFFFFF));
                }
                if(d->d_tag == DT_SYMENT) {
                    syment = (uint16_t)d->d_un.d_val;
                }
                /* move pointer to next dynamic entry */
                d++;
            }
            break;
        }
        /* move pointer to next section header */
        phdr = (Elf64_Phdr *)((uint8_t *)phdr + ehdr->e_phentsize);
    }
    /* failsafe to avoid division by zero */
    if(syment == 0)
        syment = sizeof(Elf64_Sym);
    maxfunc = ((virt_t)sym_end - (virt_t)sym)/syment;
    /* checks */
    if(sym==NULL || sym_end==NULL || sym_end<=sym || syment<8 || maxfunc>SHRT_MAX) {
        seterr(ENOEXEC);
        return EX_SOFTWARE;
    }

    /* endless loop */
    while(1) {
        /* get work */
        msg_t *msg = mq_recv();
        uint16_t func = EVT_FUNC(msg->evt);
        Elf64_Sym *symfunc = (Elf64_Sym*)((virt_t)sym + (virt_t)(func * syment));
        /* is it a valid call? */
        if(func > 0 && func < maxfunc &&                       // within boundaries?
           ELF64_ST_BIND(symfunc->st_info) == STB_GLOBAL &&    // is it a global function?
           ELF64_ST_TYPE(symfunc->st_info) == STT_FUNC
        ) {
            seterr(SUCCESS);
            /* make the call */
            i = mq_dispatchcall(
                  msg->arg0, msg->arg1, msg->arg2, msg->arg3, msg->arg4, msg->arg5,
                  (virt_t)symfunc->st_value + (virt_t)TEXT_ADDRESS
                );
        } else {
            seterr(EPERM);
            i = false;
        }
        /* send positive or negative acknowledgement back to the caller */
        mq_send(msg->evt & ~USHRT_MAX, errno == SUCCESS ? SYS_ack : SYS_nack,
            i/*return value*/, errno, 0, 0, 0, 0);
    }
    /* should never reach this */
    return EX_OK;
}

