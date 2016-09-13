#pragma once

#include <Windows.h>


typedef unsigned short grub_uint16_t;
typedef unsigned int grub_uint32_t;
typedef unsigned long long grub_uint64_t;

typedef grub_uint64_t grub_size_t;
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
