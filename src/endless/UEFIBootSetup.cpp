#include "stdafx.h"
#include <Windows.h>
#include <WinIoCtl.h>

#include "GeneralCode.h"
extern "C" {
#include "rufus.h"
}

#pragma pack(1)

typedef struct {
	DWORD attributes;
	WORD file_path_list_length;
} EFI_LOAD_OPTION;

typedef struct {
	BYTE type;
	BYTE subtype;
	WORD length;
	BYTE data[1];
} EFI_DEVICE_PATH;

typedef struct {
	BYTE type;
	BYTE subtype;
	WORD length;
} EFI_FILE_PATH;

typedef struct {
	BYTE type;
	BYTE subtype;
	WORD length;
	DWORD part_num;
	DWORDLONG start;
	DWORDLONG size;
	BYTE signature[16];
	BYTE mbr_type;
	BYTE signature_type;
} EFI_HARD_DRIVE_PATH;

typedef struct {
	BYTE type;
	BYTE subtype;
	WORD length;
} EFI_END_DEVICE_PATH;

struct ExtraEntries : DRIVE_LAYOUT_INFORMATION_EX
{
	PARTITION_INFORMATION_EX PartitionEntry[9];
};


int print_drive_letter(WCHAR *VolumeName) {
	DWORD CharCount = MAX_PATH + 1;
	DWORD error;
	PWCHAR Names = NULL;
	PWCHAR NameIDX = NULL;
	BOOL Success = FALSE;

	VolumeName[wcslen(VolumeName)] = L'\\';

retry:
	Names = (PWCHAR) new BYTE[CharCount * sizeof(WCHAR)];

	if (!Names) {
		wprintf(L"Unable to allocate RAM for drive letter query\n");
		return 1;
	}
	if (!GetVolumePathNamesForVolumeNameW(VolumeName, Names, CharCount, &CharCount)) {
		error = GetLastError();
		if (error == ERROR_MORE_DATA) {
			// CharCount has increased, try again
			delete[] Names;
			Names = NULL;
			goto retry;
		}
		wprintf(L"Unable to get volume path names: %d\n", error);
		return 2;
	}

	for (NameIDX = Names; NameIDX[0] != L'\0'; NameIDX += wcslen(NameIDX) + 1) {
		wprintf(L" (%s)", NameIDX);
	}
	return 0;
}

int print_partition_info(GUID guid, ULONG mbr, DWORD partnum) {
	HANDLE volume_handle;
	HANDLE drive_handle;
	WCHAR VolumeName[MAX_PATH];
	WCHAR DosName[MAX_PATH];
	DWORD size;
	struct ExtraEntries layout;
	PARTITION_INFORMATION_EX partition;
	PARTITION_INFORMATION_GPT *gpt;

	volume_handle = FindFirstVolumeW(VolumeName, ARRAYSIZE(VolumeName));

	for (;;) {
		BOOL is_disk;
		VolumeName[wcslen(VolumeName) - 1] = '\0';
		QueryDosDeviceW(&VolumeName[4], DosName, ARRAYSIZE(DosName));
		drive_handle = CreateFile(VolumeName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH, 0);

		if (!drive_handle || drive_handle == INVALID_HANDLE_VALUE) {
			wprintf(L"Unable to open volume %s, error %d\n", VolumeName, GetLastError());
			goto next;
		}

		if (!DeviceIoControl(drive_handle, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, 0, 0, &layout, sizeof(layout), &size, 0)) {
			wprintf(L"Unable to get drive information on %s, %d\n", VolumeName, GetLastError());
		}
		else {
			if (layout.PartitionStyle == PARTITION_STYLE_MBR) {
				if (layout.Mbr.Signature == mbr) {
					is_disk = TRUE;
				}
			}
		}

		if (!DeviceIoControl(drive_handle, IOCTL_DISK_GET_PARTITION_INFO_EX, 0, 0, &partition, sizeof(partition), &size, 0)) {
			wprintf(L"Unable to get partition information on %s, %d\n", VolumeName, GetLastError());
			goto next;
		}
		if (partition.PartitionStyle == PARTITION_STYLE_GPT) {
			GUID partguid;
			gpt = &partition.Gpt;
			memcpy(&partguid, &gpt->PartitionId, sizeof(partguid));
			if (partguid == guid) {
				print_drive_letter(VolumeName);
				goto out;
			}
		}
		else if (partition.PartitionStyle == PARTITION_STYLE_MBR) {
			if (is_disk == TRUE && partition.PartitionNumber == partnum) {
				print_drive_letter(VolumeName);
				goto out;
			}
		}

	next:
		if (!FindNextVolumeW(volume_handle, VolumeName, ARRAYSIZE(VolumeName))) {
			break;
		}
	}
	wprintf(L"(Drive not mounted)");

out:
	FindVolumeClose(volume_handle);
	return 0;
}

int print_entries(void) {wchar_t varname[9];
	BYTE vardata[10000];
	DWORD size;
	WORD *bootorder;
	int i;
	int countEmpty = 0;

	size = GetFirmwareEnvironmentVariable(L"BootOrder", _T("{8BE4DF61-93CA-11d2-AA0D-00E098032B8C}"), vardata, sizeof(vardata));
	if (size > 0) {
		bootorder = (WORD *)vardata;
		uprintf("Bootorder: ");
		for (i = 0; i < (int)(size / 2); i++) {
			uprintf("%d ", bootorder[i]);
		}
		uprintf("\n");
	}

	size = GetFirmwareEnvironmentVariable(L"BootNext", _T("{8BE4DF61-93CA-11d2-AA0D-00E098032B8C}"), vardata, sizeof(vardata));
	if (size > 0) {
		WORD *bootnext = (WORD *)vardata;
		uprintf("BootNext: %d\n", *bootnext);
	}
	for (i = 0; i <= 0xffff && countEmpty < 5; i++) {
		EFI_HARD_DRIVE_PATH *hdpath;
		swprintf(varname, sizeof(varname), L"Boot%04x", i);
		size = GetFirmwareEnvironmentVariable(varname, _T("{8BE4DF61-93CA-11d2-AA0D-00E098032B8C}"), vardata, sizeof(vardata));
		if (size > 0) {
			GUID guid;
			ULONG mbr = 0;
			DWORD partnum = 0;
			int signature_type = -1;
			wchar_t *description = (wchar_t *)&vardata[6];
			EFI_DEVICE_PATH *devpath = (EFI_DEVICE_PATH *)&vardata[6 + wcslen(description) * 2 + 2];
			uprintf("%d: %ls - ", i, description);
			while (devpath) {
				switch (devpath->type) {
				case 0x4: // Media device type
					switch (devpath->subtype) {
					case 0x1: // Hard drive
						hdpath = (EFI_HARD_DRIVE_PATH *)devpath;
						signature_type = hdpath->signature_type;
						switch (signature_type) {
						case 0x1:
							partnum = hdpath->part_num;
							mbr = hdpath->signature[0] | hdpath->signature[1] << 8 | hdpath->signature[2] << 16 | hdpath->signature[3] << 24;
							uprintf("Partition %d {%08lX} ", partnum, mbr);
							break;
						case 0x2:
							memcpy(&guid, &hdpath->signature, sizeof(guid));
							uprintf("Partition %d {%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX} ", hdpath->part_num,
								guid.Data1, guid.Data2, guid.Data3,
								guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
								guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
							break;
						default:
							uprintf("Partition %d ", hdpath->part_num);
							break;
						}
						break;
					case 0x4:
						wchar_t *filepath = (wchar_t *)((BYTE *)devpath + 4);
						uprintf("Path %ls ", filepath);
						break;
					}
					break;
				case 0x7f: // End of device path
					switch (signature_type) {
					case 0x1:
					case 0x2:
						print_partition_info(guid, mbr, partnum);
						break;
					default:
						break;
					}
					devpath = NULL;
					uprintf("\n");
					break;
				}
				if (devpath)
					devpath = (EFI_DEVICE_PATH *)((BYTE *)devpath + devpath->length);
			}
			countEmpty = 0;
		} else {
			countEmpty++;
		}
	}
	return 0;
}

bool EFIRequireNeededPrivileges()
{
	bool retResult = false;
	HANDLE tokenHandle = INVALID_HANDLE_VALUE;
	TOKEN_PRIVILEGES tp;
	ZeroMemory(&tp, sizeof(tp));

	IFFALSE_GOTOERROR(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &tokenHandle) != 0, "Error on OpenProcessToken");
	IFFALSE_GOTOERROR(LookupPrivilegeValue(NULL, SE_SYSTEM_ENVIRONMENT_NAME, &tp.Privileges[0].Luid) != 0, "Error on LookupPrivilegeValue");

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	IFFALSE_GOTOERROR(AdjustTokenPrivileges(tokenHandle, FALSE, &tp, 0, NULL, 0) != 0, "Error on AdjustTokenPrivileges");

	retResult = true;

error:
	safe_closehandle(tokenHandle);

	return retResult;
}

#define UEFI_BOOT_NAMESPACE			L"{8BE4DF61-93CA-11d2-AA0D-00E098032B8C}"
#define UEFI_VAR_BOOTORDER			L"BootOrder"
#define UEFI_VAR_BOOT_ENTRY_FORMAT	L"Boot%04x"

static BYTE vardata[10000];
static BYTE vardata1[1000];

int EFIGetBootEntryNumber(const wchar_t *desc) {
	DWORD size;
	WORD *bootorder;
	int i;
	int lowestfree = 0, target = -1;
	wchar_t varname[9];

	size = GetFirmwareEnvironmentVariable(UEFI_VAR_BOOTORDER, UEFI_BOOT_NAMESPACE, vardata, sizeof(vardata));
	if (size > 0) {
		bootorder = (WORD *)vardata;
		for (i = 0; i < (int)(size / 2); i++) {
			swprintf(varname, sizeof(varname), UEFI_VAR_BOOT_ENTRY_FORMAT, bootorder[i]);
			size = GetFirmwareEnvironmentVariable(varname, UEFI_BOOT_NAMESPACE, vardata1, sizeof(vardata1));
			IFFALSE_CONTINUE(size > 0, "");

			wchar_t *description = (wchar_t *)&vardata1[6];
			// TODO: should we add more validation than just the boot entry description matching?
			if (0 == wcscmp(desc, description)) {
				target = bootorder[i];
				lowestfree = 0x10000;
				break;
			}

			if (bootorder[i] >= lowestfree) {
				lowestfree = bootorder[i] + 1;
			}
		}
	}

	/* Find a free boot entry */
	for (i = lowestfree; i <= 0xffff; i++) {
		swprintf(varname, sizeof(varname), UEFI_VAR_BOOT_ENTRY_FORMAT, i);
		size = GetFirmwareEnvironmentVariable(varname, UEFI_BOOT_NAMESPACE, vardata, sizeof(vardata));
		if (GetLastError() == ERROR_ENVVAR_NOT_FOUND && target == -1) {
			target = i;
			break;
		}
	}

	return target;
}

bool EFICreateNewEntry(const wchar_t *drive, wchar_t *path, wchar_t *desc) {
	int i;
	int target = -1, windows = -1;

	wchar_t varname[9];
	wchar_t volumename[1000];
	wchar_t description[1000];
	wchar_t pathname[1000];

	DWORD varsize;
	DWORD size, offset, blocksize;
	WORD *bootorder;
	BYTE *target_addr;
	int lowestfree = 0;

	EFI_HARD_DRIVE_PATH hd_path;
	EFI_LOAD_OPTION *load_option;
	EFI_FILE_PATH *file_path;
	EFI_END_DEVICE_PATH end_path;

	HANDLE drive_handle;
	PARTITION_INFORMATION_EX partition;
	PARTITION_INFORMATION_GPT *gpt;

	STORAGE_PROPERTY_QUERY query;
	STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR alignment;

	bool retResult = false;

	//print_entries();

	target = EFIGetBootEntryNumber(desc);
	IFFALSE_GOTOERROR(target != -1, "Failed to find a free boot variable");

	/* Ensure that strings are in UTF-16 */
	swprintf(description, sizeof(description), L"%s", desc);
	swprintf(volumename, sizeof(description), L"\\\\.\\%s", drive);

	/* We need to get information on the EFI system partition in order to construct the UEFI device path */
	drive_handle = CreateFile(volumename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH, 0);

	IFFALSE_GOTOERROR(drive_handle != NULL && drive_handle != INVALID_HANDLE_VALUE, "Unable to open drive.");
	IFFALSE_GOTOERROR(DeviceIoControl(drive_handle, IOCTL_DISK_GET_PARTITION_INFO_EX, 0, 0, &partition, sizeof(partition), &size, 0) != 0, "Unable to get partition information");
	IFFALSE_GOTOERROR(partition.PartitionStyle == PARTITION_STYLE_GPT, "Selected disk is not a GPT disk.");

	gpt = &partition.Gpt;

	query.PropertyId = StorageAccessAlignmentProperty;
	query.QueryType = PropertyStandardQuery;

	if (!DeviceIoControl(drive_handle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &alignment, sizeof(alignment), &size, 0)) {
		/* If the device doesn't implement this ioctl, assume 512-byte blocks */
		blocksize = 512;
	} else {
		blocksize = alignment.BytesPerLogicalSector;
	}

	/* Build the device path. First entry describes the partition */
	hd_path.type = 4; /* Media device */
	hd_path.subtype = 1; /* Hard drive */
	hd_path.length = sizeof(hd_path);
	hd_path.part_num = partition.PartitionNumber;
	hd_path.start = partition.StartingOffset.QuadPart / blocksize;
	hd_path.size = partition.PartitionLength.QuadPart / blocksize;

	memcpy(hd_path.signature, &gpt->PartitionId, 16);
	hd_path.mbr_type = 2; /* GPT */
	hd_path.signature_type = 2; /* GPT partition UUID */

	/* Size of the header + size of the UTF-16 string + NULL termination */
	file_path = (EFI_FILE_PATH *)LocalAlloc(LMEM_ZEROINIT, sizeof(EFI_FILE_PATH) + wcslen(path) * 2 + 2);

	file_path->length = (WORD) (sizeof(EFI_FILE_PATH) + wcslen(path) * 2 + 2);
	file_path->type = 4; /* Media device */
	file_path->subtype = 4; /* File path */
	swprintf(pathname, sizeof(pathname), L"%s", path);
	memcpy(((BYTE*)file_path) + sizeof(EFI_FILE_PATH), pathname, wcslen(path) * 2 + 2);

	/* Path terminator */
	end_path.type = 0x7f; /* Descriptor end */
	end_path.subtype = 0xff; /* Full path end */
	end_path.length = 4;

	/* The boot variable contains an EFI_LOAD_OPTION header, the path components and the descriptive string */
	varsize = sizeof(EFI_LOAD_OPTION) + hd_path.length + file_path->length + end_path.length + wcslen(description) * 2 + 2;

	load_option = (EFI_LOAD_OPTION *)LocalAlloc(LMEM_ZEROINIT, varsize);
	target_addr = ((BYTE *)load_option) + sizeof(EFI_LOAD_OPTION);
	load_option->attributes = 1; /* Active */
	load_option->file_path_list_length = hd_path.length + file_path->length + end_path.length;

	/* Descriptive string goes here */
	memcpy(target_addr, description, wcslen(description) * 2 + 2);

	/* Followed by the partition descriptor */
	offset = wcslen(description) * 2 + 2;
	memcpy(target_addr + offset, &hd_path, hd_path.length);

	/* Followed by the path descriptor */
	offset += hd_path.length;
	memcpy(target_addr + offset, file_path, file_path->length);

	/* Followed by the terminator */
	offset += file_path->length;
	memcpy(target_addr + offset, &end_path, end_path.length);

	/* Generate the boot variable name */
	swprintf(varname, sizeof(varname), UEFI_VAR_BOOT_ENTRY_FORMAT, target);
#if 0
	for (i = 0; i<varsize; i++) {
		BYTE *dummy = (BYTE *)load_option;
		printf("%d %x %c\n", i, dummy[i] & 0xff, dummy[i] & 0xff);
	};
#endif

	/* And write it to firmware */
	IFFALSE_GOTOERROR(SetFirmwareEnvironmentVariable(varname, UEFI_BOOT_NAMESPACE, load_option, varsize), "Error on SetFirmwareEnvironmentVariable");
	uprintf("Created entry for %ls (%ls) at %d\n", path, description, target);

	/* Add our entry to the boot order */
	size = GetFirmwareEnvironmentVariable(UEFI_VAR_BOOTORDER, UEFI_BOOT_NAMESPACE, vardata, sizeof(vardata));
	IFFALSE_GOTOERROR(size > 0, "Error on querying for BootOrder with GetFirmwareEnvironmentVariable");

	bootorder = (WORD *)vardata;
	int position = (int)(size / 2);
	int extraBytes = 2;

	// find if our entry is already in the list for some reason
	for (i = 0; i < (int)(size / 2); i++) {
		if (bootorder[i] == target) {
			position = i;
			extraBytes = 0;
			break;
		}
	}

	// and add it at the beginning of the list
	for (i = position; i > 0 ; i--) {
		bootorder[i] = bootorder[i - 1];
	}
	bootorder[0] = target;
	IFFALSE_GOTOERROR(SetFirmwareEnvironmentVariable(UEFI_VAR_BOOTORDER, UEFI_BOOT_NAMESPACE, vardata, size + extraBytes), "Error on SetFirmwareEnvironmentVariable");

	//print_entries();

	retResult = true;

error: 
	return retResult;
}

bool EFIRemoveEntry(wchar_t *desc) {
	wchar_t varname[9];
	int target = -1;

	//print_entries();

	target = EFIGetBootEntryNumber(desc);
	IFFALSE_RETURN_VALUE(target != -1, "Failed to find EFI entry for Endless OS", false);

	swprintf(varname, sizeof(varname), UEFI_VAR_BOOT_ENTRY_FORMAT, target);
	/* Writing a zero length variable deletes it */
	IFFALSE_RETURN_VALUE(SetFirmwareEnvironmentVariable(varname, L"{8BE4DF61-93CA-11d2-AA0D-00E098032B8C}", NULL, 0), "Error on SetFirmwareEnvironmentVariable", false);

	/* Remove our entry from the boot order */
	DWORD size = GetFirmwareEnvironmentVariable(UEFI_VAR_BOOTORDER, UEFI_BOOT_NAMESPACE, vardata, sizeof(vardata));
	IFFALSE_RETURN_VALUE(size > 0, "Error on querying for BootOrder with GetFirmwareEnvironmentVariable", false);

	WORD *bootorder = (WORD *)vardata;
	int nrEntries = (int)(size / 2);
	int position = nrEntries;
	int i;

	// find our entry in the list
	for (i = 0; i < (int)(size / 2); i++) {
		if (bootorder[i] == target) {
			position = i;
			break;
		}
	}

	if (position != nrEntries) {
		// and remove it from the list
		for (i = position; i < nrEntries - 1; i++) {
			bootorder[i] = bootorder[i + 1];
		}
		IFFALSE_RETURN_VALUE(SetFirmwareEnvironmentVariable(UEFI_VAR_BOOTORDER, UEFI_BOOT_NAMESPACE, vardata, size - 2), "Error on SetFirmwareEnvironmentVariable", false);
	} else {
		uprintf("Our EFI entry %ls was not found in BootOrder list", varname);
	}

	//print_entries();

	return true;
}