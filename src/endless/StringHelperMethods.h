#ifndef __stringhelpermethods_header__
#define __stringhelpermethods_header__

#define GET_LOCAL_PATH(__filename__) (CEndlessUsbToolApp::m_appDir + L"\\" + (__filename__))
#define CSTRING_GET_LAST(__path__, __separator__) __path__.Right(__path__.GetLength() - __path__.ReverseFind(__separator__) - 1)
#define CSTRING_GET_PATH(__path__, __separator__) __path__.Left(__path__.ReverseFind(__separator__))

//class CComBSTR;
//class CString;
//class CStringA;
//class CStringW;

CComBSTR UTF8ToBSTR(const char *txt);

CString UTF8ToCString(const char *txt);

CStringA ConvertUnicodeToUTF8(const CStringW& uni);

#endif //__stringhelpermethods_header__