#pragma once

#ifndef __AFXWIN_H__
    #error "include 'stdafx.h' before including this file for PCH"
#endif

class EosldrInstaller {
public:
    virtual ~EosldrInstaller() {}

    bool CanInstall(const CString &bootFilesPath);
    bool Install(
        const CString &systemDriveLetter,
        const CString &bootFilesPath);

    bool IsInstalled(const CString &systemDriveLetter);
    bool Uninstall(const CString &systemDriveLetter, bool &foundBootEntry);

    static EosldrInstaller *GetInstance(
        int windowsVersion,
        const CString &name,
        const CString &uninstallKey);

protected:
    EosldrInstaller(const CString &name)
        : m_name(name)
    {}

    virtual bool AddToBootOrder(
        const CString & systemDriveLetter,
        const CString & eosldrMbrPath);
    virtual bool RemoveFromBootOrder(
        const CString & systemDriveLetter,
        const CString & eosldrMbrPath,
        bool &foundBootEntry);

    // Boot entry name
    const CString m_name;
};

class EosldrInstallerBootIni : public EosldrInstaller {
public:
    EosldrInstallerBootIni(const CString &name)
        : EosldrInstaller(name)
    {}

protected:
    virtual bool AddToBootOrder(
        const CString & systemDriveLetter,
        const CString & eosldrMbrPath);
    virtual bool RemoveFromBootOrder(
        const CString & systemDriveLetter,
        const CString & eosldrMbrPath,
        bool &foundBootEntry);

private:
    bool ModifyBootOrder(
        const CString & systemDriveLetter,
        const CString & eosldrMbrPath,
        bool add,
        bool &foundBootEntry);
};

class EosldrInstallerBcd : public EosldrInstaller {
public:
    EosldrInstallerBcd(const CString &name, const CString &uninstallKey)
        : EosldrInstaller(name),
        m_uninstallKey(uninstallKey)
    {}

protected:
    virtual bool AddToBootOrder(
        const CString & systemDriveLetter,
        const CString & eosldrMbrPath);
    virtual bool RemoveFromBootOrder(
        const CString & systemDriveLetter,
        const CString & eosldrMbrPath,
        bool &foundBootEntry);

private:
    // Key in registry to stash boot entry GUID
    const CString m_uninstallKey;
};