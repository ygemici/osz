/*
 * fs/fileop.c
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
 * @brief File operations
 */
#include <osZ.h>

public size_t read(fid_t fid, void *buf, size_t size) { return 0; }
public fid_t dup2(fid_t oldfd, fid_t newfd) { return 0; }
public size_t write(void *buf, size_t size, fid_t fid) { return 0; }
public fpos_t seek(fid_t fid, fpos_t offset, int whence) { return 0; }
public fid_t dup(fid_t oldfd) { return 0; }
public int stat(fid_t fd, stat_t *buf) { return 0; }

public void pipe(){}
public void ioctl(){}
