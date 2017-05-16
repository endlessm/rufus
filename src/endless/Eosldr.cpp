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

// Windows XP's boot configuration file, stored in the root of the system drive
#define BOOT_INI                L"boot.ini"

// Our backup of boot.ini, taken before modifying it. We give it a well-known
// name so it can be easily found despite being readonly, system and
// hidden (like boot.ini itself); and use a UUID to remove the possibility of a
// collision with some other backup of the file.
// NB: not wide, so it can be used in a static debug message.
#define BOOT_INI_BACKUP         "boot-1930e839-4cc0-4a57-a8c2-7c4dbd248e36.ini"

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

// We mark eosldr[.mbr] readonly, hidden and system, but don't go all-out with
// restrictive ACLs like we do for /endless/endless.img etc. There's no
// irreplacable data in these files, and on systems booted with this method,
// deleting them just means the user loses access to Endless OS (in a fixable
// way), not Windows as well.
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

// Temporarily sets attributes of 'path' to NORMAL, restoring the original
// attributes on dispose.
static class Unveil {
public:
    Unveil(const CString &path)
        : m_path(path),
        m_originalAttributes(INVALID_FILE_ATTRIBUTES),
        m_succeeded(false)
    {
        FUNCTION_ENTER_FMT("%ls", path);
        m_originalAttributes = GetFileAttributes(path);
        IFTRUE_RETURN(m_originalAttributes == INVALID_FILE_ATTRIBUTES, "GetFileAttributes failed");
        IFFALSE_RETURN(SetFileAttributes(path, FILE_ATTRIBUTE_NORMAL), "SetFileAttributes failed");

        m_succeeded = true;
    }

    ~Unveil() {
        FUNCTION_ENTER_FMT("%ls", m_path);
        if (m_originalAttributes != INVALID_FILE_ATTRIBUTES) {
            // This cannot be fatal because at this point we have already edited boot.ini.
            // It is not the end of the world if it is not hidden; this is also unlikely to fail.
            IFFALSE_PRINTERROR(SetFileAttributes(m_path, m_originalAttributes),
                "Couldn't restore attributes");
        }
    }

    bool succeeded() { return m_succeeded; }
    DWORD originalAttributes() { return m_originalAttributes; }

private:
    const CString &m_path;
    DWORD m_originalAttributes;
    bool m_succeeded;
};

bool EosldrInstallerBootIni::ModifyBootOrder(const CString & systemDriveLetter, const CString & eosldrMbrPath, bool add, bool &foundBootEntry)
{
    const CString bootIni = systemDriveLetter + BOOT_INI;
    FUNCTION_ENTER_FMT("bootIni=%ls", bootIni);

    Unveil unveilBootIni(bootIni);
    IFFALSE_RETURN_VALUE(unveilBootIni.succeeded(), "Couldn't make boot.ini visible", false);

    const CString bootIniBackup = systemDriveLetter + _T(BOOT_INI_BACKUP);
    uprintf("Backing up %ls to %ls", bootIni, bootIniBackup);
    IFFALSE_PRINTERROR(SetFileAttributes(bootIniBackup, FILE_ATTRIBUTE_NORMAL),
        "Couldn't make " BOOT_INI_BACKUP " visible (it probably doesn't exist)");
    if (!CopyFile(bootIni, bootIniBackup, /* bFailIfExists */ FALSE)) {
        PRINT_ERROR_MSG_FMT("Backing up boot.ini with CopyFile(%ls, %ls, FALSE) failed",
            bootIni, bootIniBackup);
        return false;
    }

    IFFALSE_PRINTERROR(SetFileAttributes(bootIniBackup, unveilBootIni.originalAttributes()),
        "SetFileAttributes failed on backup");

    // According to https://support.microsoft.com/en-gb/help/102873 and other
    // resources, the key in boot.ini are supposed to use ARC-style notation
    // for disk labels -- of the form multi(w)disk(x)rdisk(y)partition(z) --
    // rather than DOS-style notation like C:. I have been unable to find a
    // sensible API to convert DOS notation, or even HarddiskYPartitionZ
    // notation, to ARC notation. On a single-disk system, at least, C: works
    // fine!
    //
    // http://stackoverflow.com/q/43895953/173314
    if (add) {
        const CString quotedName = L'"' + m_name + L'"';
        if (!WritePrivateProfileString(BOOT_INI_SECTION, eosldrMbrPath, quotedName, bootIni)) {
            PRINT_ERROR_MSG_FMT("WritePrivateProfileString(..., %ls, %ls, %ls) failed",
                eosldrMbrPath, quotedName, bootIni);
            return false;
        }
    } else {
        // If there's an existing entry for eosldr with a non-empty-string label,
        // consider the boot entry to be found.
        wchar_t dummy[2] = { 0 };
        DWORD len = GetPrivateProfileString(BOOT_INI_SECTION, eosldrMbrPath, L"", dummy, ARRAYSIZE(dummy), bootIni);
        foundBootEntry = (len > 0);

        // passing NULL for the third parameter removes the entry
        if (!WritePrivateProfileString(BOOT_INI_SECTION, eosldrMbrPath, NULL, bootIni)) {
            PRINT_ERROR_MSG_FMT("WritePrivateProfileString(..., %ls, NULL, %ls) failed",
                eosldrMbrPath, bootIni);
            return false;
        }
    }

    return true;
}

static HRESULT CreateGuid(CString &guidOut)
{
    GUID guid;

    IFFAILED_RETURN_RES(CoCreateGuid(&guid), "CoCreateGuid failed");

    wchar_t wszGuid[128] = { 0 };
    int ret = StringFromGUID2(guid, wszGuid, ARRAYSIZE(wszGuid));
    IFFALSE_RETURN_VALUE(ret, "StringFromGUID2 failed", E_FAIL);
    guidOut = wszGuid;
    return S_OK;
}

bool EosldrInstallerBcd::AddToBootOrder(const CString & systemDriveLetter, const CString & eosldrMbrPath)
{
    FUNCTION_ENTER_FMT("%ls, %ls", systemDriveLetter, eosldrMbrPath);

    CString guid;
    IFFAILED_RETURN_VALUE(CreateGuid(guid), "CreateGuid failed", false);

    CRegKey registryKey;
    LSTATUS result;

    result = registryKey.Open(HKEY_LOCAL_MACHINE, m_uninstallKey);
    IFFALSE_RETURN_VALUE(result == ERROR_SUCCESS, "Failed to open Uninstall key", false);

    IFFALSE_RETURN_VALUE(ERROR_SUCCESS == registryKey.SetStringValue(REGKEY_BCD_GUID, guid),
        "Failed to set EndlessBcdGuid", false);

    IFFALSE_RETURN_VALUE(
        WMI::AddBcdEntry(m_name, eosldrMbrPath, guid),
        "AddBcdEntry failed", false);

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
