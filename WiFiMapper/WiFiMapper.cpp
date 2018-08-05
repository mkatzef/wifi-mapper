/*
* The module responsible for collecting data from the Kinect Sensor v2,
* and using supporting modules to form Wi-Fi point clouds.
*
* Based on the Kinect SDK 2.0 demo programs "Color Basics-D2D" and "Depth Basics-D2D"
*
* Written by Marc Katzef
*/

#include "stdafx.h"
#include <strsafe.h>
#include "resource.h"
#include "WiFiMapper.h"
#include "PointCloud.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <ctime>
#include <iomanip>


using namespace cv;
using namespace std;

std::string cloudDir = "./Clouds/";
bool writeEnvCloud = true;

int main(void) {
	WiFiMapper application;

	chrono::time_point<chrono::system_clock> now = std::chrono::system_clock::now();
	long long in_time_t = chrono::system_clock::to_time_t(now);

	if (writeEnvCloud) {
		PointCloud envCloud = application.GetEnvironmentCloud();
		std::stringstream env_ss;
		now = std::chrono::system_clock::now();
		in_time_t = std::chrono::system_clock::to_time_t(now);
		env_ss << cloudDir << "env_map " << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H.%M.%S") << ".pcd";
		envCloud.WriteToFileSafe(env_ss.str());
	}

	PointCloud mappedCloud = application.Run();

	stringstream map_ss;
	now = std::chrono::system_clock::now();
	in_time_t = std::chrono::system_clock::to_time_t(now);
	map_ss << cloudDir << "map " << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H.%M.%S") << ".pcd";
	mappedCloud.WriteToFileSafe(map_ss.str());
}


WiFiMapper::WiFiMapper() :
	m_pKinectSensor(NULL),
	m_pMultiSourceReader(NULL),
	m_pColorRGBX(NULL),
	m_receivers(cWiFiModules.size()),
	m_receiverSync(cWiFiModules.size()) {

	for (size_t i = 0; i < cWiFiModules.size(); ++i) {
		m_receivers[i].setIpAddress(cWiFiModules[i].ipAddress);
		m_receivers[i].addRenzezvous(&m_receiverSync);
		m_receivers[i].start();
	}

	// create heap storage for color pixel data in RGBX format
	m_pColorRGBX = new RGBQUAD[cColorWidth * cColorHeight];
	m_pDepth = new UINT16[cDepthWidth * cDepthHeight];
	m_displayBuffer = new UINT8[cDepthWidth * cDepthHeight * 3];

	// Initialise marker trackers
	m_trackerA.init(cDepthWidth, cDepthHeight, cDepthVFov, m_initOriginA, m_initDistance_mm, m_markerRadius_mm, m_markerRadiusTolerance, m_centerTolerance_px);
	m_trackerB.init(cDepthWidth, cDepthHeight, cDepthVFov, m_initOriginB, m_initDistance_mm, m_markerRadius_mm, m_markerRadiusTolerance, m_centerTolerance_px);

	InitializeDepthAndColorSensors();
	initMappingTable();
}


WiFiMapper::~WiFiMapper() {
	if (m_pColorRGBX) {
		delete[] m_pColorRGBX;
		m_pColorRGBX = NULL;
	}

	if (m_pDepth) {
		delete[] m_pDepth;
		m_pDepth = NULL;
	}

	if (m_displayBuffer) {
		delete[] m_displayBuffer;
		m_displayBuffer = NULL;
	}

	// done with frame readers
	SafeRelease(m_pMultiSourceReader);
	SafeRelease(m_pDepthFrameReader);

	// close the Kinect Sensor
	if (m_pKinectSensor) {
		m_pKinectSensor->Close();
	}

	SafeRelease(m_pKinectSensor);

	for (size_t i = 0; i < cWiFiModules.size(); ++i) {
		m_receivers[i].disconnect();
	}

	m_receiverSync.destroy();
}


PointCloud WiFiMapper::Run() {
	if (m_pMultiSourceReader) {
		DisableMultiSourceReader();
		InitializeDepthFrameReader();
	}

	namedWindow("Depth", WINDOW_NORMAL);
	namedWindow("Mask A", WINDOW_NORMAL);
	namedWindow("Mask B", WINDOW_NORMAL);

	int key = cv::waitKey(1);
	while (key != 'q') {
		Update();

		if (key == 's') {
			m_writeStats = !m_writeStats;
			if (m_writeStats) {
				m_sampleCount = 0;
				m_sampleCollectionCount++;
				std::cout << "Beginning collection " << m_sampleCollectionCount << "\n";
				std::cout << "Sample number,a_x,a_y,a_z,b_x,b_y,b_z";
				for (size_t i = 0; i < cWiFiModules.size(); ++i) {
					std::cout << ",rssi" << i + 1;
				}
				std::cout << "\n";
			}
		}

		key = cv::waitKey(1);
	}

	return m_wifiPointCloud;
}


void WiFiMapper::Update() {
	if (!m_pDepthFrameReader) {
		return;
	}

	IDepthFrame* pDepthFrame = NULL;

	HRESULT hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);

	if (SUCCEEDED(hr)) {
		UINT nBufferSize = 0;
		UINT16 *pBuffer = NULL;

		if (SUCCEEDED(hr)) {
			hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);
		}

		if (SUCCEEDED(hr)) {
			ProcessChannels(pBuffer, cDepthWidth, cDepthHeight);
		}
	}

	SafeRelease(pDepthFrame);
}


PointCloud WiFiMapper::GetEnvironmentCloud() {
	PointCloud result;
	bool succeeded = false;

	while (!succeeded) {
		if (!m_pMultiSourceReader) {
			continue;
		}

		IColorFrame* pColorFrame = NULL;
		IDepthFrame* pDepthFrame = NULL;
		IMultiSourceFrame* pMultiFrame = NULL;

		HRESULT hr = m_pMultiSourceReader->AcquireLatestFrame(&pMultiFrame);

		if (SUCCEEDED(hr)) {
			ColorImageFormat imageFormat = ColorImageFormat_None;
			UINT nBufferSizeColor = 0;
			RGBQUAD *pBufferColor = NULL;
			UINT nBufferSizeDepth = 0;
			UINT16 *pBufferDepth = NULL;

			IColorFrameReference *colorRef = NULL;
			pMultiFrame->get_ColorFrameReference(&colorRef);
			HRESULT hrColor = colorRef->AcquireFrame(&pColorFrame);
			if (colorRef) {
				colorRef->Release();
			}
			if (!SUCCEEDED(hrColor)) {
				SafeRelease(pColorFrame);
				SafeRelease(pDepthFrame);
				SafeRelease(pMultiFrame);
				continue;
			}

			IDepthFrameReference *depthRef = NULL;
			pMultiFrame->get_DepthFrameReference(&depthRef);
			HRESULT hrDepth = depthRef->AcquireFrame(&pDepthFrame);
			if (depthRef) {
				depthRef->Release();
			}
			if (!SUCCEEDED(hrDepth)) {
				SafeRelease(pColorFrame);
				SafeRelease(pDepthFrame);
				SafeRelease(pMultiFrame);
				continue;
			}

			if (SUCCEEDED(hrDepth)) {
				pBufferDepth = m_pDepth;
				nBufferSizeColor = cDepthWidth * cDepthHeight * sizeof(UINT16);
				hrDepth = pDepthFrame->CopyFrameDataToArray(cDepthWidth * cDepthHeight, pBufferDepth);

				pBufferColor = m_pColorRGBX;
				nBufferSizeColor = cColorWidth * cColorHeight * sizeof(RGBQUAD);
				hrColor = pColorFrame->CopyConvertedFrameDataToArray(nBufferSizeColor, reinterpret_cast<BYTE*>(pBufferColor), ColorImageFormat_Bgra);
			} else {
				hrColor = E_FAIL;
				hrDepth = E_FAIL;
			}

			if (SUCCEEDED(hrColor) && SUCCEEDED(hrDepth)) {
				SafeRelease(pColorFrame);
				SafeRelease(pDepthFrame);
				SafeRelease(pMultiFrame);

				result = formPointCloud(pBufferColor, pBufferDepth);
				succeeded = true;
			}
		}

		SafeRelease(pColorFrame);
		SafeRelease(pDepthFrame);
		SafeRelease(pMultiFrame);
	}

	return result;
}


void WiFiMapper::initMappingTable() {
	bool succeeded = false;
	while (!succeeded) {
		if (!m_pMultiSourceReader) {
			continue;
		}

		IColorFrame* pColorFrame = NULL;
		IDepthFrame* pDepthFrame = NULL;
		IMultiSourceFrame* pMultiFrame = NULL;

		HRESULT hr = m_pMultiSourceReader->AcquireLatestFrame(&pMultiFrame);

		if (SUCCEEDED(hr)) {
			ColorImageFormat imageFormat = ColorImageFormat_None;
			UINT nBufferSizeColor = 0;
			RGBQUAD *pBufferColor = NULL;
			UINT nBufferSizeDepth = 0;
			UINT16 *pBufferDepth = NULL;

			IColorFrameReference *colorRef = NULL;
			pMultiFrame->get_ColorFrameReference(&colorRef);
			HRESULT hrColor = colorRef->AcquireFrame(&pColorFrame);
			if (colorRef) {
				colorRef->Release();
			}
			if (!SUCCEEDED(hrColor)) {
				SafeRelease(pColorFrame);
				SafeRelease(pDepthFrame);
				SafeRelease(pMultiFrame);
				continue;
			}

			IDepthFrameReference *depthRef = NULL;
			pMultiFrame->get_DepthFrameReference(&depthRef);
			HRESULT hrDepth = depthRef->AcquireFrame(&pDepthFrame);
			if (depthRef) {
				depthRef->Release();
			}
			if (!SUCCEEDED(hrDepth)) {
				SafeRelease(pColorFrame);
				SafeRelease(pDepthFrame);
				SafeRelease(pMultiFrame);
				continue;
			}

			if (SUCCEEDED(hrDepth)) {
				pBufferDepth = m_pDepth;
				nBufferSizeColor = cDepthWidth * cDepthHeight * sizeof(UINT16);
				hrDepth = pDepthFrame->CopyFrameDataToArray(cDepthWidth * cDepthHeight, pBufferDepth);

				pBufferColor = m_pColorRGBX;
				nBufferSizeColor = cColorWidth * cColorHeight * sizeof(RGBQUAD);
				hrColor = pColorFrame->CopyConvertedFrameDataToArray(nBufferSizeColor, reinterpret_cast<BYTE*>(pBufferColor), ColorImageFormat_Bgra);
			} else {
				hrColor = E_FAIL;
				hrDepth = E_FAIL;
			}

			if (SUCCEEDED(hrColor) && SUCCEEDED(hrDepth)) {
				uint depthPixelCount;
				m_pMapper->GetDepthFrameToCameraSpaceTable(&depthPixelCount, &m_depthToCameraSpaceTable);
				succeeded = true;
			}
		}

		SafeRelease(pColorFrame);
		SafeRelease(pDepthFrame);
		SafeRelease(pMultiFrame);
	}
}


PointCloud WiFiMapper::formPointCloud(RGBQUAD *pBufferColor, UINT16 *pBufferDepth) {
	PointCloud result;
	size_t depthPixelCount = cDepthWidth * cDepthHeight;
	ColorSpacePoint *colorSpacePoints = new ColorSpacePoint[depthPixelCount];
	m_pMapper->MapDepthFrameToColorSpace(depthPixelCount, pBufferDepth, depthPixelCount, colorSpacePoints);

	for (size_t y = 0; y < cDepthHeight; y++) {
		for (size_t x = 0; x < cDepthHeight; x++) {
			size_t index = y * cDepthWidth + x;
			ColorSpacePoint csp = colorSpacePoints[index];
			UINT16 depth = pBufferDepth[index];

			if (depth == 0) {
				continue;
			}

			int colX = csp.X;
			int colY = csp.Y;
			if (colX < 0 || colX >= cColorWidth || colY < 0 || colY >= cColorHeight) {
				continue; // ignore 0 values
			}

			Point3f desc = { (float)x, (float)y, (float)depth };
			RGBQUAD color = pBufferColor[colY * cColorWidth + colX];

			result.AddPoint(desc2Pos(desc), color.rgbRed, color.rgbGreen, color.rgbBlue);
		}
	}
	delete[] colorSpacePoints;

	return result;
}


HRESULT WiFiMapper::InitializeDepthAndColorSensors() {
	HRESULT hr;

	hr = GetDefaultKinectSensor(&m_pKinectSensor);
	if (FAILED(hr)) {
		return hr;
	}

	if (m_pKinectSensor) {
		m_pKinectSensor->Open();
		m_pKinectSensor->get_CoordinateMapper(&m_pMapper);
		hr = m_pKinectSensor->OpenMultiSourceFrameReader(FrameSourceTypes_Depth | FrameSourceTypes_Color, &m_pMultiSourceReader);
	}

	if (!m_pKinectSensor || FAILED(hr)) {
		std::cout << "No ready Kinect found!";
		return E_FAIL;
	}

	return hr;
}


void WiFiMapper::DisableMultiSourceReader() {
	if (m_pMultiSourceReader) {
		SafeRelease(m_pMultiSourceReader);
		m_pMultiSourceReader = NULL;
	}
}


HRESULT WiFiMapper::InitializeDepthFrameReader() {
	HRESULT hr;

	if (m_pKinectSensor) {
		IDepthFrameSource *pDFS;
		m_pKinectSensor->get_DepthFrameSource(&pDFS);
		hr = pDFS->OpenReader(&m_pDepthFrameReader);
		SafeRelease(m_pKinectSensor);
	}

	return hr;
}


float distanceBetween(Point3f positionA, Point3f positionB) {
	Point3f diff = positionB - positionA;
	float diffX = diff.x;
	float diffY = diff.y;
	float diffZ = diff.z;

	return std::sqrtf(diffX * diffX + diffY * diffY + diffZ * diffZ);
}


void WiFiMapper::ProcessChannels(UINT16 *pBufferDepth, int nDepthWidth, int nDepthHeight) {
	// Make sure we've received valid data
	if (!(pBufferDepth && (nDepthWidth == cDepthWidth) && (nDepthHeight == cDepthHeight))) {
		return;
	}

	double currentTicks = getTickCount();
	double dt = (currentTicks - m_ticks) / getTickFrequency();

	m_ticks = currentTicks;

	Mat depthFrame(nDepthHeight, nDepthWidth, CV_16UC1, pBufferDepth);

	std::pair<MarkerTracker::RET_TYPE, Point3f> resultA = m_trackerA.update(depthFrame, dt);
	std::pair<MarkerTracker::RET_TYPE, Point3f> resultB = m_trackerB.update(depthFrame, dt);

	if (resultA.first == MarkerTracker::RET_TYPE::TRACKING && resultB.first == MarkerTracker::RET_TYPE::TRACKING) {
		Point3f positionA = desc2Pos(resultA.second);
		Point3f positionB = desc2Pos(resultB.second);
		float distance_mm = distanceBetween(positionA, positionB);

		if (distance_mm >= cScannerLengthMin_mm && distance_mm <= cScannerLengthMax_mm) {
			recordPoints(positionA, positionB);
		}
	}

	// Generate intuitive image from depth values
	for (size_t y = 0; y < nDepthHeight; y++) {
		for (size_t x = 0; x < nDepthWidth; x++) {
			size_t index = y * nDepthWidth + x;
			m_displayBuffer[index * 3] = pBufferDepth[index] % 256;
			m_displayBuffer[index * 3 + 1] = 0;
			m_displayBuffer[index * 3 + 2] = 0;
		}
	}

	Mat displayFrame(nDepthHeight, nDepthWidth, CV_8UC3, m_displayBuffer);

	int radius = getObjectHeight_px(cDepthVFov * PI / 180.0, m_markerRadius_mm, m_initDistance_mm, cDepthHeight);
	if (resultA.first == MarkerTracker::RET_TYPE::EMPTY) {
		cv::circle(displayFrame, m_initOriginA, radius, { 0,0,255 }, 1);
	}
	if (resultB.first == MarkerTracker::RET_TYPE::EMPTY) {
		cv::circle(displayFrame, m_initOriginB, radius, { 0,0,255 }, 1);
	}

	cv::circle(displayFrame, m_trackerA.deb_point, 1, { 255,255,0 }, 2);
	cv::circle(displayFrame, m_trackerA.deb_point, m_trackerA.deb_sd.minRadius, { 255,255,0 });
	cv::circle(displayFrame, m_trackerA.deb_point, (int)m_trackerA.deb_sd.maxRadius, { 255,255,0 });

	cv::circle(displayFrame, m_trackerB.deb_point, 1, { 255,255,0 }, 2);
	cv::circle(displayFrame, m_trackerB.deb_point, m_trackerB.deb_sd.minRadius, { 255,255,0 });
	cv::circle(displayFrame, m_trackerB.deb_point, (int)m_trackerB.deb_sd.maxRadius, { 255,255,0 });

	imshow("Depth", displayFrame);
	imshow("Mask A", m_trackerA.deb_mask);
	imshow("Mask B", m_trackerB.deb_mask);
}


void WiFiMapper::recordPoints(Point3f posA, Point3f posB) {
	Point3f diff = posB - posA;
	Point3f dirAB = diff / distanceBetween(posA, posB);

	if (m_writeStats) {
		m_sampleCount++;
		std::cout << m_sampleCount << "," << posA.x << "," << posA.y << "," << posA.z << "," << posB.x << "," << posB.y << "," << posB.z;
	}

	for (size_t i = 0; i < cWiFiModules.size(); ++i) {
		Point3f baseMarkerPosition;
		Point3f directionVector;

		if (cWiFiModules[i].baseMarker == WifiStartPosition::markerA) {
			baseMarkerPosition = posA;
			directionVector = dirAB;
		} else {
			baseMarkerPosition = posB;
			directionVector = -dirAB;
		}

		Point3f position = baseMarkerPosition + directionVector * cWiFiModules[i].offset_mm;

		int rssi = m_receivers[i].getRssi();

		m_wifiPointCloud.AddPoint(position, -rssi, 0, 0);

		if (m_writeStats) {
			 std::cout << "," << rssi;
		}
	}

	if (m_writeStats) {
		std::cout << "\n";
	}
}


Point3f WiFiMapper::desc2Pos(Point3f desc) {
	int x = desc.x + 0.5;
	int y = desc.y + 0.5;
	float z = desc.z;

	int index = y * cDepthWidth + x;
	PointF mapping = m_depthToCameraSpaceTable[index];
	float posX = mapping.X * z;
	float posY = mapping.Y * z;

	return{ posX, posY, z };
}
