/*
 * The module responsible for collecting data from the Kinect Sensor v2,
 * and using supporting modules to form Wi-Fi point clouds.
 * 
 * Based on the Kinect SDK 2.0 demo programs "Color Basics-D2D" and "Depth Basics-D2D"
 * 
 * Written by Marc Katzef
 */

#pragma once

#include "resource.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/video/tracking.hpp"

#include "MarkerTracker.h"
#include "PointCloud.h"
#include "WiFiReceiver.h"

class WiFiMapper {
	// Image properties
	static const int cColorWidth = 1920;
	static const int cColorHeight = 1080;
	static const int cDepthWidth = 512;
	static const int cDepthHeight = 424;
	static const int displayWidth = 640;
	static const int displayHeight = 480;

	// Scanner properties
	static const uint m_markerRadius_mm = 34;
	const float cScannerLength_mm = 490;

	enum class WifiStartPosition{markerA, markerB};
	struct WiFiModuleDescription {
		std::string ipAddress;
		int offset_mm;
		WifiStartPosition baseMarker;
	};

	// Wi-Fi module addresses and positions on scanner
	const std::vector<WiFiModuleDescription> cWiFiModules{
		{ "192.168.1.77", 110, WifiStartPosition::markerA },
		{ "192.168.1.76", 185, WifiStartPosition::markerA },
		{ "192.168.1.71", 255, WifiStartPosition::markerA },
		{ "192.168.1.72", 165, WifiStartPosition::markerB },
		{ "192.168.1.74", 100, WifiStartPosition::markerB }
	};
	
	// Initialisation parameters and variables
	static const uint m_initDistance_mm = 750;
	const float m_markerRadiusTolerance = 0.2f;
	static const uint m_centerTolerance_px = 25;
	cv::Rect m_initRegion;
	MarkerTracker m_trackerA;
	cv::Point2i m_initOriginA{ cDepthWidth / 3, cDepthHeight / 2 };
	MarkerTracker m_trackerB;
	cv::Point2i m_initOriginB{ 2 * cDepthWidth / 3, cDepthHeight / 2 };
	const float cDepthVFov = 60.0f;

	// Scanner length sanity check
	const float cScannerLengthMin_mm = cScannerLength_mm * 0.8;
	const float cScannerLengthMax_mm = cScannerLength_mm * 1.2;

	// Statistics variables
	bool m_writeStats = false;
	int m_sampleCollectionCount = 0;
	int m_sampleCount = 0;

public:
	// Constructor
	WiFiMapper();

	// Destructor
	~WiFiMapper();

	// The main loop
	PointCloud Run();

	// Generates a coloured point cloud of the scanned room
	PointCloud GetEnvironmentCloud();

private:
	PointCloud m_wifiPointCloud;
	std::vector<WiFiReceiver> m_receivers;
	Rendezvous m_receiverSync;
	double m_ticks = 0;

	// Current Kinect
	IKinectSensor* m_pKinectSensor;

	// Frame readers
	IMultiSourceFrameReader* m_pMultiSourceReader;
	IDepthFrameReader* m_pDepthFrameReader;

	// Coordinate mapping
	ICoordinateMapper* m_pMapper;
	PointF* m_depthToCameraSpaceTable;

	// Display images
	RGBQUAD* m_pColorRGBX;
	UINT16* m_pDepth;
	UINT8* m_displayBuffer;

	// The main processing function
	void Update();

	// Initializes the Kinect sensor
	HRESULT InitializeDepthAndColorSensors();
	HRESULT InitializeDepthFrameReader();
	void initMappingTable();

	// Switches to use only the depth frame input
	void DisableMultiSourceReader();

	// Merge a colour frame and depth frame to form a PointCloud
	PointCloud formPointCloud(RGBQUAD *pBufferColor, UINT16 *pBufferDepth);

	// Use depth values to identify scanner marker positions
	void ProcessChannels(UINT16 *pBufferDepth, int nWidthDepth, int nHeightDepth);
	
	// Use scanner marker positions to calculate Wi-Fi module positions
	// and store them in a point cloud (along with their RSSI readings).
	void recordPoints(cv::Point3f posA, cv::Point3f posB);
	
	// Convert from (px, px, mm) in depth space to (mm, mm, mm) in camera space
	cv::Point3f desc2Pos(cv::Point3f desc);
};
