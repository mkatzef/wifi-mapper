/*
* The module responsible for communicating with the ESP8266 modules.
*
* Written by Marc Katzef
*/

#include "WiFiReceiver.h"

#include <iostream>
#include <string>
#include <winhttp.h>

using namespace std;

// Convert string to the format expected by Windows network commands.
// (From https://stackoverflow.com/questions/27220/how-to-convert-stdstring-to-lpcwstr-in-c-unicode)
std::wstring s2ws(const std::string& s) {
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}


// Collect RSSI value from an ESP8266 module with the given IP address
int fetchRssiFromServer(string ipAddress) {
	DWORD dwSize = sizeof(DWORD);
	DWORD dwDownloaded = 0;
	LPSTR pszOutBuffer;
	HINTERNET hConnect = NULL;
	HINTERNET hRequest = NULL;
	HINTERNET hSession = NULL;
	bool bResults = false;
	int rssi = 0;

	// Use WinHttpOpen to obtain an HINTERNET handle.
	hSession = WinHttpOpen(NULL,
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);

	std::wstring stemp = s2ws(ipAddress);
	LPCWSTR ipAddressWindowsStr = stemp.c_str();

	// Specify an HTTP server.
	if (hSession)
		hConnect = WinHttpConnect(hSession, ipAddressWindowsStr,
			INTERNET_DEFAULT_HTTP_PORT, 0);

	// Create an HTTP request handle.
	if (hConnect)
		hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/rssi",
			NULL, WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			0);

	// Send a request.
	if (hRequest)
		bResults = WinHttpSendRequest(hRequest,
			WINHTTP_NO_ADDITIONAL_HEADERS,
			0, WINHTTP_NO_REQUEST_DATA, 0,
			0, 0);

	if (bResults)
		bResults = WinHttpReceiveResponse(hRequest, NULL);


	// Keep checking for data until there is nothing left.
	if (bResults) {
		do {
			// Check for available data.
			dwSize = 0;
			if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
				printf("Error %u in WinHttpQueryDataAvailable.\n",
					GetLastError());
				break;
			}

			// No more available data.
			if (!dwSize)
				break;

			// Allocate space for the buffer.
			pszOutBuffer = new char[dwSize + 1];
			if (!pszOutBuffer) {
				printf("Out of memory\n");
				break;
			}

			// Read the Data.
			ZeroMemory(pszOutBuffer, dwSize + 1);

			if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer,
				dwSize, &dwDownloaded)) {
				printf("Error %u in WinHttpReadData.\n", GetLastError());
			} else {
				try {
					rssi = atoi(pszOutBuffer);
				} catch (exception &e) {
					rssi = 0;
				}
			}

			// Free the memory allocated to the buffer.
			delete[] pszOutBuffer;

			// This condition should never be reached since WinHttpQueryDataAvailable
			// reported that there are bits to read.
			if (!dwDownloaded)
				break;

		} while (dwSize > 0);
	} else {
		// Report any errors.
		printf("Error %d has occurred.\n", GetLastError());
	}

	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);

	return rssi;
}


// Collect RSSI values in an infinite loop
DWORD WINAPI workerFunction(LPVOID lpParam) {
	WorkerData *arg = (WorkerData*)lpParam;
	string ipAddress = arg->ipAddress;
	HANDLE *mutexHandle = arg->mutexHandle;
	int *rssi = arg->rssi;
	Rendezvous *rendezvous = arg->rendezvous;

	while (1) {
		int newRssi = fetchRssiFromServer(ipAddress);
		
		WaitForSingleObject(*mutexHandle, INFINITE);
		*rssi = newRssi;
		ReleaseMutex(*mutexHandle);

		if (rendezvous) {
			rendezvous->arrive();
		}
	}
}


void WiFiReceiver::setIpAddress(std::string ipAddress) {
	m_ipAddress = ipAddress;
}


void WiFiReceiver::addRenzezvous(Rendezvous *rendezvous) {
	m_rendezvous = rendezvous;
}


void WiFiReceiver::start() {
	m_rssi = 0;

	m_rssiMutex = CreateMutex(
		NULL,				// default security attributes
		FALSE,				// initially not owned
		NULL);				// unnamed mutex

	workerArgs.ipAddress = m_ipAddress;
	workerArgs.mutexHandle = &m_rssiMutex;
	workerArgs.rssi = &m_rssi;
	workerArgs.rendezvous = m_rendezvous;

	m_threadHandle = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		workerFunction,			// thread function name
		&workerArgs,			// argument to thread function 
		0,                      // use default creation flags 
		&m_threadId);
}


int WiFiReceiver::getRssi() {
	int rssi;

	WaitForSingleObject(m_rssiMutex, INFINITE);
	rssi = m_rssi;
	ReleaseMutex(m_rssiMutex);

	return rssi;
}


void WiFiReceiver::disconnect() {
	CloseHandle(m_threadHandle);
	CloseHandle(m_rssiMutex);
}
