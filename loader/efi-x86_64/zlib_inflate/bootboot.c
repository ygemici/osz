/*
 * zlib_inflate/bootboot.c
 * 
 * Copyright 2016 BOOTBOOT bztsrc@github
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * This file is part of the BOOTBOOT Protocol package.
 * @brief zlib interface for initrd decompression
 * 
 */

#include <zutil.h>
#include <efi.h>
#include <efilib.h>

/* Utility function: initialize zlib, unpack binary blob, clean up zlib,
 * return len or negative error code.
 */
EFI_STATUS zlib_inflate_blob(void *gunzip_buf, unsigned int sz,
		      const void *buf, unsigned int len)
{
	// TODO: debug why this isn't working. No errors, no output
	const unsigned char *zbuf = buf;
	struct z_stream_s *strm;
	EFI_STATUS rc;
	rc = EFI_OUT_OF_RESOURCES;
	strm = AllocatePool(sizeof(*strm));
	if (strm == NULL)
		goto gunzip_nomem1;
	strm->workspace = AllocateZeroPool(zlib_inflate_workspacesize());
	if (strm->workspace == NULL)
		goto gunzip_nomem2;

	/* gzip header (1f,8b,08... 10 bytes total + possible asciz filename)
	 * expected to be stripped from input
	 */
	strm->next_in = zbuf;
	strm->avail_in = len;
	strm->next_out = gunzip_buf;
	strm->avail_out = sz;

	rc = zlib_inflateInit(strm);
	if (rc == Z_OK) {
		rc = zlib_inflate(strm, Z_FINISH);
		/* after Z_FINISH, only Z_STREAM_END is "we unpacked it all" */
		if (rc == Z_STREAM_END)
			rc = sz - strm->avail_out;
		else
			rc = EFI_INVALID_PARAMETER;
		zlib_inflateEnd(strm);
		rc = EFI_SUCCESS;
	} else
		rc = EFI_INVALID_PARAMETER;

	FreePool(strm->workspace);
gunzip_nomem2:
	FreePool(strm);
gunzip_nomem1:
	return rc;
}
