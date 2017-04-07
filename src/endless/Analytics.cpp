#include "stdafx.h"
#include "Analytics.h"
#include <afxinet.h>
#include <memory>

#include "json/json.h"

#include "Version.h"
#include "GeneralCode.h"
#include "StringHelperMethods.h"

#define TRACKING_ID "UA-61784217-3"
#define REG_SECTION "Analytics"
#define REG_KEY_CLIENT_ID "ClientID"
#define REG_KEY_DISABLED "Disabled"
#define REG_KEY_DEBUG "Debug"

#define SERVER_NAME "www.google-analytics.com"
#define SERVER_PORT 443
#define SERVER_PATH _T("collect")
#define SERVER_DEBUG_PATH _T("debug/collect")

class AnalyticsRequestThread
	: public CWinThread
{
public:
	AnalyticsRequestThread(BOOL debug)
		: m_debug(debug)
		, m_session(GetUserAgent())
	{}
	virtual int Run();
	virtual BOOL InitInstance() { return TRUE;  }

private:
	static void HandleDebugResponse(CHttpFile *file);
	static CString GetUserAgent();

	void SendRequest(const CStringA &bodyUtf8);

	BOOL m_debug;
	CInternetSession m_session;
};

void AnalyticsRequestThread::HandleDebugResponse(CHttpFile *file)
{
	std::string response;
	char chunk[1024];
	UINT n;
	while (0 < (n = file->Read(chunk, _countof(chunk)))) {
		response.append(chunk, n);
		break;
	}
	// uprintf("Analytics: response: %s", response.c_str());

	Json::Reader reader;
	Json::Value root;
	if (reader.parse(response, root)) {
		Json::Value hpr = root["hitParsingResult"];
		for (Json::ArrayIndex i = 0; i < hpr.size(); i++) {
			Json::Value result = hpr[i];
			if (!result["valid"].asBool()) {
				uprintf("Analytics: submitted an invalid hit: %s", response.c_str());
				break;
			}
		}
	}
	else {
		uprintf("Analytics: failed to parse debug result: %s", response.c_str());
	}
}

// We pretend to be sending requests with the UA used by the browser widget.
// In particular, this includes the OS version. Here is an example user agent
// from Windows 10 with IE 11 installed:
//
//    Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 10.0; WOW64; Trident/7.0; .NET4.0C; .NET4.0E)
//
// https://www.whatismybrowser.com/developers/tools/user-agent-parser/
// understands this to be IE 11 running in IE 7 Compatibility View mode.  If
// you hack the UI to go to https://www.whatismybrowser.com/ this does actually
// match the UA seen there.
//
// We send a separate IEVersion event with the *real* IE version; the UA is used
// mainly for the OS information.
CString AnalyticsRequestThread::GetUserAgent()
{
	FUNCTION_ENTER;

	DWORD uaSize = 0;
	char *ua = NULL;
	HRESULT hr;
	CString ret(_T("Endless Installer"));

	char dummy[1] = "";
	hr = ObtainUserAgentString(0, dummy, &uaSize);
	IFFALSE_GOTOERROR(hr == E_OUTOFMEMORY, "Expected ObtainUserAgentString to fail");
	IFFALSE_GOTOERROR(uaSize > 0, "Expected ObtainUserAgentString to return non-zero size");

	ua = (char *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, uaSize);
	hr = ObtainUserAgentString(0, ua, &uaSize);
	IFFALSE_GOTOERROR(hr == S_OK, "ObtainUserAgentString failed");

	ret = UTF8ToCString(ua);
	uprintf("User agent: '%ls'", ret);

error:
	if (ua != NULL) {
		HeapFree(GetProcessHeap(), 0, ua);
		ua = NULL;
	}
	return ret;
}

// In debug builds, CHttpConnection and CHttpFile print a warning (in debug
// builds) from the destructor if they've not been explicitly Close()d.
template <class T>
class CloseDelete
{
public:
	void operator()(T *x)
	{
		x->Close();
		delete x;
	}
};

template <class T>
using close_ptr = std::unique_ptr<T, CloseDelete<T> >;

void AnalyticsRequestThread::SendRequest(const CStringA &bodyUtf8)
{
	try {
		CString headers = _T("Content-type: application/x-www-form-urlencoded");
		close_ptr<CHttpConnection> conn(
			m_session.GetHttpConnection(_T(SERVER_NAME), (INTERNET_PORT)SERVER_PORT));
		CString path = m_debug ? SERVER_DEBUG_PATH : SERVER_PATH;
		const DWORD flags = INTERNET_FLAG_EXISTING_CONNECT
			| INTERNET_FLAG_KEEP_CONNECTION
			| INTERNET_FLAG_SECURE
			| INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP
			| INTERNET_FLAG_NO_COOKIES
			| INTERNET_FLAG_NO_UI;
		close_ptr<CHttpFile> file(
			conn->OpenRequest(CHttpConnection::HTTP_VERB_POST, path,
				/* Referer */ NULL, /* context */ 1, /* Accept */ NULL,
				_T("HTTP/1.1"), flags));
		if (file) {
			file->SendRequest(headers, (LPVOID)(LPCSTR)bodyUtf8, bodyUtf8.GetLength());
			uprintf("Analytics req: %s\n", (LPCSTR)bodyUtf8);
			DWORD responseCode = 0;
			IFFALSE_PRINTERROR(file->QueryInfoStatusCode(responseCode), "QueryInfoStatusCode failed");
			uprintf("Analytics: response code %d", responseCode);
			if (m_debug) {
				HandleDebugResponse(file.get());
			}
		}
		else {
			uprintf("Analytics req failed\n");
		}
	}
	catch (CInternetException *ex) {
		TCHAR err[256];
		ex->GetErrorMessage(err, 256);
		uprintf("Analytics req error: %ls\n", (LPCTSTR)err);
		ex->Delete();
	}
}

int AnalyticsRequestThread::Run()
{
	FUNCTION_ENTER;

	BOOL bRet;
	MSG msg;

	while (0 != (bRet = GetMessage(&msg, NULL, WM_APP, WM_APP+1))) {
		IFFALSE_RETURN_VALUE(bRet != -1, "GetMessage failed", 0);

		CString *pBody = (CString *)msg.wParam;
		CStringA bodyUtf8 = CW2A(*pBody);

		SendRequest(bodyUtf8);

		delete pBody;

		if (msg.message == WM_APP+1)
			break;
	}

	return 0;
}


Analytics::Analytics()
{
	m_disabled = disabled();
	if (m_disabled) return;
	FUNCTION_ENTER;
	m_trackingId = CString(_T(TRACKING_ID));
	loadUuid(m_clientId);
	m_language = "en-US";
	urlEncode(CString(WindowsVersionStr), m_windowsVersion);

	m_workerThread = new AnalyticsRequestThread(debug());
	m_workerThread->CreateThread();
}

Analytics::~Analytics()
{
}

Analytics *Analytics::instance()
{
	static Analytics instance;
	return &instance;
}

void Analytics::startSession()
{
	if (m_disabled) return;
	FUNCTION_ENTER;
	CString body;
	prefixId(body);
	body += _T("t=screenview&cd=DualBootInstallPage&sc=start");
	sendRequest(body);
}

HANDLE Analytics::stopSession(const CString &category)
{
	if (m_disabled) return INVALID_HANDLE_VALUE;
	FUNCTION_ENTER;
	CString body, catEnc;
	prefixId(body);
	urlEncode(category, catEnc);
	body.AppendFormat(_T("t=event&ec=%s&ea=Closed&sc=end"), catEnc);
	sendRequest(body, TRUE);
	return m_workerThread->m_hThread;
}

void Analytics::screenTracking(const CString &name)
{
	if (m_disabled) return;
	FUNCTION_ENTER;
	CString body, nameEnc;
	prefixId(body);
	urlEncode(name, nameEnc);
	body.AppendFormat(_T("t=screenview&cd=%s"), nameEnc);
	sendRequest(body);	
}

void Analytics::eventTracking(const CString &category, const CString &action, const CString &label, LONGLONG value)
{	
	if (m_disabled) return;
	FUNCTION_ENTER;
	CString body, catEnc, actEnc;
	prefixId(body);
	urlEncode(category, catEnc);
	urlEncode(action, actEnc);
	body.AppendFormat(_T("t=event&ec=%s&ea=%s"), catEnc, actEnc);
	if (!label.IsEmpty()) {
		CString labelEnc;
		urlEncode(label, labelEnc);
		body.AppendFormat(_T("&el=%s"), labelEnc);
	}
	if (value >= 0) {
		body.AppendFormat(_T("&ev=%I64d"), value);
	}
	sendRequest(body);
}

void Analytics::exceptionTracking(const CString &description, BOOL fatal)
{
	if (m_disabled) return;
	FUNCTION_ENTER;
	CString body, descEnc;
	prefixId(body);
	urlEncode(description, descEnc);
	body.AppendFormat(_T("t=exception&exd=%s&exf=%d"), descEnc, fatal);
	sendRequest(body);
}

void Analytics::setLanguage(const CString &language)
{
	m_language = language;
}

void Analytics::setManufacturerModel(const CString &manufacturer, const CString &model)
{
	urlEncode(manufacturer, m_manufacturer);
	urlEncode(model, m_model);
}

void Analytics::setFirmware(const CString &firmware)
{
	urlEncode(firmware, m_firmware);
}

void Analytics::sendRequest(const CString &body, BOOL lastRequest)
{
	FUNCTION_ENTER;
	CString *pBody = new CString(body);
	//CWinThread *pThread = AfxBeginThread(threadSendRequest, pBody);
	UINT msgType = WM_APP;
	if (lastRequest) msgType = WM_APP+1;
	m_workerThread->PostThreadMessage(msgType, (WPARAM)pBody, 0);
}

BOOL Analytics::debug()
{
	CWinApp *pApp = AfxGetApp();
	int debug = pApp->GetProfileInt(_T(REG_SECTION), _T(REG_KEY_DEBUG), 0);
	return (debug == 1);
}

BOOL Analytics::disabled()
{
	CWinApp *pApp = AfxGetApp();
	int disabled = pApp->GetProfileInt(_T(REG_SECTION), _T(REG_KEY_DISABLED), 0);
	return (disabled == 1);
}

void Analytics::loadUuid(CString &uuid)
{
	CWinApp *pApp = AfxGetApp();
	CString strUuid = pApp->GetProfileString(_T(REG_SECTION), _T(REG_KEY_CLIENT_ID));

	if (strUuid.IsEmpty()) {
		createUuid(strUuid);
		pApp->WriteProfileString(_T(REG_SECTION), _T(REG_KEY_CLIENT_ID), strUuid);
	}

	uuid = strUuid;
}

void Analytics::createUuid(CString &uuid)
{
	HRESULT hr;
	GUID guid;
	hr = CoCreateGuid(&guid);
	if (hr == S_OK)
		uuid.Format(_T("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"), guid.Data1, guid.Data2, guid.Data3, guid.Data4[0],
			guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
	else
		uuid = _T("00000000-0000-4000-8000-000000000000");
}

void Analytics::urlEncode(const CString &in, CString &out)
{
	wchar_t buf[256];
	DWORD bufSize = 256;
	InternetCanonicalizeUrl((LPCWSTR)in, buf, &bufSize, ICU_DECODE);
	out = buf;	
}

void Analytics::prefixId(CString &id)
{
	id.Format(_T("v=1&tid=%s&cid=%s&an=Endless%%20Installer&av=%s&ul=%s&cd1=%s&"),
		m_trackingId, m_clientId, _T(RELEASE_VER_STR), m_language, m_windowsVersion);

	if (m_manufacturer && m_model)
	    id.AppendFormat(L"cd3=%s&cd4=%s&", m_manufacturer, m_model);

	if (m_firmware)
	    id.AppendFormat(L"cd5=%s&", m_firmware);
}
