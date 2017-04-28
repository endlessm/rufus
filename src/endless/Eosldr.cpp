#include "stdafx.h"
#include "Eosldr.h"
#include "GeneralCode.h"
#include "WMI.h"

// Paths for g2ldr binaries, used to chainload GRUB from the Windows Boot
// Manager on BIOS systems, rather than the other way round.
#define EOSLDR_SUBDIRECTORY             L"eosldr"
// Stub loaded by WBM
#define EOSLDR_MBR_FILE                 L"eosldr.mbr"
// GRUB core.img with a special preamble, loaded by eosldr.mbr
#define EOSLDR_IMG_FILE                 L"eosldr"

// The section in Windows XP's boot.ini for boot menu entries
#define BOOT_INI_SECTION        L"operating systems"

// GUID for Endless OS's entry in the BCD boot menu, stored in the Uninstall
// registry key
#define REGKEY_BCD_GUID         L"EndlessBcdGuid"

bool EosldrInstaller::CanInstall(const CString & bootFilesPath)
{
    FUNCTION_ENTER_FMT("%ls", bootFilesPath);

    return !!PathFileExists(bootFilesPath + EOSLDR_SUBDIRECTORY);
}

static bool CopyAndHide(const CString &source, const CString &target)
{
    FUNCTION_ENTER_FMT("%ls -> %ls", source, target);

    IFFALSE_RETURN_VALUE(
        CopyFileEx(source, target, NULL, NULL, NULL, 0),
        "CopyFileEx failed", false);
    IFFALSE_RETURN_VALUE(
        SetFileAttributes(target, 
            FILE_ATTRIBUTE_READONLY |
            FILE_ATTRIBUTE_HIDDEN |
            FILE_ATTRIBUTE_SYSTEM),
        "SetFileAttributes failed", false);

    return true;
}

bool EosldrInstaller::Install(const CString & systemDriveLetter, const CString & bootFilesPath)
{
    const CString eosldrImgPath = systemDriveLetter + EOSLDR_IMG_FILE;
    const CString eosldrMbrPath = systemDriveLetter + EOSLDR_MBR_FILE;

    const CString eosldrSourceDir = bootFilesPath + EOSLDR_SUBDIRECTORY L"\\";
    const CString eosldrImgSource = eosldrSourceDir + EOSLDR_IMG_FILE;
    const CString eosldrMbrSource = eosldrSourceDir + EOSLDR_MBR_FILE;

    if (!(
        CopyAndHide(eosldrImgSource, eosldrImgPath) &&
        CopyAndHide(eosldrMbrSource, eosldrMbrPath)
        )) {
        return false;
    }

    return AddToBootOrder(systemDriveLetter, eosldrMbrPath);
}

bool EosldrInstaller::IsInstalled(const CString & systemDriveLetter)
{
    FUNCTION_ENTER_FMT("%ls", systemDriveLetter);

    return !!PathFileExists(systemDriveLetter + EOSLDR_MBR_FILE);
}

static bool UnhideAndDelete(const CString &path)
{
    FUNCTION_ENTER_FMT("%ls", path);

    IFFALSE_PRINTERROR(SetFileAttributes(path, FILE_ATTRIBUTE_NORMAL),
        "SetFileAttributes failed; will try to delete anyway");
    IFFALSE_RETURN_VALUE(DeleteFile(path), "DeleteFile failed", false);

    return true;
}

bool EosldrInstaller::Uninstall(const CString & systemDriveLetter, bool &foundBootEntry)
{
    const CString eosldrImgPath = systemDriveLetter + EOSLDR_IMG_FILE;
    const CString eosldrMbrPath = systemDriveLetter + EOSLDR_MBR_FILE;

    IFFALSE_PRINTERROR(RemoveFromBootOrder(systemDriveLetter, eosldrMbrPath, foundBootEntry),
        "Can't remove Endless OS bootloader from boot menu");
    IFFALSE_PRINTERROR(UnhideAndDelete(eosldrMbrPath), "Deleting eosldr.mbr failed");
    IFFALSE_PRINTERROR(UnhideAndDelete(eosldrImgPath), "Deleting eosldr failed");

    return true;
}

EosldrInstaller * EosldrInstaller::GetInstance(int nWindowsVersion, const CString & name, const CString & uninstallKey)
{
    if (nWindowsVersion >= WINDOWS_7) {
        return new EosldrInstallerBcd(name, uninstallKey);
    } else if (nWindowsVersion >= WINDOWS_XP) {
        return new EosldrInstallerBootIni(name);
    } else {
        uprintf("Unsupported Windows version %x", nWindowsVersion);
        return new EosldrInstaller(name);
    }
}

bool EosldrInstaller::AddToBootOrder(const CString & systemDriveLetter, const CString & eosldrMbrPath)
{
    FUNCTION_ENTER;

    return false;
}

bool EosldrInstaller::RemoveFromBootOrder(
    const CString & systemDriveLetter,
    const CString & eosldrMbrPath,
    bool &foundBootEntry)
{
    FUNCTION_ENTER;

    foundBootEntry = false;
    return false;
}

bool EosldrInstallerBootIni::AddToBootOrder(const CString & systemDriveLetter, const CString & eosldrMbrPath)
{
    FUNCTION_ENTER;

    bool dummyFoundBootEntry;

    return ModifyBootOrder(systemDriveLetter, eosldrMbrPath, true, dummyFoundBootEntry);
}

bool EosldrInstallerBootIni::RemoveFromBootOrder(
    const CString & systemDriveLetter,
    const CString & eosldrMbrPath,
    bool &foundBootEntry)
{
    FUNCTION_ENTER;

    IFFALSE_PRINTERROR(ModifyBootOrder(systemDriveLetter, eosldrMbrPath, false, foundBootEntry), "ModifyBootOrder failed");

    return true;
}

bool EosldrInstallerBootIni::ModifyBootOrder(const CString & systemDriveLetter, const CString & eosldrMbrPath, bool add, bool &foundBootEntry)
{
    const CString bootIni = systemDriveLetter + L"boot.ini";
    FUNCTION_ENTER_FMT("bootIni=%ls", bootIni);
    const DWORD originalAttributes = GetFileAttributes(bootIni);

    IFTRUE_RETURN_VALUE(originalAttributes == INVALID_FILE_ATTRIBUTES,
        "GetFileAttributes failed", false);
    IFFALSE_RETURN_VALUE(SetFileAttributes(bootIni, FILE_ATTRIBUTE_NORMAL),
        "SetFileAttributes failed", false);

    // passing NULL for the third parameter removes the entry
    if (add) {
        if (!WritePrivateProfileString(BOOT_INI_SECTION, eosldrMbrPath, m_name, bootIni)) {
            PRINT_ERROR_MSG_FMT("WritePrivateProfileString(..., %ls, %ls, %ls) failed",
                eosldrMbrPath, m_name, bootIni);
            IFFALSE_PRINTERROR(SetFileAttributes(bootIni, originalAttributes), "SetFileAttributes failed too");
            return false;
        }
    } else {
        // If there's an existing entry for eosldr with a non-empty-string label,
        // consider the boot entry to be found.
        wchar_t dummy[2] = { 0 };
        DWORD len = GetPrivateProfileString(BOOT_INI_SECTION, eosldrMbrPath, L"", dummy, ARRAYSIZE(dummy), bootIni);
        foundBootEntry = (len > 0);

        if (!WritePrivateProfileString(BOOT_INI_SECTION, eosldrMbrPath, NULL, bootIni)) {
            PRINT_ERROR_MSG_FMT("WritePrivateProfileString(..., %ls, NULL, %ls) failed",
                eosldrMbrPath, bootIni);
            IFFALSE_PRINTERROR(SetFileAttributes(bootIni, originalAttributes), "SetFileAttributes failed too");
            return false;
        }
    }

    IFFALSE_RETURN_VALUE(SetFileAttributes(bootIni, originalAttributes),
        "SetFileAttibutes failed for originalAttributes", false);

    return true;
}

bool EosldrInstallerBcd::AddToBootOrder(const CString & systemDriveLetter, const CString & eosldrMbrPath)
{
    FUNCTION_ENTER_FMT("%ls, %ls", systemDriveLetter, eosldrMbrPath);

    CString guid;
    IFFALSE_RETURN_VALUE(
        WMI::AddBcdEntry(m_name, eosldrMbrPath, guid),
        "AddBcdEntry failed", false);

    // At this point it's too late to fail: we've already added the entry to the boot order.
    // TODO: pass GUID into ::AddBcdEntry instead, and add it to the registry *first*
    // Assumes that m_uninstallKey has already been created (and populated with everything else).
    CRegKey registryKey;
    LSTATUS result;

    result = registryKey.Open(HKEY_LOCAL_MACHINE, m_uninstallKey);
    IFFALSE_RETURN_VALUE(result == ERROR_SUCCESS, "Failed to open Uninstall key", false);

    IFFALSE_RETURN_VALUE(ERROR_SUCCESS == registryKey.SetStringValue(REGKEY_BCD_GUID, guid),
        "Failed to set EndlessBcdGuid", false);

    return true;
}

bool EosldrInstallerBcd::RemoveFromBootOrder(
    const CString & systemDriveLetter,
    const CString & eosldrMbrPath,
    bool &foundBootEntry)
{
    FUNCTION_ENTER;

    foundBootEntry = false;

    CRegKey registryKey;
    LSTATUS result;

    result = registryKey.Open(HKEY_LOCAL_MACHINE, m_uninstallKey);
    IFFALSE_RETURN_VALUE(result == ERROR_SUCCESS, "Failed to open Uninstall key", false);

    wchar_t guid[40] = { 0 };
    ULONG nChars = ARRAYSIZE(guid);
    IFFALSE_RETURN_VALUE(
        ERROR_SUCCESS == registryKey.QueryStringValue(REGKEY_BCD_GUID, guid, &nChars),
        "Failed to get EndlessBcdGuid", false);

    IFFALSE_RETURN_VALUE(
        WMI::RemoveBcdEntry(guid, foundBootEntry),
        "RemoveBcdEntry failed", false);

    return true;
}
