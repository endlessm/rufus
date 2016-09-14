/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2006,2007,2008,2009,2010,2011  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <Windows.h>
#include <inttypes.h>


typedef uint8_t grub_uint8_t;
typedef uint16_t grub_uint16_t;
typedef uint32_t grub_uint32_t;
typedef uint64_t grub_uint64_t;

typedef size_t grub_size_t;
typedef SSIZE_T grub_ssize_t;
typedef int grub_err_t;

/* The type for representing a disk block address.  */
typedef grub_uint64_t grub_disk_addr_t;
/* The type for representing a file offset.  */
typedef grub_uint64_t	grub_off_t;

typedef enum
{
	GRUB_EMBED_PCBIOS
} grub_embed_type_t;

static inline grub_uint16_t grub_get_unaligned16(const void *ptr)
{
#pragma pack(push,1)
	struct grub_unaligned_uint16_t
	{
		grub_uint16_t d;
	};
#pragma pack(pop)
	const struct grub_unaligned_uint16_t *dd = (const struct grub_unaligned_uint16_t *) ptr;
	return dd->d;
}

static inline void grub_set_unaligned32(void *ptr, grub_uint32_t val)
{
#pragma pack(push,1)
	struct grub_unaligned_uint32_t
	{
		grub_uint32_t d;
	};
#pragma pack(pop)
	struct grub_unaligned_uint32_t *dd = (struct grub_unaligned_uint32_t *) ptr;
	dd->d = val;
}

int grub_util_bios_setup(const wchar_t *core_path, HANDLE dest_dev, grub_disk_addr_t last_available_sector);
