/*
 * The module responsible for communicating with the ESP8266 modules.
 *
 * Written by Marc Katzef
 */

#pragma once

#include "stdafx.h"
#include <strsafe.h>
#include <string>
#include <vector>

#include "Rendezvous.h"

// A container to pass all required information to a worker thread
struct WorkerData {
	std::string ipAddress;
	HANDLE *mutexHandle;
	int *rssi;
	Rendezvous *rendezvous;
};


// The object which communicates with a single ESP8266 module
class WiFiReceiver {
public:
	WiFiReceiver() = default;

	// Set the IP address of the target ESP8266 (eg: "192.168.1.72")
	void setIpAddress(std::string ipAddress);

	// (Optional) Set a receiver to wait for synchronization
	void addRenzezvous(Rendezvous *rendezvous);

	// Begin collecting samples
	void start();

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
	Rendezvous *m_rendezvous;
};