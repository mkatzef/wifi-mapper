/*
 * The module responsible for communicating with the ESP8266 modules.
 *
 * Written by Marc Katzef
 */

#pragma once

#include "stdafx.h"
#include <strsafe.h>
#include <string>


// A container to pass all required information to a worker thread
struct WorkerData {
	std::string ipAddress;
	HANDLE *mutexHandle;
	int *rssiVar;
};


// The object which communicates with a single ESP8266 module
class WiFiReceiver {
public:
	// Constructor, requires the IP address of the target ESP8266 (eg: "192.168.1.72")
	WiFiReceiver(std::string ipAddress);

	// A non-blocking function which returns the most recently-received RSSI value (in dBm) from the ESP8266
	int getRssi();

	// Kill worker thread (Warning: no reconnect method has been implemented.)
	void disconnect();

private:
	std::string m_ipAddress;
	int m_rssi;
	DWORD m_threadId;
	HANDLE m_threadHandle;
	HANDLE m_rssiMutex;
	WorkerData workerArgs;
};