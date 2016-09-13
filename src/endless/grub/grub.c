#include <stdlib.h>
#include <stdio.h>

#include "grub.h"
#include "reed_solomon.h"
#include "GeneralCode.h"
#include "rufus.h"
#include "file.h"

#define GRUB_DISK_SECTOR_SIZE	0x200
#define GRUB_DISK_SECTOR_BITS	9

#define GRUB_KERNEL_I386_PC_LINK_ADDR  0x9000

/* Offset of field holding no reed solomon length.  */
#define GRUB_KERNEL_I386_PC_NO_REED_SOLOMON_LENGTH      0x14

/* Offset of reed_solomon_redundancy.  */
#define GRUB_KERNEL_I386_PC_REED_SOLOMON_REDUNDANCY	0x10

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

int grub_util_bios_setup(const wchar_t *core_path, HANDLE dest_dev, grub_disk_addr_t last_available_sector)
{
	unsigned int nsec, maxsec;
	grub_uint16_t core_sectors;
	char *core_img = NULL;
	grub_disk_addr_t *sectors = NULL;
	int retResult = 0;
	size_t countRead, core_size;
	FILE *coreImgFile = NULL;

	uprintf("grub_util_bios_setup called with '%ls' and last_available_sector=%I64i", core_path, last_available_sector);

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

	/* Round up to the nearest sector boundary, and zero the extra memory */
	core_img = realloc(core_img, nsec * GRUB_DISK_SECTOR_SIZE);
	//assert(core_img && (nsec * GRUB_DISK_SECTOR_SIZE >= core_size));
	memset(core_img + core_size, 0, nsec * GRUB_DISK_SECTOR_SIZE - core_size);

	uprintf("after pc_partition_map_embed nsec=%d", nsec);

	grub_size_t no_rs_length;
	no_rs_length = grub_get_unaligned16(core_img + GRUB_DISK_SECTOR_SIZE + GRUB_KERNEL_I386_PC_NO_REED_SOLOMON_LENGTH);
	uprintf("no_rs_length=%I64u", no_rs_length);

	if (no_rs_length == 0xffff) uprintf("core.img version mismatch");

	grub_set_unaligned32((core_img + GRUB_DISK_SECTOR_SIZE + GRUB_KERNEL_I386_PC_REED_SOLOMON_REDUNDANCY), nsec * GRUB_DISK_SECTOR_SIZE - core_size);

	uprintf("nsec*GRUB_DISK_SECTOR_SIZE = %u, core_size=%u", nsec * GRUB_DISK_SECTOR_SIZE, core_size);

	grub_size_t redudancy = nsec * GRUB_DISK_SECTOR_SIZE - core_size;
	grub_size_t data_size = core_size - no_rs_length - GRUB_DISK_SECTOR_SIZE;
	void *buffer_start = core_img + no_rs_length + GRUB_DISK_SECTOR_SIZE;

	uprintf("calling grub_reed_solomon_add_redundancy with buffer=0x%08X, data_size=%I64u, redudancy=%I64u", buffer_start, data_size, redudancy);
	grub_reed_solomon_add_redundancy(buffer_start, data_size, redudancy);

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
