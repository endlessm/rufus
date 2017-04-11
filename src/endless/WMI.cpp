#include "stdafx.h"

#include <functional>

#include <Wbemidl.h>

#include "GeneralCode.h"
#include "WMI.h"

#pragma comment(lib, "wbemuuid.lib")

static BOOL RunWMIQuery(const CString &objectPath, const CString &query, std::function< BOOL( CComPtr<IEnumWbemClassObject> & )> callback)
{
    FUNCTION_ENTER;

    HRESULT hres;
    BOOL retResult = FALSE;
    CComPtr<IWbemLocator> pLoc;
    CComPtr<IWbemServices> pSvc;
    CComPtr<IEnumWbemClassObject> pEnumerator;
    CString bitlockerQuery;

    hres = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    IFFALSE_PRINTERROR(SUCCEEDED(hres), "Error on CoInitializeEx.");

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
    IFFALSE_PRINTERROR(SUCCEEDED(hres), "Error on CoInitializeSecurity.");

    // Obtain the initial locator to WMI
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);
    IFFALSE_GOTOERROR(SUCCEEDED(hres), "Error on CoCreateInstance.");

    // Connect to WMI through the IWbemLocator::ConnectServer method
    // Connect to the root\cimv2 namespace with the current user and obtain pointer pSvc to make IWbemServices calls.
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
    IFFALSE_GOTOERROR(SUCCEEDED(hres), "Error on pLoc->ConnectServer.");

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
    IFFALSE_GOTOERROR(SUCCEEDED(hres), "Error on CoSetProxyBlanket.");

    // Use the IWbemServices pointer to make requests of WMI
    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(query), WBEM_FLAG_FORWARD_ONLY | WBEM_RETURN_WHEN_COMPLETE, NULL, &pEnumerator);
    IFFALSE_GOTOERROR(SUCCEEDED(hres), "Error on pSvc->ExecQuery.");

    retResult = callback(pEnumerator);

error:
    CoUninitialize();

    return retResult;
}

BOOL WMI::IsBitlockedDrive(const CString & drive)
{
    FUNCTION_ENTER;

    const CString objectPath(L"ROOT\\CIMV2\\Security\\MicrosoftVolumeEncryption");
    CString bitlockerQuery;

    bitlockerQuery.Format(L"Select * from Win32_EncryptableVolume where ProtectionStatus != 0 and DriveLetter = '%ls'", drive);

    BOOL retResult = RunWMIQuery(objectPath, bitlockerQuery, [](CComPtr<IEnumWbemClassObject> &pEnumerator) {
        HRESULT hres;
        CComPtr<IWbemClassObject> pclsObj;
        ULONG uReturned = 0;

        hres = pEnumerator->Next(0, 1, &pclsObj, &uReturned);
        uprintf("IsBitlockedDrive: hres=%x and uReturned=%d", hres, uReturned);
        return (hres == S_OK) && (uReturned == 1);
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
