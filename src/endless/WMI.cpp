#include "stdafx.h"

#include <functional>

#include <Wbemidl.h>
#include <atlsafe.h>

#include "GeneralCode.h"
#include "StringHelperMethods.h"
#include "WMI.h"

#pragma comment(lib, "wbemuuid.lib")

static class COMThreading {
public:
    COMThreading() {
        // Regardless of return value, you must balance each call to
        // CoInitialize[Ex] with a call to CoUninitialize.
        HRESULT hres = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
        IFFAILED_PRINTERROR(hres, "Error on CoInitializeEx");
    }

    ~COMThreading() {
        CoUninitialize();
    }
};

static HRESULT GetWMIProxy(const CString &objectPath, CComPtr<IWbemServices> &pSvc)
{
    FUNCTION_ENTER;

    HRESULT hres;
    CComPtr<IWbemLocator> pLoc;

    hres = CoInitializeSecurity(
        NULL,
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities
        NULL                         // Reserved
    );
    IFFAILED_PRINTERROR(hres, "Error on CoInitializeSecurity.");

    // Obtain the initial locator to WMI
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);
    IFFAILED_RETURN_RES(hres, "Error on CoCreateInstance.");

    hres = pLoc->ConnectServer(
        _bstr_t(objectPath),     // Object path of WMI namespace
        NULL,                    // User name. NULL = current user
        NULL,                    // User password. NULL = current
        0,                       // Locale. NULL indicates current
        NULL,                    // Security flags.
        0,                       // Authority (for example, Kerberos)
        0,                       // Context object
        &pSvc                    // pointer to IWbemServices proxy
    );
    IFFAILED_RETURN_RES(hres, "Error on pLoc->ConnectServer.");

    // Set security levels on the proxy
    hres = CoSetProxyBlanket(
        pSvc,                        // Indicates the proxy to set
        RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
        RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
        NULL,                        // Server principal name
        RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
        RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
        NULL,                        // client identity
        EOAC_NONE                    // proxy capabilities
    );
    IFFAILED_RETURN_RES(hres, "Error on CoSetProxyBlanket.");

    return hres;
}

static BOOL RunWMIQuery(const CString &objectPath, const CString &query, std::function< BOOL( CComPtr<IEnumWbemClassObject> & )> callback)
{
    FUNCTION_ENTER_FMT("%ls \"%ls\"", objectPath, query);

    COMThreading t;
    HRESULT hres;
    CComPtr<IWbemServices> pSvc;
    CComPtr<IEnumWbemClassObject> pEnumerator;

    IFFAILED_RETURN_VALUE(GetWMIProxy(objectPath, pSvc), "Couldn't get WMI proxy", FALSE);

    // Use the IWbemServices pointer to make requests of WMI
    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(query), WBEM_FLAG_FORWARD_ONLY | WBEM_RETURN_WHEN_COMPLETE, NULL, &pEnumerator);
    IFFAILED_RETURN_VALUE(hres, "Error on pSvc->ExecQuery.", FALSE);

    return callback(pEnumerator);
}

BOOL WMI::IsBitlockedDrive(const CString & drive)
{
    FUNCTION_ENTER;

    const CString objectPath(L"ROOT\\CIMV2\\Security\\MicrosoftVolumeEncryption");
    CString bitlockerQuery;

    bitlockerQuery.Format(L"Select * from Win32_EncryptableVolume where DriveLetter = '%ls'", drive);

    BOOL retResult = RunWMIQuery(objectPath, bitlockerQuery, [](CComPtr<IEnumWbemClassObject> &pEnumerator) {
        CComPtr<IWbemClassObject> pclsObj;
        ULONG uReturned = 0;
        HRESULT hres = pEnumerator->Next(0, 1, &pclsObj, &uReturned);

        /* If we can't get the volume's properties, assume it is encrypted. */
        IFFAILED_RETURN_VALUE(hres, "Next() failed", TRUE);

        /* Machines with no Win32_EncryptableVolume objects are hopefully not
         * encrypted with BitLocker (Windows feature disabled / not installed?)
         * https://phabricator.endlessm.com/T22290
         */
        IFTRUE_RETURN_VALUE(uReturned == 0, "No matching Win32_EncryptableVolume found", FALSE);

        _variant_t vtProp;

        /* https://msdn.microsoft.com/en-us/library/windows/desktop/aa376433(v=vs.85).aspx
         * 0: FullyDecrypted. "For a standard hard drive (HDD), the volume is fully decrypted.
         * For a hardware encrypted hard drive (EHDD), the volume is perpetually unlocked."
         */
        IFFAILED_RETURN_VALUE(
            pclsObj->Get(L"ConversionStatus", 0, &vtProp, 0, 0),
            "Could not get ConversionStatus", TRUE);
        long conversionStatus = vtProp;

        /* https://msdn.microsoft.com/en-us/library/windows/desktop/aa376448(v=vs.85).aspx
         * "Protection is off if a volume is unencrypted or partially encrypted, or if the
         * volume's encryption key is available in the clear on the hard disk."
         * So though 0 is "Unprotected", this doesn't mean the disk is unencrypted.
         */
        IFFAILED_RETURN_VALUE(
            pclsObj->Get(L"ProtectionStatus", 0, &vtProp, 0, 0),
            "Could not get ProtectionStatus", TRUE);
        long protectionStatus = vtProp;

        uprintf("ConversionStatus = %ld, ProtectionStatus = %ld", conversionStatus, protectionStatus);
        /* Let's play it safe and check both. */
        return (conversionStatus != 0 || protectionStatus != 0) ? TRUE : FALSE;
    });

    uprintf("IsBitlockedDrive done with retResult=%d", retResult);
    return retResult;
}

BOOL WMI::GetMachineInfo(CString & manufacturer, CString & model)
{
    FUNCTION_ENTER;

    const CString objectPath(L"ROOT\\CIMV2");
    const CString query(L"Select * from Win32_ComputerSystem");

    return RunWMIQuery(objectPath, query, [&](CComPtr<IEnumWbemClassObject> &pEnumerator) {
        while (pEnumerator)
        {
            CComPtr<IWbemClassObject> pclsObj;
            ULONG uReturn = 0;
            HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
                &pclsObj, &uReturn);

            if (0 == uReturn)
            {
                break;
            }

            VARIANT vtProp;

            hr = pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);
            manufacturer = vtProp.bstrVal;
            VariantClear(&vtProp);

            hr = pclsObj->Get(L"Model", 0, &vtProp, 0, 0);
            model = vtProp.bstrVal;
            VariantClear(&vtProp);
        }

        return TRUE;
    });
}

#pragma region BCD

// http://www.geoffchappell.com/notes/windows/boot/bcd/objects.htm
#define BCD_APPLICATION_BOOTSECTOR 0x10400008

#define BCD_GUID_BOOTMGR L"{9dea862c-5cdd-4e70-acc1-f32b344d4795}"


// https://msdn.microsoft.com/en-us/library/aa362652.aspx
typedef enum BcdLibraryElementTypes {
    BcdLibraryDevice_ApplicationDevice = 0x11000001,
    BcdLibraryString_ApplicationPath = 0x12000002,
    BcdLibraryString_Description = 0x12000004,
    // ... other elements ...
};

// https://msdn.microsoft.com/en-us/library/windows/desktop/aa362641(v=vs.85).aspx
typedef enum BcdBootMgrElementTypes {
    BcdBootMgrObjectList_DisplayOrder = 0x24000001,
    BcdBootMgrObjectList_BootSequence = 0x24000002,
    BcdBootMgrObject_DefaultObject = 0x23000003,
    BcdBootMgrInteger_Timeout = 0x25000004,
    BcdBootMgrBoolean_AttemptResume = 0x26000005,
    BcdBootMgrObject_ResumeObject = 0x23000006,
    BcdBootMgrObjectList_ToolsDisplayOrder = 0x24000010,
    BcdBootMgrBoolean_DisplayBootMenu = 0x26000020,
    BcdBootMgrBoolean_NoErrorDisplay = 0x26000021,
    BcdBootMgrDevice_BcdDevice = 0x21000022,
    BcdBootMgrString_BcdFilePath = 0x22000023,
    BcdBootMgrBoolean_ProcessCustomActionsFirst = 0x26000028,
    BcdBootMgrIntegerList_CustomActionsList = 0x27000030,
    BcdBootMgrBoolean_PersistBootSequence = 0x26000031
} BcdBootMgrElementTypes;


// Inspired by https://github.com/sysprogs/BazisLib/blob/master/bzshlp/Win32/BCD.cpp and
// https://social.msdn.microsoft.com/Forums/sqlserver/en-US/a9996e4a-d2d7-4c42-87d2-48096ea47eb5/wmi-bcdobjects-using-c?forum=windowsgeneraldevelopmentissues#cb08edd9-297b-453d-a7ca-6c96574aee3f
// -- thanks, both.
class WmiObject {
public:
    WmiObject(IWbemServices *pSvc, IWbemClassObject *pClass, IWbemClassObject *pInstance) :
        m_pSvc(pSvc),
        m_pClass(pClass),
        m_pInstance(pInstance)
    {
    }

    HRESULT ExecMethod(
        LPCWSTR wszMethodName,
        IWbemClassObject **ppOutParams = NULL,
        LPCWSTR wszParam1Name = NULL, const CComVariant &cvParam1 = CComVariant(),
        LPCWSTR wszParam2Name = NULL, const CComVariant &cvParam2 = CComVariant(),
        LPCWSTR wszParam3Name = NULL, const CComVariant &cvParam3 = CComVariant(),
        LPCWSTR wszParam4Name = NULL, const CComVariant &cvParam4 = CComVariant());

protected:
    static HRESULT EnumerateProperties(IWbemClassObject *pObject);
    static HRESULT Get(IWbemClassObject *pObject, LPCWSTR wszName, VARIANT *vParam);
    static HRESULT Put(IWbemClassObject *pObject, LPCWSTR wszName, const VARIANT &cvParam);

    CComPtr<IWbemServices> m_pSvc;
    // Methods are looked up on this
    CComPtr<IWbemClassObject> m_pClass;
    // Methods are called on this -- may equal m_pClass
    CComPtr<IWbemClassObject> m_pInstance;
};

class BcdBootmgr : public WmiObject {
public:
    BcdBootmgr() : WmiObject(NULL, NULL, NULL) {}
    BcdBootmgr(IWbemServices *pSvc, IWbemClassObject *pClass, IWbemClassObject *pInstance)
        : WmiObject(pSvc, pClass, pInstance) {}

    HRESULT GetDisplayOrder(CComSafeArray<BSTR> &displayOrder);
    HRESULT SetDisplayOrder(CComSafeArray<BSTR> &displayOrder);
    HRESULT ExtendDisplayOrder(const CString &guid);
    HRESULT RemoveFromDisplayOrder(const CString &guid);

    HRESULT GetTimeout(ULONGLONG &timeout);
    HRESULT SetTimeout(const int32_t timeout);
    HRESULT EnsureTimeout(const int32_t timeout);
};

class BcdBootSectorApplication : public WmiObject {
public:
    BcdBootSectorApplication() : WmiObject(NULL, NULL, NULL) {}
    BcdBootSectorApplication(IWbemServices *pSvc, IWbemClassObject *pClass, IWbemClassObject *pInstance)
        : WmiObject(pSvc, pClass, pInstance) {}

    HRESULT SetDescription(const CString &description) {
        return ExecMethod(L"SetStringElement", /* ppOutParams */ NULL,
            L"Type", static_cast<int32_t>(BcdLibraryString_Description),
            L"String", CComVariant(description));
    }

    HRESULT SetPartition(const wchar_t *partitionPath) {
        return ExecMethod(L"SetPartitionDeviceElement", /* ppOutParams */ NULL,
            L"Type", static_cast<int32_t>(BcdLibraryDevice_ApplicationDevice),
            L"DeviceType", static_cast<int32_t>(2), // type partition
            L"AdditionalOptions", L"",
            L"Path", partitionPath);
    }

    HRESULT SetApplicationPath(const CString &mbrPathWithoutDrive) {
        return ExecMethod(L"SetStringElement", /* ppOutParams */ NULL,
            L"Type", static_cast<int32_t>(BcdLibraryString_ApplicationPath),
            L"String", CComVariant(mbrPathWithoutDrive));
    }
};

class BcdStore : public WmiObject {
public:
    BcdStore() : WmiObject(NULL, NULL, NULL) {}
    BcdStore(IWbemServices *pSvc, IWbemClassObject *pClass, IWbemClassObject *pInstance)
        : WmiObject(pSvc, pClass, pInstance) {}

    static HRESULT OpenStore(BcdStore *bcdStore);
    HRESULT DeleteObject(const CString &guid);

    HRESULT CreateBootSectorApplication(const CString &guid, BcdBootSectorApplication *bcdObject) {
        return CreateObject<BcdBootSectorApplication>(guid, BCD_APPLICATION_BOOTSECTOR, bcdObject);
    }

    HRESULT GetBootmgr(BcdBootmgr *bootmgr) {
        return GetObject<BcdBootmgr>(BCD_GUID_BOOTMGR, bootmgr);
    }

private:
    template <typename T>
    HRESULT CreateObject(const CString &guid, int32_t type, T *bcdObject);

    template <typename T>
    HRESULT GetObject(const CString &guid, T *bcdObject);
};

HRESULT WmiObject::EnumerateProperties(IWbemClassObject * pObject)
{
    BSTR name;
    VARIANT val;
    CIMTYPE type;
    LONG flavor;

    IFFAILED_RETURN_RES(pObject->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY), "BeginEnumeration");

    uprintf("Properties:");
    while (pObject->Next(0, &name, &val, &type, &flavor) != WBEM_S_NO_MORE_DATA) {
        uprintf("%-20ls type=0x%04x val.vt=0x%04x", name, type, val.vt);
        SysFreeString(name);
        VariantClear(&val);
    }

    return S_OK;
}

HRESULT WmiObject::Get(IWbemClassObject * pObject, LPCWSTR wszName, VARIANT *vParam)
{
    HRESULT hres = S_OK;

    if (wszName != NULL) {
        CIMTYPE ctype;
        LONG flavor;
        hres = pObject->Get(wszName, 0, vParam, &ctype, &flavor);
        if (FAILED(hres)) {
            CString message;
            message.Format(L"Get(\"%ls\") failed", wszName);
            PRINT_HRESULT(hres, ConvertUnicodeToUTF8(message));
            EnumerateProperties(pObject);
        }
    }

    return hres;
}

HRESULT WmiObject::Put(IWbemClassObject * pObject, LPCWSTR wszName, const VARIANT & cvParam)
{
    HRESULT hres = S_OK;

    if (wszName != NULL) {
        VARIANT vParam = cvParam;
        hres = pObject->Put(wszName, 0, &vParam, 0);
        if (FAILED(hres)) {
            CString message;
            message.Format(L"Put(\"%ls\", variant type 0x%x) failed", wszName, vParam.vt);
            PRINT_HRESULT(hres, ConvertUnicodeToUTF8(message));
            EnumerateProperties(pObject);
        }
    }

    return hres;
}

HRESULT WmiObject::ExecMethod(
    LPCWSTR wszMethodName,
    IWbemClassObject **ppOutParams,
    LPCWSTR wszParam1Name, const CComVariant &cvParam1,
    LPCWSTR wszParam2Name, const CComVariant &cvParam2,
    LPCWSTR wszParam3Name, const CComVariant &cvParam3,
    LPCWSTR wszParam4Name, const CComVariant &cvParam4)
{
    FUNCTION_ENTER_FMT("%ls", wszMethodName);

    HRESULT hres;

    // get the path of the instance
    VARIANT bcdStorePath;
    IFFAILED_RETURN_RES(Get(m_pInstance, L"__RELPATH", &bcdStorePath), "Failed to get path");

    // Get the parameter specification
    CComPtr<IWbemClassObject> pInParamsDefinition;
    WBEM_E_TYPE_MISMATCH;
    hres = m_pClass->GetMethod(wszMethodName, 0, &pInParamsDefinition, NULL);
    IFFAILED_RETURN_RES(hres, "GetMethod failed");

    CComPtr<IWbemClassObject> pInParams;
    if (pInParamsDefinition == NULL) {
        // Method accepts no parameters
        IFFALSE_RETURN_VALUE(wszParam1Name == NULL, "Expected method to accept >=1 parameters", E_FAIL);
        IFFALSE_RETURN_VALUE(wszParam2Name == NULL, "Expected method to accept >=2 parameters", E_FAIL);
        IFFALSE_RETURN_VALUE(wszParam3Name == NULL, "Expected method to accept >=3 parameters", E_FAIL);
        IFFALSE_RETURN_VALUE(wszParam4Name == NULL, "Expected method to accept >=4 parameters", E_FAIL);
    } else {
        // spawn instance of parameter structure
        hres = pInParamsDefinition->SpawnInstance(0, &pInParams);
        IFFAILED_RETURN_RES(hres, "SpawnInstance failed");

        // Create the values for the in parameters
        IFFAILED_RETURN_RES(Put(pInParams, wszParam1Name, cvParam1), "Setting parameter 1 failed");
        IFFAILED_RETURN_RES(Put(pInParams, wszParam2Name, cvParam2), "Setting parameter 2 failed");
        IFFAILED_RETURN_RES(Put(pInParams, wszParam3Name, cvParam3), "Setting parameter 3 failed");
        IFFAILED_RETURN_RES(Put(pInParams, wszParam4Name, cvParam4), "Setting parameter 4 failed");
    }

    // execute the method
    hres = m_pSvc->ExecMethod(bcdStorePath.bstrVal, _bstr_t(wszMethodName), 0, NULL,
        pInParams, ppOutParams, NULL);
    IFFAILED_RETURN_RES(hres, "Failed to execute method");

    return S_OK;
}

HRESULT BcdStore::OpenStore(BcdStore *bcdStore)
{
    CComPtr<IWbemServices> pSvc;
    CComPtr<IWbemClassObject> pBcdStoreClass;
    CComPtr<IWbemClassObject> pBcdObjectClass;

    IFFAILED_RETURN_RES(GetWMIProxy(L"root\\wmi", pSvc), "Couldn't get WMI proxy");

    // Fetch class definition
    HRESULT hres = pSvc->GetObject(_bstr_t(L"BCDStore"), 0, NULL, &pBcdStoreClass, NULL);
    IFFAILED_RETURN_RES(hres, "Could not get BCDStore class");

    WmiObject bcdStoreClass(pSvc, pBcdStoreClass, pBcdStoreClass);

    // BCDStore.OpenStore(File=""), returning (Store=vStore)
    CComPtr<IWbemClassObject> pOutParams;
    IFFAILED_RETURN_RES(
        bcdStoreClass.ExecMethod(L"OpenStore", &pOutParams,
            L"File", L""),
        "OpenStore failed");

    CComVariant vStore;
    IFFAILED_RETURN_RES(Get(pOutParams, L"Store", &vStore), "Getting Store out param failed");

    CComPtr<IWbemClassObject> pBcdStore = (IWbemClassObject*)vStore.byref;
    *bcdStore = BcdStore(pSvc, pBcdStoreClass, pBcdStore);
    return S_OK;
}

template <typename T>
HRESULT BcdStore::CreateObject(const CString & guid, int32_t type, T * bcdObject)
{
    FUNCTION_ENTER_FMT("%ls", guid);
    CComPtr<IWbemClassObject> pBcdObjectClass;

    IFFAILED_RETURN_RES(
        m_pSvc->GetObject(_bstr_t(L"BCDObject"), 0, NULL, &pBcdObjectClass, NULL),
        "Could not get BCDObject class");

    CComPtr<IWbemClassObject> pOutParams;
    IFFAILED_RETURN_RES(
        ExecMethod(L"CreateObject", &pOutParams,
            L"Id", CComVariant(guid),
            // If you enumerate this method's parameters, Type is allegedly a CIM_UINT32 == VT_UI4 == 19 == 0x13.
            // However, if you supply a variant of this type, you will receive WBEM_E_TYPE_MISMATCH.
            // Supplying a variant of type VT_I4, on the other hand, works...
            L"Type", type),
        "CreateObject failed");

    CComVariant vBcdObject;
    IFFAILED_RETURN_RES(Get(pOutParams, L"Object", &vBcdObject), "Getting Object out param failed");

    CComPtr<IWbemClassObject> pBcdObject = (IWbemClassObject*)vBcdObject.byref;
    *bcdObject = T(m_pSvc, pBcdObjectClass, pBcdObject);

    return S_OK;
}

template <typename T>
HRESULT BcdStore::GetObject(const CString & guid, T * bcdObject)
{
    CComPtr<IWbemClassObject> pBcdObjectClass;

    IFFAILED_RETURN_RES(
        m_pSvc->GetObject(_bstr_t(L"BCDObject"), 0, NULL, &pBcdObjectClass, NULL),
        "Could not get BCDObject class");

    CComPtr<IWbemClassObject> pOutParams;
    IFFAILED_RETURN_RES(
        ExecMethod(L"OpenObject", &pOutParams,
            // In parameters:
            L"Id", CComVariant(guid)),
        "OpenObject failed");

    CComVariant vBcdObject;
    IFFAILED_RETURN_RES(Get(pOutParams, L"Object", &vBcdObject), "Getting Object out param failed");

    CComPtr<IWbemClassObject> pBcdObject = (IWbemClassObject*)vBcdObject.byref;
    *bcdObject = T(m_pSvc, pBcdObjectClass, pBcdObject);

    return S_OK;
}

HRESULT BcdStore::DeleteObject(const CString & guid)
{
    FUNCTION_ENTER_FMT("%ls", guid);

    return ExecMethod(L"DeleteObject", /* ppOutParams */ NULL,
        L"Id", CComVariant(guid));
}

HRESULT BcdBootmgr::GetDisplayOrder(CComSafeArray<BSTR>& displayOrder)
{
    const int32_t type = BcdBootMgrObjectList_DisplayOrder;

    /* Fetch current DisplayOrder */
    CComPtr<IWbemClassObject> pOutParams;
    IFFAILED_RETURN_RES(
        ExecMethod(L"GetElement", &pOutParams,
            L"Type", type),
        "Can't get current DisplayOrder");

    CComVariant vDisplayOrderElement;
    IFFAILED_RETURN_RES(Get(pOutParams, L"Element", &vDisplayOrderElement), "Getting Element out param failed");

    CComPtr<IWbemClassObject> pDisplayOrderElement = (IWbemClassObject*)vDisplayOrderElement.byref;
    CComVariant vDisplayOrder;
    IFFAILED_RETURN_RES(
        WmiObject::Get(pDisplayOrderElement, L"Ids", &vDisplayOrder),
        "Failed to get Ids property from Element");

    displayOrder = vDisplayOrder.parray;
    return S_OK;
}

HRESULT BcdBootmgr::SetDisplayOrder(CComSafeArray<BSTR>& displayOrder)
{
    const int32_t type = BcdBootMgrObjectList_DisplayOrder;

    return ExecMethod(L"SetObjectListElement", /* ppOutParams */ NULL,
        L"Type", type,
        L"Ids", CComVariant(displayOrder.m_psa));
}

HRESULT BcdBootmgr::ExtendDisplayOrder(const CString &guid)
{
    FUNCTION_ENTER_FMT("%ls", guid);

    CComSafeArray<BSTR> displayOrder;

    IFFAILED_RETURN_RES(GetDisplayOrder(displayOrder), "Can't get DisplayOrder");
    IFFAILED_RETURN_RES(displayOrder.Add(_bstr_t(guid)), "Can't append GUID to DisplayOrder");
    IFFAILED_RETURN_RES(SetDisplayOrder(displayOrder), "Can't set DisplayOrder");

    return S_OK;
}

HRESULT BcdBootmgr::RemoveFromDisplayOrder(const CString &guid)
{
    FUNCTION_ENTER_FMT("%ls", guid);

    CComSafeArray<BSTR> displayOrder;

    IFFAILED_RETURN_RES(GetDisplayOrder(displayOrder), "Can't get DisplayOrder");

    CComSafeArray<BSTR> newDisplayOrder; // = filter (!= guid) displayOrder
    bool found = false;
    // GetCount() returns ULONG but [] and GetAt() take signed LONG...
    ULONG n_ = displayOrder.GetCount();
    IFFALSE_RETURN_VALUE(n_ <= LONG_MAX, "Do you really have >= 2**31 boot entries?", E_FAIL);
    LONG n = static_cast<LONG>(n_);
    for (LONG i = 0; i < n; i++) {
        BSTR g = displayOrder[i];
        if (guid.CompareNoCase(g) == 0) {
            found = true;
        } else {
            newDisplayOrder.Add(g);
        }
    }
    IFFALSE_RETURN_VALUE(found, "Entry not found in DisplayOrder", E_FAIL);

    IFFAILED_RETURN_RES(SetDisplayOrder(newDisplayOrder), "Can't set DisplayOrder");

    return S_OK;
}

HRESULT BcdBootmgr::GetTimeout(ULONGLONG & timeout)
{
    FUNCTION_ENTER;
    const int32_t type = BcdBootMgrInteger_Timeout;

    CComPtr<IWbemClassObject> pOutParams;
    IFFAILED_RETURN_RES(
        ExecMethod(L"GetElement", &pOutParams,
            L"Type", type),
        "Can't get current Timeout");

    CComVariant vTimeoutElement;
    IFFAILED_RETURN_RES(Get(pOutParams, L"Element", &vTimeoutElement), "Getting Element out param failed");

    CComPtr<IWbemClassObject> pElement = (IWbemClassObject*)vTimeoutElement.byref;
    CComVariant vTimeout;
    IFFAILED_RETURN_RES(
        WmiObject::Get(pElement, L"Integer", &vTimeout),
        "Getting Integer from Element failed");

    // Although this value is conceptually an uint64, it's passed as a string.
    // https://msdn.microsoft.com/en-us/library/windows/desktop/aa362650(v=vs.85).aspx
    wchar_t *wszTimeout = vTimeout.bstrVal;
    wchar_t *end = NULL;
    timeout = wcstoull(wszTimeout, &end, 10);

    if (*end != 0) {
        PRINT_ERROR_MSG_FMT("Failed to parse Timeout value \"%ls\"", wszTimeout);
        return E_FAIL;
    }

    return S_OK;
}

// https://msdn.microsoft.com/en-us/library/windows/desktop/aa362696(v=vs.85).aspx
// documents the Integer parameter to be of type UI8 but only I4 works in practice.
HRESULT BcdBootmgr::SetTimeout(const int32_t timeout)
{
    FUNCTION_ENTER_FMT("%d", timeout);
    const int32_t type = BcdBootMgrInteger_Timeout;

    return ExecMethod(L"SetIntegerElement", NULL,
        L"Type", type,
        L"Integer", timeout);
}

HRESULT BcdBootmgr::EnsureTimeout(const int32_t timeout)
{
    FUNCTION_ENTER_FMT("%d", timeout);

    ULONGLONG currentTimeout;
    IFFAILED_RETURN_RES(GetTimeout(currentTimeout), "Failed to get timeout");

    if (currentTimeout < timeout) {
        IFFAILED_RETURN_RES(SetTimeout(timeout), "Failed to set timeout");
    }

    return S_OK;
}

BOOL WMI::AddBcdEntry(const CString & name, const CString & mbrPath, const CString & guid)
{
    FUNCTION_ENTER_FMT("%ls %ls %ls", name, mbrPath, guid);

    const CString mbrDriveLetter = mbrPath.Left(2);
    const CString mbrPathWithoutDrive = mbrPath.Mid(2);

    COMThreading t;

    wchar_t wszPartitionPath[MAX_PATH + 1] = { 0 };
    IFFALSE_RETURN_VALUE(
        QueryDosDevice(mbrDriveLetter, wszPartitionPath, ARRAYSIZE(wszPartitionPath)),
        "Can't get device path for mbrPath drive", FALSE);

    BcdStore bcdStore;
    IFFAILED_RETURN_VALUE(
        BcdStore::OpenStore(&bcdStore),
        "Can't get BcdStore", FALSE);

    BcdBootmgr bootmgr;
    IFFAILED_RETURN_VALUE(
        bcdStore.GetBootmgr(&bootmgr),
        "Can't get {bootmgr}", FALSE);

    // Ensure a non-zero timeout. The "upstream" default is 30 but 0 has been
    // observed in the wild. Equivalent to running bcdedit, checking the
    // timeout, and if it's less than 30:
    // > bcdedit /timeout 30
    IFFAILED_RETURN_VALUE(
        bootmgr.EnsureTimeout(30),
        "Can't set boot menu timeout to at least 30 seconds", FALSE);

    // Roughly equivalent to:
    // > bcdedit /create /d $name /application bootsector
    // which returns a GUID
    BcdBootSectorApplication bcdObject;
    IFFAILED_RETURN_VALUE(
        bcdStore.CreateBootSectorApplication(guid, &bcdObject),
        "Can't create new boot entry", FALSE);

    IFFAILED_RETURN_VALUE(
        bcdObject.SetDescription(name),
        "SetDescription failed", FALSE);

    // Roughly equivalent to:
    // > bcdedit /set $guid path $mbrPathWithoutDrive
    IFFAILED_RETURN_VALUE(
        bcdObject.SetApplicationPath(mbrPathWithoutDrive),
        "SetStringElement failed", FALSE);

    // Roughly equivalent to:
    // > bcdedit /set $guid device $wszPartitionPath
    IFFAILED_RETURN_VALUE(
        bcdObject.SetPartition(wszPartitionPath),
        "SetPartitionDeviceElement failed", FALSE);

    // Roughly equivalent to:
    // > bcdedit /displayorder $guid /addlast
    IFFAILED_RETURN_VALUE(
        bootmgr.ExtendDisplayOrder(guid),
        "Can't add to DisplayOrder", FALSE);

    return TRUE;
}

BOOL WMI::RemoveBcdEntry(const CString & guid, bool &foundBootEntry)
{
    FUNCTION_ENTER_FMT("%ls", guid);

    COMThreading t;

    BcdStore bcdStore;
    IFFAILED_RETURN_VALUE(
        BcdStore::OpenStore(&bcdStore),
        "Can't get BcdStore", FALSE);

    BcdBootmgr bootmgr;
    IFFAILED_RETURN_VALUE(
        bcdStore.GetBootmgr(&bootmgr),
        "Can't get {bootmgr}", FALSE);

    HRESULT hr = bootmgr.RemoveFromDisplayOrder(guid);
    if (FAILED(hr)) {
        PRINT_HRESULT(hr, "Can't remove from DisplayOrder (already removed?)");
        foundBootEntry = false;
    } else {
        foundBootEntry = true;
    }

    IFFAILED_PRINTERROR(
        bcdStore.DeleteObject(guid),
        "DeleteObject failed (already deleted?)");

    return TRUE;
}

#pragma endregion BCD
