/* code obtained from GRUB, main files:
 *  - util/setup.c - make GRUB usable
 *  - grub-core/partmap/msdos.c - Read PC style partition tables
 *
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

#include <stdlib.h>
#include <stdio.h>

#include "grub.h"
#include "reed_solomon.h"
#include "GeneralCode.h"
#include "rufus.h"
#include "file.h"

#define GRUB_SETUP_BIOS 1

#define GRUB_DISK_SECTOR_SIZE	0x200
#define GRUB_DISK_SECTOR_BITS	9

/* The segment where the kernel is loaded.  */
#define GRUB_BOOT_I386_PC_KERNEL_SEG	0x800

#define GRUB_KERNEL_I386_PC_LINK_ADDR  0x9000

/* Offset of field holding no reed solomon length.  */
#define GRUB_KERNEL_I386_PC_NO_REED_SOLOMON_LENGTH      0x14

/* Offset of reed_solomon_redundancy.  */
#define GRUB_KERNEL_I386_PC_REED_SOLOMON_REDUNDANCY	0x10

# define grub_cpu_to_le16(x)	((grub_uint16_t) (x))
# define grub_cpu_to_le32(x)	((grub_uint32_t) (x))
# define grub_cpu_to_le64(x)	((grub_uint64_t) (x))
# define grub_le_to_cpu16(x)	((grub_uint16_t) (x))
# define grub_le_to_cpu32(x)	((grub_uint32_t) (x))
# define grub_le_to_cpu64(x)	((grub_uint64_t) (x))
# define grub_cpu_to_be16(x)	grub_swap_bytes16(x)
# define grub_cpu_to_be32(x)	grub_swap_bytes32(x)
# define grub_cpu_to_be64(x)	grub_swap_bytes64(x)
# define grub_be_to_cpu16(x)	grub_swap_bytes16(x)
# define grub_be_to_cpu32(x)	grub_swap_bytes32(x)
# define grub_be_to_cpu64(x)	grub_swap_bytes64(x)
# define grub_cpu_to_be16_compile_time(x)	grub_swap_bytes16_compile_time(x)
# define grub_cpu_to_be32_compile_time(x)	grub_swap_bytes32_compile_time(x)
# define grub_cpu_to_be64_compile_time(x)	grub_swap_bytes64_compile_time(x)
# define grub_be_to_cpu64_compile_time(x)	grub_swap_bytes64_compile_time(x)
# define grub_cpu_to_le16_compile_time(x)	((grub_uint16_t) (x))
# define grub_cpu_to_le32_compile_time(x)	((grub_uint32_t) (x))
# define grub_cpu_to_le64_compile_time(x)	((grub_uint64_t) (x))

#define grub_target_to_host16(x)	grub_le_to_cpu16(x)
#define grub_target_to_host32(x)	grub_le_to_cpu32(x)
#define grub_target_to_host64(x)	grub_le_to_cpu64(x)
#define grub_host_to_target16(x)	grub_cpu_to_le16(x)
#define grub_host_to_target32(x)	grub_cpu_to_le32(x)
#define grub_host_to_target64(x)	grub_cpu_to_le64(x)

struct embed_signature
{
	const char *name;
	const char *signature;
	int signature_len;
	enum { TYPE_SOFTWARE, TYPE_RAID } type;
};

const char message_warn[][200] = {
	/* TRANSLATORS: MBR gap and boot track is the same thing and is the space
	between MBR and first partitition. If your language translates well only
	"boot track", you can just use it everywhere. Next two messages are about
	RAID controllers/software bugs which GRUB has to live with. Please spread
	the message that these are bugs in other software and not merely
	suboptimal behaviour.  */
	[TYPE_RAID] = "Sector %llu is already in use by raid controller `%s';"
	" avoiding it.  "
		"Please ask the manufacturer not to store data in MBR gap",
	[TYPE_SOFTWARE] = "Sector %llu is already in use by the program `%s';"
		" avoiding it.  "
		"This software may cause boot or other problems in "
		"future.  Please ask its authors not to store data "
		"in the boot track"
};

/* Signatures of other software that may be using sectors in the embedding
area.  */
struct embed_signature embed_signatures[] =
{
	{
		.name = "ZISD",
		.signature = "ZISD",
		.signature_len = 4,
		.type = TYPE_SOFTWARE
	},
	{
		.name = "FlexNet",
		.signature = "\xd4\x41\xa0\xf5\x03\x00\x03\x00",
		.signature_len = 8,
		.type = TYPE_SOFTWARE
	},
	{
		.name = "FlexNet",
		.signature = "\xd8\x41\xa0\xf5\x02\x00\x02\x00",
		.signature_len = 8,
		.type = TYPE_SOFTWARE
	},
	{
		/* from Ryan Perkins */
		.name = "HP Backup and Recovery Manager (?)",
		.signature = "\x70\x8a\x5d\x46\x35\xc5\x1b\x93"
		"\xae\x3d\x86\xfd\xb1\x55\x3e\xe0",
	.signature_len = 16,
	.type = TYPE_SOFTWARE
	},
	{
		.name = "HighPoint RAID controller",
		.signature = "ycgl",
		.signature_len = 4,
		.type = TYPE_RAID
	},
	{
		/* https://bugs.launchpad.net/bugs/987022 */
		.name = "Acer registration utility (?)",
		.signature = "GREGRegDone.Tag\x00",
		.signature_len = 16,
		.type = TYPE_SOFTWARE
	}
};

#define ARRAY_SIZE(array) (sizeof (array) / sizeof (array[0]))

int64_t grub_disk_read(HANDLE dest_dev, grub_disk_addr_t sector, void *buf)
{
	return read_sectors(dest_dev, GRUB_DISK_SECTOR_SIZE, sector, 1, buf);
}

int64_t grub_disk_write(HANDLE dest_dev, grub_disk_addr_t sector, void *buf)
{
	return write_sectors(dest_dev, GRUB_DISK_SECTOR_SIZE, sector, 1, buf);
}

grub_err_t pc_partition_map_embed(HANDLE dest_dev, grub_disk_addr_t last_available_sector, unsigned int *nsectors, unsigned int max_nsectors, grub_disk_addr_t **sectors)
{
	grub_disk_addr_t end = last_available_sector;
	unsigned i, j;
	char *embed_signature_check;
	unsigned int orig_nsectors, avail_nsectors;

	orig_nsectors = *nsectors;
	*nsectors = (unsigned int) (end - 1);
	avail_nsectors = *nsectors;
	if (*nsectors > max_nsectors)
		*nsectors = max_nsectors;
	*sectors = malloc(*nsectors * sizeof(**sectors));
	if (!*sectors) return ERROR_OUTOFMEMORY;
	for (i = 0; i < *nsectors; i++) {
		(*sectors)[i] = 1 + i;
	}

	/* Check for software that is already using parts of the embedding area. */
	embed_signature_check = malloc(GRUB_DISK_SECTOR_SIZE);
	for (i = 0; i < *nsectors; i++)	{
		if (-1 == grub_disk_read(dest_dev, (*sectors)[i], embed_signature_check)) continue;

		for (j = 0; j < ARRAY_SIZE(embed_signatures); j++) {
			if (!memcmp(embed_signatures[j].signature, embed_signature_check, embed_signatures[j].signature_len)) break;
		}

		if (j == ARRAY_SIZE(embed_signatures)) continue;
		uprintf(message_warn[embed_signatures[j].type], (*sectors)[i], embed_signatures[j].name);
		avail_nsectors--;
		if (avail_nsectors < *nsectors) *nsectors = avail_nsectors;

		/* Avoid this sector.  */
		for (j = i; j < *nsectors; j++) (*sectors)[j]++;

		/* Have we run out of space?  */
		if (avail_nsectors < orig_nsectors) break;

		/* Make sure to check the next sector.  */
		i--;
	}
	free(embed_signature_check);

	if (*nsectors < orig_nsectors) {
		uprintf("other software is using the embedding area, and "
			"there is not enough room for core.img.  Such "
			"software is often trying to store data in a way "
			"that avoids detection.  We recommend you "
			"investigate");
		return ERROR_OUTOFMEMORY;
	}

	return ERROR_SUCCESS;
}

#define grub_boot_blocklist grub_pc_bios_boot_blocklist

#pragma pack (push, 1)
struct grub_pc_bios_boot_blocklist
{
    grub_uint64_t start;
    grub_uint16_t len;
    grub_uint16_t segment;
};
#pragma pack(pop)

struct blocklists
{
    struct grub_boot_blocklist *first_block, *block;
#ifdef GRUB_SETUP_BIOS
    grub_uint16_t current_segment;
#endif
    grub_uint16_t last_length;
    grub_disk_addr_t first_sector;
};

/* Helper for setup.  */
static void
save_blocklists(grub_disk_addr_t sector, unsigned offset, unsigned length,
    void *data)
{
    struct blocklists *bl = data;
    struct grub_boot_blocklist *prev = bl->block + 1;
    grub_uint64_t seclen;

    uprintf("saving <%"  PRIu64 ",%u,%u>",
        sector, offset, length);

    if (bl->first_sector == (grub_disk_addr_t)-1)
    {
        if (offset != 0 || length < GRUB_DISK_SECTOR_SIZE)
            PRINT_ERROR_MSG("the first sector of the core file is not sector-aligned");

        bl->first_sector = sector;
        sector++;
        length -= GRUB_DISK_SECTOR_SIZE;
        if (!length)
            return;
    }

    if (offset != 0 || bl->last_length != 0)
        PRINT_ERROR_MSG("non-sector-aligned data is found in the core file");

    seclen = (length + GRUB_DISK_SECTOR_SIZE - 1) >> GRUB_DISK_SECTOR_BITS;

    if (bl->block != bl->first_block
        && (grub_target_to_host64(prev->start)
            + grub_target_to_host16(prev->len)) == sector)
    {
        grub_uint16_t t = grub_target_to_host16(prev->len);
        t += seclen;
        prev->len = grub_host_to_target16(t);
    }
    else
    {
        bl->block->start = grub_host_to_target64(sector);
        bl->block->len = grub_host_to_target16(seclen);
#ifdef GRUB_SETUP_BIOS
        bl->block->segment = grub_host_to_target16(bl->current_segment);
#endif

        bl->block--;
        if (bl->block->len)
            PRINT_ERROR_MSG("the sectors of the core file are too fragmented");
    }

    bl->last_length = length & (GRUB_DISK_SECTOR_SIZE - 1);
#ifdef GRUB_SETUP_BIOS
    bl->current_segment += seclen << (GRUB_DISK_SECTOR_BITS - 4);
#endif
}


int grub_util_bios_setup(const wchar_t *core_path, HANDLE dest_dev, grub_disk_addr_t last_available_sector)
{
	unsigned int nsec, maxsec;
	grub_uint16_t core_sectors;
	char *core_img = NULL;
	grub_disk_addr_t *sectors = NULL;
	int retResult = 0;
	size_t countRead, core_size;
	FILE *coreImgFile = NULL;

	uprintf("grub_util_bios_setup called with '%ls' and last_available_sector=%"PRIu64, core_path, last_available_sector);

	// read data from core.img
	IFFALSE_GOTOERROR(0 == _wfopen_s(&coreImgFile, core_path, L"rb"), "Error opening core.img file");
	fseek(coreImgFile, 0L, SEEK_END);
	core_size = ftell(coreImgFile);
	rewind(coreImgFile);
	uprintf("Size of SBR is %d bytes from %ls", core_size, core_path);

	core_img = malloc(core_size);
	size_t sizeRead = 0;
	while (!feof(coreImgFile) && sizeRead < core_size) {
		countRead = fread(core_img + sizeRead, 1, GRUB_DISK_SECTOR_SIZE, coreImgFile);
		sizeRead += countRead;
	}
	// grub 2 stuff copied from https://github.com/endlessm/grub/blob/master/util/setup.c#L490

	struct blocklists bl;

	bl.first_sector = (grub_disk_addr_t)-1;

#ifdef GRUB_SETUP_BIOS
	bl.current_segment =
		GRUB_BOOT_I386_PC_KERNEL_SEG + (GRUB_DISK_SECTOR_SIZE >> 4);
#endif
	bl.last_length = 0;

	/* Have FIRST_BLOCK to point to the first blocklist.  */
	bl.first_block = (struct grub_boot_blocklist *) (core_img
		+ GRUB_DISK_SECTOR_SIZE
		- sizeof(*bl.block));

	core_sectors = (grub_uint16_t) ((core_size + GRUB_DISK_SECTOR_SIZE - 1) >> GRUB_DISK_SECTOR_BITS);

	if (core_size < GRUB_DISK_SECTOR_SIZE) uprintf("the size of '%s' is too small", core_path);
	if (core_size > 0xFFFF * GRUB_DISK_SECTOR_SIZE) uprintf("the size of '%s' is too large", core_path);

	nsec = core_sectors;
	maxsec = 2 * core_sectors;

	uprintf("nsec=%d, maxsec=%d", nsec, maxsec);

	unsigned int max_allowed_sec = (0x78000 - GRUB_KERNEL_I386_PC_LINK_ADDR) >> GRUB_DISK_SECTOR_BITS;
	if (maxsec > max_allowed_sec) maxsec = max_allowed_sec;

	int err = pc_partition_map_embed(dest_dev, last_available_sector, &nsec, maxsec, &sectors);
	if (err == ERROR_SUCCESS && nsec < core_sectors)  uprintf("Your embedding area is unusually small. core.img won't fit in it.");
	IFFALSE_GOTOERROR(err == ERROR_SUCCESS && nsec >= core_sectors, "Error after pc_partition_map_embed.");

    /* Clean out the blocklists.  */
    bl.block = bl.first_block;
    while (bl.block->len)
    {
        memset(bl.block, 0, sizeof(bl.block));

        bl.block--;

        IFTRUE_GOTOERROR(((char *)bl.block <= core_img), "no terminator in the core image");
    }

    /* Bake the list of sectors returned by pc_partition_map_embed() into the core image */
    bl.block = bl.first_block;
    unsigned int i;
    for (i = 0; i < nsec; i++)
        save_blocklists(sectors[i],
            0, GRUB_DISK_SECTOR_SIZE, &bl);

    /* Make sure that the last blocklist is a terminator.  */
    if (bl.block == bl.first_block)
        bl.block--;
    bl.block->start = 0;
    bl.block->len = 0;
    bl.block->segment = 0;

	/* Round up to the nearest sector boundary, and zero the extra memory */
	core_img = realloc(core_img, nsec * GRUB_DISK_SECTOR_SIZE);
	//assert(core_img && (nsec * GRUB_DISK_SECTOR_SIZE >= core_size));
	memset(core_img + core_size, 0, nsec * GRUB_DISK_SECTOR_SIZE - core_size);

	uprintf("after pc_partition_map_embed nsec=%d", nsec);

	grub_size_t no_rs_length;
	no_rs_length = grub_target_to_host16(grub_get_unaligned16(core_img + GRUB_DISK_SECTOR_SIZE + GRUB_KERNEL_I386_PC_NO_REED_SOLOMON_LENGTH));
	uprintf("no_rs_length=%"PRIu64, no_rs_length);

	if (no_rs_length == 0xffff) uprintf("core.img version mismatch");

	grub_set_unaligned32((core_img + GRUB_DISK_SECTOR_SIZE + GRUB_KERNEL_I386_PC_REED_SOLOMON_REDUNDANCY), nsec * GRUB_DISK_SECTOR_SIZE - core_size);

	uprintf("nsec*GRUB_DISK_SECTOR_SIZE = %u, core_size=%u", nsec * GRUB_DISK_SECTOR_SIZE, core_size);

	grub_size_t redundancy = grub_host_to_target32(nsec * GRUB_DISK_SECTOR_SIZE - core_size);
	grub_size_t data_size = core_size - no_rs_length - GRUB_DISK_SECTOR_SIZE;
	void *buffer_start = core_img + no_rs_length + GRUB_DISK_SECTOR_SIZE;

	uprintf("calling grub_reed_solomon_add_redundancy with buffer=0x%08X, data_size=%"PRIu64", redundancy=%"PRIu64, buffer_start, data_size, redundancy);
	grub_reed_solomon_add_redundancy(buffer_start, data_size, redundancy);

	/* Write the core image onto the disk.  */
	for (size_t i = 0; i < nsec; i++) {
		if (-1 == grub_disk_write(dest_dev, sectors[i], core_img + i * GRUB_DISK_SECTOR_SIZE)) {
			uprintf("Error on grub_disk_write with sector %u", sectors[i]);
			goto error;
		}
	}

	retResult = 1;

error:
	if (coreImgFile != NULL) {
		fclose(coreImgFile);
		coreImgFile = NULL;
	}

	safe_free(sectors);
	safe_free(core_img);
	return retResult;
}
