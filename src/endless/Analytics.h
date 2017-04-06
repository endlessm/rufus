#pragma once

class Analytics
{
public:
	Analytics();
	~Analytics();

	static Analytics *instance();

	void startSession();
	// like eventTracking(category, _T("Closed")), but with the session-close
	// flag set.
	HANDLE stopSession(const CString &category);

	void screenTracking(const CString &name);
	void eventTracking(const CString &category, const CString &action, const CString &label = CString(), LONGLONG value = -1);
	void exceptionTracking(const CString &description, BOOL fatal);

	void setLanguage(const CString &language);
	void setManufacturerModel(const CString &manufacturer, const CString &model);

private:
	void sendRequest(const CString &body, BOOL lastRequest = FALSE);
	BOOL debug();
	BOOL disabled();
	void loadUuid(CString &uuid);
	void createUuid(CString &uuid);
	void urlEncode(const CString &in, CString &out);
	void prefixId(CString &id);

	CString m_trackingId;
	CString m_clientId;
	BOOL m_disabled;
	CString m_language;
	CString m_manufacturer;
	CString m_model;
	CString m_windowsVersion;
	CWinThread *m_workerThread;
};
