/*
 * core/x86_64/disasm.c
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
 * @brief Disassembler
 */

#if DEBUG

/* this function is used to print to a string */
extern char *sprintf(char *dst,char* fmt, ...);
/* this variable is set if the instruction has a symbol */
extern virt_t dbg_comment;
/* this variable should be set by page fault handler if address is unavailable */
extern uint8_t sys_fault;

enum {
    DA_BYTE,
    DA_WORD,
    DA_LONG,
    DA_QUAD,
    DA_SNGL,
    DA_DBLR,
    DA_EXTR,
    DA_SDEP,
    DA_NONE,
    DA_MODRM=0x80
};

enum {
    DA_INVALID,
    DA_E,
    DA_EInd,
    DA_Ew,
    DA_Eb,
    DA_R,
    DA_Rw,
    DA_Ri,
    DA_S,
    DA_Si,
    DA_A,
    DA_BX,
    DA_CL,
    DA_DX,
    DA_SI,
    DA_DI,
    DA_CR,
    DA_DR,
    DA_TR,
    DA_I,
    DA_Is,
    DA_Ib,
    DA_Ibs,
    DA_Iw,
    DA_Iq,
    DA_O,
    DA_Db,
    DA_Dl,
    DA_o1,
    DA_o3,
    DA_OS,
    DA_ST,
    DA_STI,
    DA_X,
    DA_XA,
    DA_Ril,
    DA_Iba,
    DA2_A_Ri  = DA_A*256+DA_Ri,
    DA2_I_Ri  = DA_I*256+DA_Ri,
    DA2_Ib_Ri = DA_Ib*256+DA_Ri,
    DA2_Iq_Ib = DA_Iq*256+DA_Ib,
    DA2_E_R   = DA_E*256+DA_R,
    DA2_E_A   = DA_E*256+DA_A,
    DA2_O_A   = DA_O*256+DA_A,
    DA2_A_O   = DA_A*256+DA_O,
    DA2_Eb_R  = DA_Eb*256+DA_R,
    DA2_Ew_R  = DA_Ew*256+DA_R,
    DA2_CL_E  = DA_CL*256+DA_E,
    DA2_R_E   = DA_R*256+DA_E,
    DA2_I_E   = DA_I*256+DA_E,
    DA2_Ib_E  = DA_Ib*256+DA_E,
    DA2_I_EInd= DA_I*256+DA_EInd,
    DA2_I_A   = DA_I*256+DA_A,
    DA2_Ib_A  = DA_Ib*256+DA_A,
    DA2_A_Ib  = DA_A*256+DA_Ib,
    DA2_DX_A  = DA_DX*256+DA_A,
    DA2_A_DX  = DA_A*256+DA_DX,
    DA2_CR_E  = DA_CR*256+DA_E,
    DA2_DR_E  = DA_DR*256+DA_E,
    DA2_TR_E  = DA_DR*256+DA_E,
    DA2_E_CR  = DA_E*256+DA_CR,
    DA2_E_DR  = DA_E*256+DA_DR,
    DA2_E_TR  = DA_E*256+DA_TR,
    DA2_S_Ew  = DA_S*256+DA_Ew,
    DA2_Ew_S  = DA_Ew*256+DA_S,
    DA2_STI_ST= DA_STI*256+DA_ST,
    DA2_ST_STI= DA_ST*256+DA_STI,
    DA2_SI_DI = DA_SI*256+DA_DI,
    DA2_DX_DI = DA_DX*256+DA_DI,
    DA2_SI_DX = DA_SI*256+DA_DX,
    DA2_o1_E  = DA_o1*256+DA_E,
    DA3_Ib_R_E= DA_Ib*65536+DA_R*256+DA_E,
    DA3_CL_R_E= DA_CL*65536+DA_R*256+DA_E,
    DA3_Ib_E_R= DA_Ib*65536+DA_E*256+DA_R,
    DA3_CL_E_R= DA_CL*65536+DA_E*256+DA_R,
    DA3_I_E_R = DA_I*65536+DA_E*256+DA_R
};

/* opcodes
 *  1 byte: inst_tbl
 *  2 bytes: first byte 0f, inst_tbl0f -> inst0fX
 */
typedef struct {
    char *name;
    uint8_t size;
    uint32_t mode;
    void *ext;
} instdesc;

char __attribute__ ((section (".data"))) *grp1[] = {
    "add",
    "or",
    "adc",
    "sbb",
    "and",
    "sub",
    "xor",
    "cmp"
};

char __attribute__ ((section (".data"))) *grp2[] = {
    "rol",
    "ror",
    "rcl",
    "rcr",
    "shl",
    "shr",
    "sal",
    "sar"
};

instdesc __attribute__ ((section (".data"))) grp3[] = {
    { "test",   DA_NONE+DA_MODRM, DA2_I_E, 0 },
    { "test",   DA_NONE+DA_MODRM, DA2_I_E, 0 },
    { "not",    DA_NONE+DA_MODRM, DA_E, 0 },
    { "neg",    DA_NONE+DA_MODRM, DA_E, 0 },
    { "mul",    DA_NONE+DA_MODRM, DA2_I_E, 0 },
    { "imul",   DA_NONE+DA_MODRM, DA2_I_E, 0 },
    { "div",    DA_NONE+DA_MODRM, DA2_I_E, 0 },
    { "idiv",   DA_NONE+DA_MODRM, DA2_I_E, 0 },
};

instdesc __attribute__ ((section (".data"))) grp4[] = {
    { "inc",    DA_BYTE+DA_MODRM, DA_E, 0 },
    { "dec",    DA_BYTE+DA_MODRM, DA_E, 0 },
    { "",       DA_NONE+DA_MODRM, 0, 0 },
    { "",       DA_NONE+DA_MODRM, 0, 0 },
    { "",       DA_NONE+DA_MODRM, 0, 0 },
    { "",       DA_NONE+DA_MODRM, 0, 0 },
    { "",       DA_NONE+DA_MODRM, 0, 0 },
    { "",       DA_NONE+DA_MODRM, 0, 0 }
};

instdesc __attribute__ ((section (".data"))) grp5[] = {
    { "inc",    DA_LONG+DA_MODRM, DA_E, 0 },
    { "dec",    DA_LONG+DA_MODRM, DA_E, 0 },
    { "call",   DA_QUAD+DA_MODRM, DA_EInd, 0 },
    { "lcall",  DA_NONE+DA_MODRM, DA_EInd, 0 },
    { "jmp",    DA_NONE+DA_MODRM, DA_EInd, 0 },
    { "ljmp",   DA_NONE+DA_MODRM, DA_EInd, 0 },
    { "push",   DA_QUAD+DA_MODRM, DA_E, 0 },
    { "",       DA_NONE+DA_MODRM, 0, 0 }
};

char __attribute__ ((section (".data"))) *grp6[] = {
    "sldt",
    "str",
    "lldt",
    "ltr",
    "verr",
    "verw",
    "",
    ""
};

char __attribute__ ((section (".data"))) *inst0f00[] = {
    "invplg",
    "swapgs",
    "rdtscp",
    "",
    "",
    "",
    "",
    ""
};

instdesc __attribute__ ((section (".data"))) grp7[] = {
    { "sgdt",   DA_NONE+DA_MODRM, DA_Rw, 0 },
    { "sidt",   DA_NONE+DA_MODRM, DA_Rw, 0 },
    { "lgdt",   DA_NONE+DA_MODRM, DA_Rw, 0 },
    { "lidt",   DA_NONE+DA_MODRM, DA_Rw, 0 },
    { "smsw",   DA_NONE+DA_MODRM, DA_Rw, 0 },
    { "",       DA_NONE+DA_MODRM, DA_Rw, 0 },
    { "lmsw",   DA_NONE+DA_MODRM, DA_Rw, 0 },
    { NULL,     DA_NONE+DA_MODRM, DA_Rw, inst0f00 }
};

char __attribute__ ((section (".data"))) *grp8[] = {
    "",
    "",
    "",
    "",
    "bt",
    "bts",
    "btr",
    "btc"
};

char __attribute__ ((section (".data"))) *grp9[] = {
    "",
    "cmpxchg8b",
    "",
    "",
    "",
    "",
    "",
    ""
};

char __attribute__ ((section (".data"))) *grpA[] = {
    "",
    "cmpxchg8b",
    "",
    "",
    "",
    "",
    "",
    ""
};

char __attribute__ ((section (".data"))) *grpB[] = {
    "xstorerng",
    "xcryptecb",
    "xcryptcbc",
    "",
    "xcryptcfb",
    "xcryptofb",
    "",
    ""
};

char __attribute__ ((section (".data"))) *inst0f71[] = {
    "",
    "",
    "psrlw",
    "",
    "psraw",
    "",
    "psllw",
    ""
};

char __attribute__ ((section (".data"))) *inst0f72[] = {
    "",
    "",
    "psrld",
    "",
    "psrad",
    "",
    "pslld",
    ""
};

char __attribute__ ((section (".data"))) *inst0f73[] = {
    "",
    "psrlq",
    "psrldq",
    "psraq",
    "",
    "psllq",
    "pslldq"
};

char __attribute__ ((section (".data"))) *inst0fae[] = {
    "sfence",
    "clflush",
    "",
    "",
    "",
    "",
    "",
    ""
};

instdesc __attribute__ ((section (".data"))) inst0f0[] = {
    { NULL,     DA_NONE+DA_MODRM, DA_Ew, grp6 },
    { NULL,     DA_NONE+DA_MODRM, DA_Ew, grp7 },
    { "lar",    DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "lsl",    DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "",       DA_NONE, 0, 0 },
    { "syscall",DA_NONE, 0, 0 },
    { "clts",   DA_NONE, 0, 0 },
    { "sysret", DA_NONE, 0, 0 },

    { "invd",   DA_NONE, 0, 0 },
    { "wbinvd", DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 }
};

instdesc __attribute__ ((section (".data"))) inst0f2[] = {
    { "mov",    DA_LONG+DA_MODRM, DA2_CR_E, 0 },
    { "mov",    DA_LONG+DA_MODRM, DA2_DR_E, 0 },
    { "mov",    DA_LONG+DA_MODRM, DA2_E_CR, 0 },
    { "mov",    DA_LONG+DA_MODRM, DA2_E_DR, 0 },
    { "mov",    DA_LONG+DA_MODRM, DA2_TR_E, 0 },
    { "",       DA_NONE, 0, 0 },
    { "mov",    DA_LONG+DA_MODRM, DA2_E_TR, 0 },
    { "",       DA_NONE, 0, 0 },

    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 }
};

instdesc __attribute__ ((section (".data"))) inst0f3[] = {
    { "wrmsr",  DA_NONE, 0, 0 },
    { "rdtsc",  DA_NONE, 0, 0 },
    { "rdmsr",  DA_NONE, 0, 0 },
    { "rdpmc",  DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },

    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 }
};

instdesc __attribute__ ((section (".data"))) inst0f7[] = {
    { "",       DA_NONE, 0, 0 },
    { NULL,     DA_NONE, 0, inst0f71 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },

    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 }
};

instdesc __attribute__ ((section (".data"))) inst0f8[] = {
    { "jo",     DA_NONE, DA_Dl, 0 },
    { "jno",    DA_NONE, DA_Dl, 0 },
    { "jb",     DA_NONE, DA_Dl, 0 },
    { "jnb",    DA_NONE, DA_Dl, 0 },
    { "jz",     DA_NONE, DA_Dl, 0 },
    { "jnz",    DA_NONE, DA_Dl, 0 },
    { "jbe",    DA_NONE, DA_Dl, 0 },
    { "jnbe",   DA_NONE, DA_Dl, 0 },

    { "js",     DA_NONE, DA_Dl, 0 },
    { "jns",    DA_NONE, DA_Dl, 0 },
    { "jp",     DA_NONE, DA_Dl, 0 },
    { "jnp",    DA_NONE, DA_Dl, 0 },
    { "jl",     DA_NONE, DA_Dl, 0 },
    { "jnl",    DA_NONE, DA_Dl, 0 },
    { "jle",    DA_NONE, DA_Dl, 0 },
    { "jnle",   DA_NONE, DA_Dl, 0 }
};

instdesc __attribute__ ((section (".data"))) inst0f9[] = {
    { "seto",   DA_NONE+DA_MODRM, DA_Eb, 0 },
    { "setno",  DA_NONE+DA_MODRM, DA_Eb, 0 },
    { "setb",   DA_NONE+DA_MODRM, DA_Eb, 0 },
    { "setnb",  DA_NONE+DA_MODRM, DA_Eb, 0 },
    { "setz",   DA_NONE+DA_MODRM, DA_Eb, 0 },
    { "setnz",  DA_NONE+DA_MODRM, DA_Eb, 0 },
    { "setbe",  DA_NONE+DA_MODRM, DA_Eb, 0 },
    { "setnbe", DA_NONE+DA_MODRM, DA_Eb, 0 },

    { "sets",   DA_NONE+DA_MODRM, DA_Eb, 0 },
    { "setns",  DA_NONE+DA_MODRM, DA_Eb, 0 },
    { "setp",   DA_NONE+DA_MODRM, DA_Eb, 0 },
    { "setnp",  DA_NONE+DA_MODRM, DA_Eb, 0 },
    { "setl",   DA_NONE+DA_MODRM, DA_Eb, 0 },
    { "setnl",  DA_NONE+DA_MODRM, DA_Eb, 0 },
    { "setle",  DA_NONE+DA_MODRM, DA_Eb, 0 },
    { "setnle", DA_NONE+DA_MODRM, DA_Eb, 0 }
};

instdesc __attribute__ ((section (".data"))) inst0fa[] = {
    { "push",   DA_QUAD, DA_Si, 0 },
    { "pop",    DA_QUAD, DA_Si, 0 },
    { "cpuid",  DA_NONE, 0, 0 },
    { "bt",     DA_LONG+DA_MODRM, DA2_R_E, 0 },
    { "shld",   DA_LONG+DA_MODRM, DA3_Ib_R_E, 0 },
    { "shld",   DA_LONG+DA_MODRM, DA3_CL_R_E, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE+DA_MODRM, 0, grpB },

    { "push",   DA_QUAD, DA_Si, 0 },
    { "pop",    DA_QUAD, DA_Si, 0 },
    { "",       DA_NONE, 0, 0 },
    { "bts",    DA_LONG+DA_MODRM, DA2_R_E, 0 },
    { "shrd",   DA_LONG+DA_MODRM, DA3_Ib_R_E, 0 },
    { "shrd",   DA_LONG+DA_MODRM, DA3_CL_R_E, 0 },
    { NULL,     DA_NONE+DA_MODRM, DA_E, grp9 },
    { "imul",   DA_LONG+DA_MODRM, DA2_E_R, 0 }
};

instdesc __attribute__ ((section (".data"))) inst0fb[] = {
    { "cmpxchg",DA_BYTE+DA_MODRM, DA2_R_E, 0 },
    { "cmpxchg",DA_LONG+DA_MODRM, DA2_R_E, 0 },
    { "lss",    DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "btr",    DA_LONG+DA_MODRM, DA2_R_E, 0 },
    { "lfs",    DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "lgs",    DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "movzb",  DA_LONG+DA_MODRM, DA2_Eb_R, 0 },
    { "movzw",  DA_LONG+DA_MODRM, DA2_Ew_R, 0 },

    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { NULL,     DA_LONG+DA_MODRM, DA2_Ib_E, grp8 },
    { "btc",    DA_LONG+DA_MODRM, DA2_R_E, 0 },
    { "bsf",    DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "bsr",    DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "movsb",  DA_LONG+DA_MODRM, DA2_Eb_R, 0 },
    { "movsw",  DA_LONG+DA_MODRM, DA2_Ew_R, 0 }
};

instdesc __attribute__ ((section (".data"))) inst0fc[] = {
    { "xadd",   DA_BYTE+DA_MODRM, DA2_R_E, 0 },
    { "xadd",   DA_LONG+DA_MODRM, DA2_R_E, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { NULL,       DA_NONE+DA_MODRM, DA_E, grp9 },

    { "bswap",  DA_LONG, DA_Ril, 0 },
    { "bswap",  DA_LONG, DA_Ril, 0 },
    { "bswap",  DA_LONG, DA_Ril, 0 },
    { "bswap",  DA_LONG, DA_Ril, 0 },
    { "bswap",  DA_LONG, DA_Ril, 0 },
    { "bswap",  DA_LONG, DA_Ril, 0 },
    { "bswap",  DA_LONG, DA_Ril, 0 },
    { "bswap",  DA_LONG, DA_Ril, 0 }
};

char __attribute__ ((section (".data"))) *esc92[] = {
    "fnop",
    "",
    "",
    "",
    "",
    "",
    "",
    ""
};

char __attribute__ ((section (".data"))) *esc94[] = {
    "fchs",
    "fabs",
    "",
    "",
    "ftst",
    "fxam",
    "",
    ""
};

char __attribute__ ((section (".data"))) *esc95[] = {
    "fld1",
    "fldl2t",
    "fldl2e",
    "fldpi",
    "fldlg2",
    "fldln2",
    "fldz",
    ""
};

char __attribute__ ((section (".data"))) *esc96[] = {
    "f2xm1",
    "fyl2x",
    "fptan",
    "fpatan",
    "fxtract",
    "fprem1",
    "fdecstp",
    "fincstp"
};

char __attribute__ ((section (".data"))) *esc97[] = {
    "fprem",
    "fyl2xp1",
    "fsqrt",
    "fsincos",
    "frndint",
    "fscale",
    "fsin",
    "fcos"
};

char __attribute__ ((section (".data"))) *esca5[] = {
    "",
    "fucompp",
    "",
    "",
    "",
    "",
    "",
    ""
};

char __attribute__ ((section (".data"))) *escb4[] = {
    "fneni",
    "fndisi",
    "fnclex",
    "fninit",
    "fsetpm",
    "",
    "",
    ""
};

char __attribute__ ((section (".data"))) *esce3[] = {
    "",
    "fcompp",
    "",
    "",
    "",
    "",
    "",
    ""
};

char __attribute__ ((section (".data"))) *escf4[] = {
    "fnstsw",
    "",
    "",
    "",
    "",
    "",
    "",
    ""
};

instdesc __attribute__ ((section (".data"))) esc8[] = {
    { "fadd",   DA_SNGL, DA2_STI_ST, 0 },
    { "fmul",   DA_SNGL, DA2_STI_ST, 0 },
    { "fcom",   DA_SNGL, DA2_STI_ST, 0 },
    { "fcomp",  DA_SNGL, DA2_STI_ST, 0 },
    { "fsub",   DA_SNGL, DA2_STI_ST, 0 },
    { "fsubr",  DA_SNGL, DA2_STI_ST, 0 },
    { "fdiv",   DA_SNGL, DA2_STI_ST, 0 },
    { "fdivr",  DA_SNGL, DA2_STI_ST, 0 }
};

instdesc __attribute__ ((section (".data"))) esc9[] = {
    { "fld",    DA_SNGL, DA_STI, 0 },
    { "",       DA_NONE, DA_STI, "fxch" },
    { "fst",    DA_SNGL, DA_X, esc92 },
    { "fstp",   DA_SNGL, DA_X, 0 },
    { "fldenv", DA_NONE, DA_X, esc94 },
    { "fldcw",  DA_NONE, DA_X, esc95 },
    { "fnstenv",DA_NONE, DA_X, esc96 },
    { "fnstcw", DA_NONE, DA_X, esc97 }
};

instdesc __attribute__ ((section (".data"))) esca[] = {
    { "fiadd",  DA_LONG, 0, 0 },
    { "fimul",  DA_LONG, 0, 0 },
    { "ficom",  DA_LONG, 0, 0 },
    { "ficomp", DA_LONG, 0, 0 },
    { "fisub",  DA_LONG, DA_X, 0 },
    { "fisubr", DA_LONG, 0, 0 },
    { "fidiv",  DA_LONG, 0, 0 },
    { "fidivr", DA_LONG, 0, 0 }

};

instdesc __attribute__ ((section (".data"))) escb[] = {
    { "fild",   DA_LONG, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "fist",   DA_LONG, 0, 0 },
    { "fistp",  DA_LONG, 0, 0 },
    { "",       DA_WORD, DA_X, escb4 },
    { "fld",    DA_EXTR, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "fstp",   DA_EXTR, 0, 0 }
};

instdesc __attribute__ ((section (".data"))) escc[] = {
    { "fadd",   DA_DBLR, DA2_STI_ST, 0 },
    { "fmul",   DA_DBLR, DA2_STI_ST, 0 },
    { "fcom",   DA_DBLR, DA2_STI_ST, 0 },
    { "fcomp",  DA_DBLR, DA2_STI_ST, 0 },
    { "fsub",   DA_DBLR, DA2_STI_ST, 0 },
    { "fsubr",  DA_DBLR, DA2_STI_ST, 0 },
    { "fdiv",   DA_DBLR, DA2_STI_ST, 0 },
    { "fdivr",  DA_DBLR, DA2_STI_ST, 0 }
};

instdesc __attribute__ ((section (".data"))) escd[] = {
    { "fld",    DA_DBLR, DA_STI, "ffree" },
    { "",       DA_NONE, 0, 0 },
    { "fst",    DA_DBLR, DA_STI, 0 },
    { "fstp",   DA_DBLR, DA_STI, 0 },
    { "frstor", DA_NONE, DA_STI, "fucom" },
    { "",       DA_NONE, DA_STI, "fucomp" },
    { "fnsave", DA_NONE, 0, 0 },
    { "fnstsw", DA_NONE, 0, 0 }
};

instdesc __attribute__ ((section (".data"))) esce[] = {
    { "fiadd",  DA_WORD, DA2_ST_STI, "faddp" },
    { "fimul",  DA_WORD, DA2_ST_STI, "fmulp" },
    { "ficom",  DA_WORD, 0, 0 },
    { "ficomp", DA_WORD, DA_X, esce3 },
    { "fisub",  DA_WORD, DA2_ST_STI, "fsubrp" },
    { "fisubr", DA_WORD, DA2_ST_STI, "fsubp" },
    { "fidiv",  DA_WORD, DA2_ST_STI, "fdivrp" },
    { "fidivr", DA_WORD, DA2_ST_STI, "fdivp" }
};

instdesc __attribute__ ((section (".data"))) escf[] = {
    { "fild",   DA_WORD, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "fist",   DA_WORD, 0, 0 },
    { "fistp",  DA_WORD, 0, 0 },
    { "fbld",   DA_NONE, DA_XA, escf4 },
    { "fild",   DA_QUAD, 0, 0 },
    { "fbstp",  DA_NONE, 0, 0 },
    { "fistp",  DA_QUAD, 0, 0 }
};

instdesc __attribute__ ((section (".data"))) inst_tbl[] = {
    { "add",    DA_BYTE+DA_MODRM, DA2_R_E, 0 },     /*00*/
    { "add",    DA_LONG+DA_MODRM, DA2_R_E, 0 },
    { "add",    DA_BYTE+DA_MODRM, DA2_E_R, 0 },
    { "add",    DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "add",    DA_BYTE, DA2_Ib_A, 0 },
    { "add",    DA_LONG, DA2_I_A, 0 },
    { "",       DA_NONE, DA_Si, 0 },
    { "",       DA_NONE, DA_Si, 0 },

    { "or",     DA_BYTE+DA_MODRM, DA2_R_E, 0 },     /*08*/
    { "or",     DA_LONG+DA_MODRM, DA2_R_E, 0 },
    { "or",     DA_BYTE+DA_MODRM, DA2_E_R, 0 },
    { "or",     DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "or",     DA_BYTE, DA2_Ib_A, 0 },
    { "or",     DA_LONG, DA2_I_A, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },

    { "adc",    DA_BYTE+DA_MODRM, DA2_R_E, 0 },     /*10*/
    { "adc",    DA_LONG+DA_MODRM, DA2_R_E, 0 },
    { "adc",    DA_BYTE+DA_MODRM, DA2_E_R, 0 },
    { "adc",    DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "adc",    DA_BYTE, DA2_Ib_A, 0 },
    { "adc",    DA_LONG, DA2_I_A, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },

    { "sbb",    DA_BYTE+DA_MODRM, DA2_R_E, 0 },     /*18*/
    { "sbb",    DA_LONG+DA_MODRM, DA2_R_E, 0 },
    { "sbb",    DA_BYTE+DA_MODRM, DA2_E_R, 0 },
    { "sbb",    DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "sbb",    DA_BYTE, DA2_Ib_A, 0 },
    { "sbb",    DA_LONG, DA2_I_A, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },

    { "and",    DA_BYTE+DA_MODRM, DA2_R_E, 0 },     /*20*/
    { "and",    DA_LONG+DA_MODRM, DA2_R_E, 0 },
    { "and",    DA_BYTE+DA_MODRM, DA2_E_R, 0 },
    { "and",    DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "and",    DA_BYTE, DA2_Ib_A, 0 },
    { "and",    DA_LONG, DA2_I_A, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },

    { "sub",    DA_BYTE+DA_MODRM, DA2_R_E, 0 },     /*28*/
    { "sub",    DA_LONG+DA_MODRM, DA2_R_E, 0 },
    { "sub",    DA_BYTE+DA_MODRM, DA2_E_R, 0 },
    { "sub",    DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "sub",    DA_BYTE, DA2_Ib_A, 0 },
    { "sub",    DA_LONG, DA2_I_A, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },

    { "xor",    DA_BYTE+DA_MODRM, DA2_R_E, 0 },     /*30*/
    { "xor",    DA_LONG+DA_MODRM, DA2_R_E, 0 },
    { "xor",    DA_BYTE+DA_MODRM, DA2_E_R, 0 },
    { "xor",    DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "xor",    DA_BYTE, DA2_Ib_A, 0 },
    { "xor",    DA_LONG, DA2_I_A, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },

    { "cmp",    DA_BYTE+DA_MODRM, DA2_R_E, 0 },     /*38*/
    { "cmp",    DA_BYTE+DA_MODRM, DA2_R_E, 0 },
    { "cmp",    DA_BYTE+DA_MODRM, DA2_E_R, 0 },
    { "cmp",    DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "cmp",    DA_BYTE, DA2_Ib_A, 0 },
    { "cmp",    DA_LONG, DA2_I_A, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },

    { "",       DA_LONG, DA_Ri, 0 },                /*40*/
    { "",       DA_LONG, DA_Ri, 0 },
    { "",       DA_LONG, DA_Ri, 0 },
    { "",       DA_LONG, DA_Ri, 0 },
    { "",       DA_LONG, DA_Ri, 0 },
    { "",       DA_LONG, DA_Ri, 0 },
    { "",       DA_LONG, DA_Ri, 0 },
    { "",       DA_LONG, DA_Ri, 0 },

    { "",       DA_LONG, DA_Ri, 0 },                /*48*/
    { "",       DA_LONG, DA_Ri, 0 },
    { "",       DA_LONG, DA_Ri, 0 },
    { "",       DA_LONG, DA_Ri, 0 },
    { "",       DA_LONG, DA_Ri, 0 },
    { "",       DA_LONG, DA_Ri, 0 },
    { "",       DA_LONG, DA_Ri, 0 },
    { "",       DA_LONG, DA_Ri, 0 },

    { "push",   DA_QUAD, DA_Ri, 0 },                /*50*/
    { "push",   DA_QUAD, DA_Ri, 0 },
    { "push",   DA_QUAD, DA_Ri, 0 },
    { "push",   DA_QUAD, DA_Ri, 0 },
    { "push",   DA_QUAD, DA_Ri, 0 },
    { "push",   DA_QUAD, DA_Ri, 0 },
    { "push",   DA_QUAD, DA_Ri, 0 },
    { "push",   DA_QUAD, DA_Ri, 0 },

    { "pop",    DA_QUAD, DA_Ri, 0 },                /*58*/
    { "pop",    DA_QUAD, DA_Ri, 0 },
    { "pop",    DA_QUAD, DA_Ri, 0 },
    { "pop",    DA_QUAD, DA_Ri, 0 },
    { "pop",    DA_QUAD, DA_Ri, 0 },
    { "pop",    DA_QUAD, DA_Ri, 0 },
    { "pop",    DA_QUAD, DA_Ri, 0 },
    { "pop",    DA_QUAD, DA_Ri, 0 },

    { "",       DA_LONG, 0, 0 },                    /*60*/
    { "",       DA_LONG, 0, 0 },
    { "",       DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "movsxd", DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },

    { "push",   DA_LONG, DA_I, 0 },                 /*68*/
    { "imul",   DA_LONG+DA_MODRM, DA3_I_E_R, 0 },
    { "push",   DA_QUAD, DA_Iq, 0 },
    { "imul",   DA_LONG+DA_MODRM, DA3_Ib_E_R, 0 },
    { "ins",    DA_BYTE, DA2_DX_DI, 0 },
    { "ins",    DA_LONG, DA2_DX_DI, 0 },
    { "outs",   DA_BYTE, DA2_SI_DX, 0 },
    { "outs",   DA_LONG, DA2_SI_DX, 0 },

    { "jo",     DA_NONE, DA_Db, 0 },                /*70*/
    { "jno",    DA_NONE, DA_Db, 0 },
    { "jb",     DA_NONE, DA_Db, 0 },
    { "jnb",    DA_NONE, DA_Db, 0 },
    { "jz",     DA_NONE, DA_Db, 0 },
    { "jnz",    DA_NONE, DA_Db, 0 },
    { "jbe",    DA_NONE, DA_Db, 0 },
    { "jnbe",   DA_NONE, DA_Db, 0 },

    { "js",     DA_NONE, DA_Db, 0 },                /*78*/
    { "jns",    DA_NONE, DA_Db, 0 },
    { "jp",     DA_NONE, DA_Db, 0 },
    { "jnp",    DA_NONE, DA_Db, 0 },
    { "jl",     DA_NONE, DA_Db, 0 },
    { "jnl",    DA_NONE, DA_Db, 0 },
    { "jle",    DA_NONE, DA_Db, 0 },
    { "jnle",   DA_NONE, DA_Db, 0 },

    { NULL,     DA_BYTE+DA_MODRM, DA2_Ib_E, grp1 },  /*80*/
    { NULL,     DA_LONG+DA_MODRM, DA2_I_E, grp1 },
    { NULL,     DA_BYTE+DA_MODRM, DA2_Ib_E, grp1 },
    { NULL,     DA_LONG+DA_MODRM, DA2_I_E, grp1 },
    { "test",   DA_BYTE+DA_MODRM, DA2_R_E, 0 },
    { "test",   DA_LONG+DA_MODRM, DA2_R_E, 0 },
    { "xchg",   DA_BYTE+DA_MODRM, DA2_R_E, 0 },
    { "xchg",   DA_LONG+DA_MODRM, DA2_R_E, 0 },

    { "mov",    DA_BYTE+DA_MODRM, DA2_R_E, 0 },     /*88*/
    { "mov",    DA_LONG+DA_MODRM, DA2_R_E, 0 },
    { "mov",    DA_BYTE+DA_MODRM, DA2_E_R, 0 },
    { "mov",    DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "mov",    DA_NONE+DA_MODRM, DA2_S_Ew, 0 },
    { "lea",    DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "mov",    DA_NONE+DA_MODRM, DA2_Ew_S, 0 },
    { "pop",    DA_QUAD+DA_MODRM, DA_E, 0 },

    { "nop",    DA_NONE, 0, 0 },                    /*90*/
    { "xchg",   DA_LONG, DA2_A_Ri, 0 },
    { "xchg",   DA_LONG, DA2_A_Ri, 0 },
    { "xchg",   DA_LONG, DA2_A_Ri, 0 },
    { "xchg",   DA_LONG, DA2_A_Ri, 0 },
    { "xchg",   DA_LONG, DA2_A_Ri, 0 },
    { "xchg",   DA_LONG, DA2_A_Ri, 0 },
    { "xchg",   DA_LONG, DA2_A_Ri, 0 },

    { "cbw",    DA_SDEP, 0, "cdqe" },               /*98*/
    { "cwd",    DA_SDEP, 0, "cqo" },
    { "",       DA_NONE, DA_OS, 0 },
    { "wait",   DA_NONE, 0, 0 },
    { "pushf",  DA_QUAD, 0, 0 },
    { "popf",   DA_QUAD, 0, 0 },
    { "sahf",   DA_NONE, 0, 0 },
    { "lahf",   DA_NONE, 0, 0 },

    { "mov",    DA_BYTE, DA2_O_A, 0 },              /*A0*/
    { "mov",    DA_LONG, DA2_O_A, 0 },
    { "mov",    DA_BYTE, DA2_A_O, 0 },
    { "mov",    DA_LONG, DA2_A_O, 0 },
    { "movsb",  DA_BYTE, 0, 0 },
    { "movsd",  DA_LONG, 0, 0 },
    { "cmpsb",  DA_BYTE, 0, 0 },
    { "cmpsd",  DA_LONG, 0, 0 },

    { "test",   DA_BYTE, DA2_I_A, 0 },              /*A8*/
    { "test",   DA_LONG, DA2_I_A, 0 },
    { "stosb",  DA_BYTE, 0, 0 },
    { "stosw",  DA_WORD, 0, 0 },
    { "lodsb",  DA_BYTE, 0, 0 },
    { "lodsw",  DA_WORD, 0, 0 },
    { "scasb",  DA_BYTE, 0, 0 },
    { "scasd",  DA_LONG, 0, 0 },

    { "mov",    DA_BYTE, DA2_Ib_Ri, 0 },            /*B0*/
    { "mov",    DA_BYTE, DA2_Ib_Ri, 0 },
    { "mov",    DA_BYTE, DA2_Ib_Ri, 0 },
    { "mov",    DA_BYTE, DA2_Ib_Ri, 0 },
    { "mov",    DA_BYTE, DA2_Ib_Ri, 0 },
    { "mov",    DA_BYTE, DA2_Ib_Ri, 0 },
    { "mov",    DA_BYTE, DA2_Ib_Ri, 0 },
    { "mov",    DA_BYTE, DA2_Ib_Ri, 0 },

    { "mov",    DA_LONG, DA2_I_Ri, 0 },             /*B8*/
    { "mov",    DA_LONG, DA2_I_Ri, 0 },
    { "mov",    DA_LONG, DA2_I_Ri, 0 },
    { "mov",    DA_LONG, DA2_I_Ri, 0 },
    { "mov",    DA_LONG, DA2_I_Ri, 0 },
    { "mov",    DA_LONG, DA2_I_Ri, 0 },
    { "mov",    DA_LONG, DA2_I_Ri, 0 },
    { "mov",    DA_LONG, DA2_I_Ri, 0 },

    { NULL,     DA_BYTE+DA_MODRM, DA2_Ib_E, grp2 }, /*C0*/
    { NULL,     DA_LONG+DA_MODRM, DA2_Ib_E, grp2 },
    { "ret",    DA_NONE, DA_Iw, 0 },
    { "ret",    DA_NONE, 0, 0 },
    { "",       DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "",       DA_LONG+DA_MODRM, DA2_E_R, 0 },
    { "mov",    DA_BYTE+DA_MODRM, DA2_I_E, 0 },
    { "mov",    DA_LONG+DA_MODRM, DA2_I_E, 0 },

    { "enter",  DA_NONE, DA2_Iq_Ib, 0 },            /*C8*/
    { "leave",  DA_NONE, 0, 0 },
    { "lret",   DA_NONE, DA_Iw, 0 },
    { "lret",   DA_NONE, 0, 0 },
    { "int",    DA_NONE, DA_o3, 0 },
    { "int",    DA_NONE, DA_Ib, 0 },
    { "",       DA_NONE, 0, 0 },
    { "iret",   DA_NONE, 0, 0 },

    { NULL,     DA_BYTE+DA_MODRM, DA2_o1_E, grp2 }, /*D0*/
    { NULL,     DA_LONG+DA_MODRM, DA2_o1_E, grp2 },
    { NULL,     DA_BYTE+DA_MODRM, DA2_CL_E, grp2 },
    { NULL,     DA_LONG+DA_MODRM, DA2_CL_E, grp2 },
    { "",       DA_NONE+DA_MODRM, DA_Ib, 0 },
    { "",       DA_NONE+DA_MODRM, DA_Ib, 0 },
    { "",       DA_NONE, 0, 0 },
    { "xlat",   DA_BYTE, DA_BX, 0 },

    { "",       DA_NONE+DA_MODRM, 0, esc8 },        /*D8*/
    { "",       DA_NONE+DA_MODRM, 0, esc9 },
    { "",       DA_NONE+DA_MODRM, 0, esca },
    { "",       DA_NONE+DA_MODRM, 0, escb },
    { "",       DA_NONE+DA_MODRM, 0, escc },
    { "",       DA_NONE+DA_MODRM, 0, escd },
    { "",       DA_NONE+DA_MODRM, 0, esce },
    { "",       DA_NONE+DA_MODRM, 0, escf },

    { "loopne", DA_NONE, DA_Db, 0 },                /*E0*/
    { "loope",  DA_NONE, DA_Db, 0 },
    { "loop",   DA_NONE, DA_Db, 0 },
    { "jcxz",   DA_SDEP, DA_Db, "jrcxz" },
    { "in",     DA_BYTE, DA2_Ib_A, 0 },
    { "in",     DA_LONG, DA2_Ib_A, 0 },
    { "out",    DA_BYTE, DA2_A_Ib, 0 },
    { "out",    DA_LONG, DA2_A_Ib, 0 },

    { "call",   DA_LONG, DA_Dl, 0 },                /*E8*/
    { "jmp",    DA_LONG, DA_Dl, 0 },
    { "",       DA_NONE, DA_OS, 0 },
    { "jmp",    DA_NONE, DA_Db, 0 },
    { "in",     DA_BYTE, DA2_DX_A, 0 },
    { "in",     DA_LONG, DA2_DX_A, 0 },
    { "out",    DA_BYTE, DA2_A_DX, 0 },
    { "out",    DA_LONG, DA2_A_DX, 0 },

    { "",       DA_NONE, 0, 0 },                    /*F0*/
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "",       DA_NONE, 0, 0 },
    { "hlt",    DA_NONE, 0, 0 },
    { "cmc",    DA_NONE, 0, 0 },
    { NULL,     DA_BYTE+DA_MODRM, 0, grp3 },
    { NULL,     DA_LONG+DA_MODRM, 0, grp3 },

    { "clc",    DA_NONE, 0, 0 },                    /*F8*/
    { "stc",    DA_NONE, 0, 0 },
    { "cli",    DA_NONE, 0, 0 },
    { "sti",    DA_NONE, 0, 0 },
    { "cld",    DA_NONE, 0, 0 },
    { "std",    DA_NONE, 0, 0 },
    { NULL,     DA_NONE+DA_MODRM, 0, grp4 },
    { NULL,     DA_NONE+DA_MODRM, 0, grp5 }
};

instdesc __attribute__ ((section (".data"))) *inst_tbl0f[] = {
    inst0f0,
    0,
    inst0f2,
    inst0f3,
    0,
    0,
    0,
    inst0f7,
    inst0f8,
    inst0f9,
    inst0fa,
    inst0fb,
    inst0fc,
    0,
    0,
    0
};

char __attribute__ ((section (".data"))) *sizestr[7] = {
    "byte",
    "word",
    "dword",
    "qword",
    "single",
    "double",
    "extend"
};

char __attribute__ ((section (".data"))) *regs[4][16] = {
    { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"},
    { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di", "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"},
    { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"},
    { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15" }
};

char __attribute__ ((section (".data"))) *sregs[8] = {
    "es", "cs", "ss", "ds", "fs", "gs", "?", "?"
};
char __attribute__ ((section (".data"))) *unkmnemonic = "???";

// address resolve
uint8_t __attribute__ ((section (".data"))) addr_reg;
uint8_t __attribute__ ((section (".data"))) addr_scale;
uint64_t __attribute__ ((section (".data"))) addr_disp;
uint8_t __attribute__ ((section (".data"))) addr_seg;
uint8_t __attribute__ ((section (".data"))) addr_saddr;
char __attribute__ ((section (".data"))) *addr_base;
char __attribute__ ((section (".data"))) *addr_idx;

char *disasm_prtaddr(char *str, virt_t addr, uint8_t mode, uint8_t size, uint8_t reg)
{
    if(addr_reg) {
        str = sprintf(str, regs[size][reg]);
    } else {
        str = sprintf(str,"%s ", sizestr[size]);
        if (addr_seg)
            str = sprintf(str, "%cs:", addr_seg);
        str = sprintf(str,"[");
        if(addr_base!=NULL) {
            str = sprintf(str,"%s", addr_base);
            if(addr_idx!=NULL) {
                str = sprintf(str,"+%s", addr_idx);
                if(addr_scale>1)
                    str = sprintf(str,"*%d", addr_scale);
            }
        } else {
            if(mode!=DA_E)
                addr_disp += addr;
        }
        if(addr_disp) {
            str = sprintf(str,"%s%x", addr_base!=NULL?"+":"", addr_disp);
            if(addr_base==NULL)
                dbg_comment = addr_disp;
        }
        str = sprintf(str,"]");
    }
    return str;
}

virt_t disasm(virt_t addr, char *str)
{
    virt_t disasmstart;
    //prefix
    uint8_t rex = 0;
    //opcode and it's argument bytes
    uint8_t opcode = 0, opcode2 = 0;
    uint8_t modrm = 0;
    uint8_t sib = 0;
    uint64_t imm = 0;
    uint64_t disp = 0;
    //parsed values
    uint8_t size = DA_LONG;
    uint8_t reg = 0;
    instdesc *inst;
    uint32_t i_size = DA_NONE;
    uint32_t i_mode;
    char *i_name;

    disasmstart = addr;
    addr_seg = 0;
    addr_saddr = DA_QUAD;
    addr_base = NULL;
    addr_idx = NULL;
    dbg_comment = 0;

    sys_fault = false;
getprefix:
    __asm__ __volatile__ ("pushq %%rax;pushq %%rdi":::"%rax","%rdi");
    opcode = *((uint8_t*)addr);
    __asm__ __volatile__ ("popq %%rdi;popq %%rax":::"%rax","%rdi");
    if(sys_fault)
        goto end;
    addr++;
    //segment ovverride for CS, DS, ES, SS are ignored in long mode
    if(opcode==0x26||opcode==0x36||opcode==0x2e||opcode==0x3e)
        goto getprefix;
    //fs
    if(opcode==0x64) {
        addr_seg='f';
        goto getprefix;
    }
    //gs
    if(opcode==0x65) {
        addr_seg='g';
        goto getprefix;
    }
    //size override prefix
    if(opcode==0x66) {
        rex=0x40;
        size=DA_WORD;
        goto getprefix;
    }
    //address size override
    if(opcode==0x67) {
        addr_saddr=DA_LONG;
        goto getprefix;
    }
    if(opcode==0xf0) {
        if(str!=NULL)
            str = sprintf(str, "lock ");
        goto getprefix;
    }
    if(opcode==0xf2) {
        if(str!=NULL)
            str = sprintf(str, "repnz ");
        goto getprefix;
    }
    if(opcode==0xf3) {
        if(str!=NULL)
            str = sprintf(str, "repe ");
        goto getprefix;
    }
    //rex
    if((opcode&0xF0)==0x40) {
        rex = opcode;
        if(rex&0b1000)
            size=DA_QUAD;
        goto getopcode;
    }
    goto prefixend;

getopcode:
    opcode = *((uint8_t*)addr);
    if(sys_fault)
        goto end;
    addr++;
prefixend:
    //nop
    if(opcode==0x90) {
        i_size = 1;
        while(*((uint8_t*)addr)==0x90) { i_size++; addr++; }
        if(str != NULL)
            str = sprintf(str, "%d x nop", i_size);
        goto end;
    }
    //2 bytes opcode?
    if (opcode == 0x0f) {
        opcode2 = *((uint8_t*)addr);
        addr++;
        //handle syscall specially (mov eax,...;syscall)
        if (opcode2 == 0x05) {
            if(str != NULL)
                str = sprintf(str, "syscall '%c%c%c%c'",
                    *((uchar*)addr-6),*((uchar*)addr-5),*((uchar*)addr-4),*((uchar*)addr-3));
            goto end;
        }
        inst = inst_tbl0f[opcode2>>4];
        if (inst != NULL)
            inst = &inst[opcode2&0xf];
    } else {
        //no, 1 byte opcode
        inst = &inst_tbl[opcode];
    }
    if (inst == NULL) {
        if(str != NULL)
            str = sprintf(str, unkmnemonic);
        goto end;
    }
    //has modrm?
    if(inst->size & DA_MODRM) {
        modrm = *((uint8_t*)addr);
        addr++;
        uint8_t mod = (modrm>>6) & 3;
        uint8_t rm = ((rex & 0b1)<<3) + (modrm & 7);
        uint16_t idx;
        reg = ((rex & 0b100)<<1) + ((modrm>>3) & 7);
        addr_disp = 0;
        addr_base = addr_idx = NULL;
        addr_scale = addr_reg = false;
        if(mod==3) {
            addr_reg = true;
            addr_disp = (uint64_t)((uint8_t)rm);
        } else {
            // do we have a sib byte?
            if((modrm & 7)==0b100) {
                sib = *((uint8_t*)addr);
                addr++;
                addr_scale=sib>>6;
                idx=((rex&2)<<2)+((sib>>3)&7);
                if(idx!=4)
                    addr_idx=regs[DA_QUAD][idx];
                rm = ((rex&1)<<3)+(sib&7);
                addr_disp = (uint64_t)(*((uint32_t*)addr));
                addr+=4;
            } else {
                addr_base = regs[size][rm];
//                if(mod==0) {
//                    addr_disp = (uint64_t)(*((uint32_t*)addr));
//                    addr+=4;
//                    addr_base = 0;
//                } else
                if(mod==1) {
                    addr_disp = (uint64_t)(*((uint8_t*)addr));
                    addr++;
                } else
                if(mod==2) {
                    addr_disp = (uint64_t)(*((uint32_t*)addr));
                    addr+=4;
                }
            }
        }
    }
    //ModRM specific
    i_name = inst->name;
    if((inst->name==NULL||inst->name[0]==0) && inst->ext != 0) {
        if(opcode>=0xd0&&opcode<=0xd8){
            //TODO: esc
            inst = (instdesc*)inst->ext;
            inst = &inst[(modrm>>3)&7];
            i_name = inst->name;
        } else {
            // typeof inst->ext == instdesc*?
            if(inst->ext==grp3 || inst->ext==grp4 || inst->ext==grp5 ||
               inst->ext==grp7) {
                inst = (instdesc*)inst->ext;
                inst = &inst[(modrm>>3)&7];
            }
            if (opcode==0x0f && (opcode2 == 0x01 || opcode2 == 0xae)) {

            }
        }
    }
    i_name = inst->name!=NULL ? inst->name : ((char**)inst->ext)[reg];
    i_size = inst->size&0xf;
    i_mode = inst->mode;

    //mnemonic
    if (i_size == DA_SDEP) {
        if (size == DA_LONG)
            i_name=(char *)inst->ext;
        i_size = size;
    }
    if(rex)
        i_size = size;
    if(str!=NULL)
        str = sprintf(str, "%s\t", i_name);

    //arguments
    for (;i_mode != 0; i_mode >>= 8) {
        if (str!=NULL && *(str-1)!='\t')
            str = sprintf(str, ", ");

        switch (i_mode & 0xFF) {
            case DA_E:
            case DA_EInd:
                if(str != NULL)
                    str = disasm_prtaddr(str, addr, i_mode&0xFF, i_size, addr_disp);
                break;
            case DA_Ew:
                if(str != NULL)
                    str = disasm_prtaddr(str, addr, i_mode&0xFF, DA_WORD, reg);
                break;
            case DA_Eb:
                if(str != NULL)
                    str = disasm_prtaddr(str, addr, i_mode&0xFF, DA_BYTE, reg);
                break;
            case DA_R:
                if(str != NULL)
                    str = sprintf(str, regs[size][reg]);
                break;
            case DA_Rw:
                if(str != NULL)
                    str = sprintf(str, regs[DA_WORD][reg]);
                break;
            case DA_Ri:
                if(str != NULL)
                    str = sprintf(str, regs[size][(opcode&7)+((rex&1)<<3)]);
                break;
            case DA_Ril:
                if(str != NULL)
                    str = sprintf(str, regs[DA_LONG][(opcode&7)+((rex&1)<<3)]);
                break;
            case DA_S:
                if(str != NULL)
                    str = sprintf(str, sregs[(modrm&7)]);
                break;
            case DA_Si:
                if(str != NULL)
                    str = sprintf(str, sregs[(opcode&7)]);
                break;
            case DA_A:
                if(str != NULL)
                    str = sprintf(str, regs[size][0]);
                break;
            case DA_BX:
                if(str != NULL) {
                    if (addr_seg)
                        str = sprintf(str, "%cs:", addr_seg);
                    str = sprintf(str,"%s [%s]", sizestr[i_size], regs[addr_saddr][3]);
                }
                break;
            case DA_CL:
                if(str != NULL)
                    str = sprintf(str, regs[DA_BYTE][1]);
                break;
            case DA_DX:
                if(str != NULL)
                    str = sprintf(str, regs[DA_WORD][2]);
                break;
            case DA_SI:
                if(str != NULL) {
                    if (addr_seg)
                        str = sprintf(str, "%cs:", addr_seg);
                    str = sprintf(str,"%s [%s]", sizestr[i_size], regs[addr_saddr][6]);
                }
                break;
            case DA_DI:
                if(str != NULL) {
                    if (addr_seg)
                        str = sprintf(str, "%cs:", addr_seg);
                    str = sprintf(str,"%s [%s]", sizestr[i_size], regs[addr_saddr][7]);
                }
                break;
            case DA_CR:
                if (str != NULL)
                    str = sprintf(str, "cr%d", reg);
                break;
            case DA_DR:
                if (str != NULL)
                    str = sprintf(str, "dr%d", reg);
                break;
            case DA_TR:
                if (str != NULL)
                    str = sprintf(str, "tr%d", reg);
                break;
            case DA_I:
            case DA_Is:
                // get immediate
                imm = (uint64_t)(*((uint32_t*)addr));
                addr+=4;
                if (str != NULL)
                    str = sprintf(str, "%xh", imm);
                if(!dbg_comment)
                    dbg_comment=imm;
                break;
            case DA_Ib:
                // get immediate
                imm = (uint64_t)(*((uint8_t*)addr));
                addr++;
                if (str != NULL)
                    str = sprintf(str, "%1xh", imm);
                if(!dbg_comment)
                    dbg_comment=imm;
                break;
            case DA_Iba:
            case DA_Ibs:
                // get immediate
                imm = (uint64_t)(*((uint8_t*)addr));
                addr++;
                if (str != NULL && imm !=0x0a)
                    str = sprintf(str, "%1xh", imm);
                if(!dbg_comment)
                    dbg_comment=imm;
                break;
            case DA_Iw:
                // get immediate
                imm = (uint64_t)(*((uint16_t*)addr));
                addr+=2;
                if (str != NULL)
                    str = sprintf(str, "%2xh", imm);
                if(!dbg_comment)
                    dbg_comment=imm;
                break;
            case DA_Iq:
                // get immediate
                imm = (uint64_t)(*((uint64_t*)addr));
                addr+=8;
                if (str != NULL)
                    str = sprintf(str, "%8xh", imm);
                if(!dbg_comment)
                    dbg_comment=imm;
                break;
            case DA_O:
                // get displacement
                if (addr_saddr==DA_QUAD) {
                    disp = (uint64_t)(*((uint32_t*)addr));
                    addr+=4;
                } else {
                    disp = (uint64_t)(*((uint16_t*)addr));
                    addr+=2;
                }
                if(str != NULL) {
                    if (addr_seg)
                        str = sprintf(str, "%cs:", addr_seg);
                    str = sprintf(str,"%xh", disp);
                }
                if(!dbg_comment)
                    dbg_comment = disp;
                break;
            case DA_Db:
                // get displacement
                disp = (uint64_t)(*((uint8_t*)addr));
                addr++;
                disp += addr;
                if(str != NULL)
                    str = sprintf(str, "%xh", disp);
                if(!dbg_comment)
                    dbg_comment=disp;
                break;
            case DA_Dl:
                // get displacement
                if (i_size==DA_QUAD) {
                    disp = (uint64_t)(*((uint64_t*)addr));
                    addr+=8;
                } else if (i_size==DA_LONG) {
                    disp = (uint64_t)(*((uint32_t*)addr));
                    addr+=4;
                } else if (i_size==DA_WORD) {
                    disp = (uint64_t)(*((uint16_t*)addr));
                    addr+=2;
                } else if (i_size==DA_BYTE) {
                    disp = (uint64_t)(*((uint8_t*)addr));
                    addr++;
                }
                disp += addr;
                if(str != NULL)
                    str = sprintf(str, "%xh", disp);
                if(!dbg_comment)
                    dbg_comment=disp;
                break;
            case DA_o1:
                if(str != NULL)
                    str = sprintf(str, "1");
                break;
            case DA_o3:
                if(str != NULL)
                    str = sprintf(str, "3");
                break;
            case DA_OS:
                // get displacement
                if (size==DA_QUAD) {
                    disp = (uint64_t)(*((uint64_t*)addr));
                    addr+=8;
                } else if (size==DA_LONG) {
                    disp = (uint64_t)(*((uint32_t*)addr));
                    addr+=4;
                } else if (size==DA_WORD) {
                    disp = (uint64_t)(*((uint16_t*)addr));
                    addr+=2;
                } else {
                    disp = (uint64_t)(*((uint8_t*)addr));
                    addr++;
                }
                // get immediate
                imm = (uint64_t)(*((uint16_t*)addr));
                addr+=2;
                if(str != NULL)
                    str = sprintf(str, "%x:%xh", imm, disp);
                break;
            default:
                if(str != NULL)
                    str = sprintf(str, "?");
                break;
        }
    }

end:
    if (addr - disasmstart > 15 && str!=NULL)
        str = sprintf(str," <too long>");
    *str=0;
    return addr;
}

#endif
