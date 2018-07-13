/*
 * The program which sets an ESP8266 microcontroller to act as a Wi-Fi scanner.
 * 
 * Written by Marc Katzef
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "WifiCredentials.h"

#define MAX_SSID_LENGTH 64
#define MAX_PASSWORD_LENGTH 64

ESP8266WebServer server(80);
IPAddress broadcastAddress = IPAddress(0, 0, 0, 0);
char g_ssid[MAX_SSID_LENGTH] = {0};
char g_password[MAX_PASSWORD_LENGTH] = {0};


// Returns true if soft access point is enabled
bool isAp() {
  return WiFi.softAPIP() != broadcastAddress;
}


// Returns true if connected to an existing network
bool isStation() {
  return WiFi.localIP() != broadcastAddress;
}


// Enables soft access point with SSID hardcoded
void startSoftAp() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ScannerSetup0"); // Starts WiFi network, on which the ESP8266 is 192.168.4.1
}


// Attempts to join an existing network with ssid and password data from an HTTP POST message
void handleLogin() {
  if( !server.hasArg("ssid") || !server.hasArg("password") 
      || server.arg("ssid") == NULL || server.arg("password") == NULL) {
    server.send(400, "text/plain", "400: Invalid Request");
    return;
  }

  if (!isAp()) {
    server.send(200, "text/html", "No fallback connection detected, please enable AP.</br><a href=\"/\">Home</a>");
    return;
  }
  
  server.send(200, "text/html", "Attempting to login...</br>Please refresh <a href=\"/\">Home</a> for connection status.");

  server.arg("ssid").toCharArray(g_ssid, MAX_SSID_LENGTH);
  server.arg("password").toCharArray(g_password, MAX_PASSWORD_LENGTH);
  
  WiFi.begin(g_ssid, g_password);
}


// Responds to HTTP GET messages with the simple home page HTML
void handleRoot() {
  server.send(200, "text/html", "Status:</br>AP IP: " + WiFi.softAPIP().toString() + "</br>Network IP: " + WiFi.localIP().toString() + "</br></br>Options:</br><a href=\"/set-network\">Set network</a></br><a href=\"/toggle-ap\">Toggle AP</a></br><a href=\"/rssi\">RSSI</a>");
}


// Responds to HTTP GET messages with the current RSSI as a string
void handleRssi() {
  server.send(200, "text/html", String(WiFi.RSSI()));
}


// Responds to an HTTP GET message with a text entry form to supply a ssid and password
void handleSetNetwork() {
  server.send(200, "text/html", "<form action=\"/login\" method=\"POST\"><input type=\"text\" name=\"ssid\" placeholder=\"SSID\"></br><input type=\"password\" name=\"password\" placeholder=\"Password\"></br><input type=\"submit\" value=\"Submit\"></form><p>Please enter the name and password of the WiFi network to map.</br><a href=\"/\">Home</a></p>");
}


// Responds to an HTTP GET request by toggling soft access point mode (if connected to an existing network
void handleToggleAp() {
  if (!isAp()) {
    server.send(200, "text/html", "Enabling AP...</br><a href=\"/\">Home</a>");
    startSoftAp();
  } else if (isStation()) {
    server.send(200, "text/html", "Disabling AP...</br><a href=\"/\">Home</a>");
    WiFi.mode(WIFI_STA);
  } else {
    server.send(200, "text/html", "No fallback network detected!</br><a href=\"/\">Home</a>");
  }
}


// Installs HTTP request handlers
void setup(void) {
  Serial.begin(9600);
  startSoftAp();

  WIFI_SSID.toCharArray(g_ssid, MAX_SSID_LENGTH);
  WIFI_PASSPHRASE.toCharArray(g_password, MAX_PASSWORD_LENGTH);
  WiFi.begin(g_ssid, g_password);
  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/rssi", HTTP_GET, handleRssi);
  server.on("/set-network", HTTP_GET, handleSetNetwork);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/toggle-ap", HTTP_GET, handleToggleAp);
  server.begin();
}


// Waits for and handles incoming requests
void loop(void) {
  server.handleClient();
}
