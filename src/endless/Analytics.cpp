#include "stdafx.h"
#include "Analytics.h"
#include <afxinet.h>

extern "C" {
	#include "rufus.h"
}

#include "json/json.h"

#include "Version.h"
#include "GeneralCode.h"

#define TRACKING_ID "UA-61784217-3"
#define REG_SECTION "Analytics"
#define REG_KEY_CLIENT_ID "ClientID"
#define REG_KEY_DISABLED "Disabled"
#define REG_KEY_DEBUG "Debug"

#define SERVER_NAME "www.google-analytics.com"
#define SERVER_PORT 443

static void HandleDebugResponse(CHttpFile *file)
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

static UINT threadSendRequest(LPVOID pParam)
{
	FUNCTION_ENTER;

	BOOL debug = (BOOL) pParam;

	CInternetSession session(_T("Endless Installer"));
	MSG msg;

	while (GetMessage(&msg, NULL, WM_APP, WM_APP+1)) {
		CString *pBody = (CString *)msg.wParam;
		CStringA bodyUtf8 = CW2A(*pBody);

		try {
			CString headers = _T("Content-type: application/x-www-form-urlencoded");
			CHttpConnection *conn = session.GetHttpConnection(_T(SERVER_NAME), (INTERNET_PORT)SERVER_PORT);
			CString path = debug ? _T("debug/collect") : _T("collect");
			const DWORD flags = INTERNET_FLAG_EXISTING_CONNECT
				| INTERNET_FLAG_KEEP_CONNECTION
				| INTERNET_FLAG_SECURE
				| INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP
				| INTERNET_FLAG_NO_COOKIES
				| INTERNET_FLAG_NO_UI;
			CHttpFile *file = conn->OpenRequest(CHttpConnection::HTTP_VERB_POST, path,
				/* Referer */ NULL, /* context */ 1, /* Accept */ NULL,
				_T("HTTP/1.1"), flags);
			if (file) {
				file->SendRequest(headers, (LPVOID)(LPCSTR)bodyUtf8, bodyUtf8.GetLength());
				uprintf("Analytics req: %s\n", (LPCSTR)bodyUtf8);
				DWORD responseCode = 0;
				IFFALSE_PRINTERROR(file->QueryInfoStatusCode(responseCode), "QueryInfoStatusCode failed");
				uprintf("Analytics: response code %d", responseCode);
				if (debug) {
					HandleDebugResponse(file);
				}
				file->Close();
				delete file;
			}
			else {
				uprintf("Analytics req failed\n");
			}
			conn->Close();
			delete conn;
		}
		catch (CInternetException *ex) {
			TCHAR err[256];
			ex->GetErrorMessage(err, 256);
			uprintf("Analytics req error: %ls\n", (LPCTSTR)err);
		}

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
	m_workerThread = AfxBeginThread(threadSendRequest, (LPVOID) debug());
}

Analytics::~Analytics()
{
}

Analytics *Analytics::instance()
{
	static Analytics instance;
	return &instance;
}

void Analytics::sessionControl(BOOL start)
{
	if (m_disabled) return;
	FUNCTION_ENTER;
	CString body;
	prefixId(body);
	if (start) {
		body = body + _T("t=screenview&cd=DualBootInstallPage&sc=start");
		sendRequest(body);
	}
	else {
		body = body + _T("t=screenview&cd=LastPage&sc=end");
		sendRequest(body, TRUE);
		DWORD ret = WaitForSingleObject(m_workerThread->m_hThread, 4000);
		if (ret == WAIT_TIMEOUT) {
			delete m_workerThread;
			m_workerThread = NULL;
		}
	}
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
}
