/*
 * The module responsible for tracking (spherical) visual markers,
 * and supporting functions.
 *
 * Written by Marc Katzef
 */

#pragma once

#include "stdafx.h"
#include <strsafe.h>
#include <iostream>

#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/video/tracking.hpp"

#define PI 3.14159265358979


// Calculates the height (in pixels) of an object in a frame
double getObjectHeight_px(double verticalFov_rad, double objectHeight_mm, double distance_mm, uint imageHeight_px) {
	double imageHeight_mm = 2 * distance_mm * std::tan(verticalFov_rad / 2);
	double imageProportion = objectHeight_mm / imageHeight_mm;

	return imageProportion * imageHeight_px;
}

// Returns a rectangle centred around the given point, with the given dimensions
// The rectangle is cropped not to exceed the given limits.
cv::Rect getCenteredRect(const cv::Point center, uint width, uint height, int minX = -1, int maxX = -1, int minY = -1, int maxY = -1) {
	cv::Point bottomLeft = center - cv::Point{ (int)(width / 2), (int)(height / 2) };
	cv::Point topRight = bottomLeft + cv::Point{ (int)width, (int)height };

	cv::Rect result{ bottomLeft, topRight };

	if (minX != -1 && result.x < minX) {
		result.x = minX;
	}

	if (maxX != -1 && (result.x + width) >= maxX) {
		result.x = std::max(maxX - width, (uint)minX);
	}

	if (minY != -1 && result.y < minY) {
		result.y = minY;
	}

	if (maxY != -1 && (result.y + height) >= maxY) {
		result.y = std::max(maxY - height, (uint)minY);
	}

	return result;
}


// Returns the mean of the values in the given single-channel matrix
float getMatMean(const cv::Mat img) {
	double total = 0;
	int rowCount = img.rows;
	int colCount = img.cols;

	if (rowCount == -1 || colCount == -1) {
		return 0;
	}

	for (size_t y = 0; y < rowCount; y++) {
		for (size_t x = 0; x < colCount; x++) {
			total += img.at<UINT16>(y * colCount + x);
		}
	}

	return total / (rowCount * colCount);
}


// Returns the median of the values in the given single-channel matrix
float getMatMedian(const cv::Mat img) {
	double total = 0;
	int rowCount = img.rows;
	int colCount = img.cols;

	if (rowCount == -1 || colCount == -1) {
		return 0;
	}

	std::vector<UINT16> data(rowCount * colCount);
	for (size_t y = 0; y < rowCount; y++) {
		for (size_t x = 0; x < colCount; x++) {
			int index = y * colCount + x;
			data[index] = img.at<UINT16>(index);
		}
	}

	std::sort(data.begin(), data.end());

	return data[data.size() / 2];
}


// A comparison functor for two contours based on their size
bool contourPredicate(std::vector<cv::Point> i, std::vector<cv::Point> j) {
	return i.size() > j.size();
}


// A container for the values needed to describe a region of a depth image to search for a marker
struct SearchDescription {
	UINT16 minDepth;
	UINT16 maxDepth;
	float minRadius;
	float maxRadius;
	cv::Point2f origin;
};


// An object responsible for tracking a single spherical marker
class MarkerTracker {
public:
	enum class STATES { INITIALIZING, TRACKING };
	enum class RET_TYPE { EMPTY, INITIALIZING, TRACKING, TRACKING_EMPTY };
	
	// Indicators of current state (for debugging and visualisation)
	cv::Mat deb_mask;
	cv::Point deb_point;
	SearchDescription deb_sd;
	std::pair<cv::Point2f, float> deb_ball;

	MarkerTracker() = default;

	// Initial tracking position
	void init(uint depthImageWidth, uint depthImageHeight, double vfov_deg, cv::Point2f initOrigin, uint depth_px, uint radius_mm, float radiusTolerance, uint originTolerance_px) {
		m_depthImageWidth = depthImageWidth;
		m_depthImageHeight = depthImageHeight;
		m_initDepth_mm = depth_px;
		m_radius_mm = radius_mm;
		m_radiusTolerance = radiusTolerance;
		m_originTolerance_px = originTolerance_px;
		m_vfov_rad = vfov_deg * PI / 180.0;
		m_initOrigin = initOrigin;
	}

	// Uses the given depth frame and the time since the previous was given to approximate marker position.
	// Returns a status code and a point containing x and y (in pixels) and the depth of the identified marker.
	std::pair<RET_TYPE, cv::Point3f> update(const cv::Mat &depthFrame, double timeDiff) {
		RET_TYPE resultType = RET_TYPE::EMPTY;
		cv::Point3f markerPosition;

		// Get search region
		SearchDescription sd = getSearchDescription(depthFrame, timeDiff);
		deb_sd = sd;

		// Search for ball
		std::pair<cv::Point2f, float> ball = findBall(depthFrame, sd.minDepth, sd.maxDepth, sd.minRadius, sd.maxRadius, sd.origin, m_originTolerance_px);
		deb_ball = ball;

		// Update filter
		processResults(ball, timeDiff);

		// Package results
		if (ball.second == -1) {
			if (m_state == STATES::TRACKING) {
				resultType = RET_TYPE::TRACKING_EMPTY;
			}
		} else {
			switch (m_state) {
			case STATES::INITIALIZING: {
				resultType = RET_TYPE::INITIALIZING;
				break;
			}
			case STATES::TRACKING: {
				resultType = RET_TYPE::TRACKING;
				float depth = (int)getMatMedian(depthFrame(getCenteredRect({ (int)ball.first.x, (int)ball.first.y }, std::max((int)(ball.second / 2), 1), std::max((int)(ball.second / 2), 1), 0, depthFrame.cols, 0, depthFrame.rows)));
				markerPosition = cv::Point3f(ball.first.x, ball.first.y, depth);
				break;
			}
			}
		}

		return{ resultType, markerPosition };
	}


private:
	uint m_depthImageWidth;
	uint m_depthImageHeight;
	uint m_initDepth_mm;
	uint m_radius_mm;
	float m_radiusTolerance;
	uint m_originTolerance_px;
	double m_vfov_rad;
	cv::Point2f m_initOrigin;

	STATES m_state = STATES::INITIALIZING;
	cv::KalmanFilter m_filter;
	uint m_stateCount = 6;
	uint m_measurementCount = 4;
	cv::Mat m_markerState;
	cv::Mat m_markerMeasurement;
	uint m_initCounter = 0;
	uint m_missCounter = 0;

	// Sets the initial values of the Kalman filter used for prediction.
	void initMarkerKalmanFilter(std::pair<cv::Point2f, float> firstDescription) {
		m_filter.init(m_stateCount, m_measurementCount, 0, CV_32F);
		m_markerState = cv::Mat::zeros(m_stateCount, 1, CV_32F);
		m_markerMeasurement = cv::Mat::zeros(m_measurementCount, 1, CV_32F);

		cv::setIdentity(m_filter.transitionMatrix);

		m_filter.measurementMatrix = cv::Mat::zeros(m_measurementCount, m_stateCount, CV_32F);
		m_filter.measurementMatrix.at<float>(0) = 1.0f;
		m_filter.measurementMatrix.at<float>(7) = 1.0f;
		m_filter.measurementMatrix.at<float>(16) = 1.0f;
		m_filter.measurementMatrix.at<float>(23) = 1.0f;

		m_filter.processNoiseCov.at<float>(0) = 0.01f;
		m_filter.processNoiseCov.at<float>(7) = 0.01f;
		m_filter.processNoiseCov.at<float>(14) = 3.0f;
		m_filter.processNoiseCov.at<float>(21) = 3.0f;
		m_filter.processNoiseCov.at<float>(28) = 0.01f;
		m_filter.processNoiseCov.at<float>(35) = 3.0f;

		cv::setIdentity(m_filter.measurementNoiseCov, cv::Scalar(1e-1));

		m_filter.errorCovPre.at<float>(0) = 1; // px
		m_filter.errorCovPre.at<float>(7) = 1; // px
		m_filter.errorCovPre.at<float>(14) = 1;
		m_filter.errorCovPre.at<float>(21) = 1;
		m_filter.errorCovPre.at<float>(28) = 1; // px
		m_filter.errorCovPre.at<float>(35) = 1; // px

		m_markerState.at<float>(0) = firstDescription.first.x;
		m_markerState.at<float>(1) = firstDescription.first.y;
		m_markerState.at<float>(2) = 0;
		m_markerState.at<float>(3) = 0;
		m_markerState.at<float>(4) = firstDescription.second;
		m_markerState.at<float>(5) = 0;

		m_filter.statePost = m_markerState;
	}

	// Searches for a circle in the given single-channel frame.
	// The found circle must:
	//   - have an origin within the bounding box specified by bottomLeft and topRight (both in pixels).
	//   - have a radius in [minRadius, maxRadius] 
	// Returns the position (x and y in pixels) of the centre of the marker in the depth image, and the identified marker's radius
	std::pair<cv::Point2f, float> findBall(const cv::Mat &img, UINT16 minDepth_mm, UINT16 maxDepth_mm, float minRadius_px, float maxRadius_px, cv::Point2f origin_px, uint originTolerance_px) {
		std::pair<cv::Point2f, float> invalidRet{ { -1, -1 }, -1 };

		// Get subsection of the given frame which has to contain the ball + padding to avoid false positives
		int searchWidth = 100;
		int searchHeight = 100;
		int rows = img.rows;
		int cols = img.cols;
		int originX = (int)(origin_px.x + 0.5);
		int originY = (int)(origin_px.y + 0.5);

		int minCol = std::max(0, originX - searchWidth / 2);
		int maxCol = std::min(cols, minCol + searchWidth);
		int minRow = std::max(0, originY - searchHeight / 2);
		int maxRow = std::min(rows, minRow + searchHeight);

		cv::Rect searchRect = cv::Rect(cv::Point{ minCol, minRow }, cv::Point{ maxCol, maxRow });
		cv::Mat searchRegion = img(searchRect);

		// Use thresholding to only consider objects in expected depth range
		cv::Mat mask{ searchRegion.size(), searchRegion.type() };
		inRange(searchRegion, minDepth_mm, maxDepth_mm, mask);

		// Morphology to reduce noise
		cv::Mat mask1{ mask.size(), mask.type() };
		cv::erode(mask, mask1, cv::Mat::ones(3, 3, CV_16UC1), cv::Point(-1, -1), 1);
		cv::dilate(mask1, mask, cv::Mat::ones(3, 3, CV_16UC1), cv::Point(-1, -1), 1);
		deb_mask = mask;

		// Find a series of points which outline the shapes in the mask.
		std::vector<std::vector<cv::Point>> contours;
		std::vector<cv::Vec4i> hierarchy;
		findContours(mask, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

		if (contours.empty()) {
			return invalidRet;
		}

		// Consider contours in order of decreasing point count
		sort(contours.begin(), contours.end(), contourPredicate);

		// Fit a circle to the points of the contour enclosing features which could be the ball.
		// Record the first one to have a radius and origin in the expected ranges.
		std::pair<cv::Point2f, float> result = invalidRet;
		std::vector<std::vector<cv::Point>> validContours;
		cv::Rect originRegion = getCenteredRect(origin_px, originTolerance_px, originTolerance_px);

		for (auto contour = contours.begin(); contour != contours.end(); contour++) {
			float radius;
			cv::Point2f center;
			cv::minEnclosingCircle(*contour, center, radius);
			cv::Point2f absCenter{ center.x + searchRect.x, center.y + searchRect.y };

			if (originRegion.contains(absCenter) && radius >= minRadius_px && radius <= maxRadius_px) {
				result = { absCenter, radius };
				break;
			}
		}

		return result;
	}


	// Use the current state and the given depth image and time since last
	// frame to estimate the region in which the marker will be found.
	SearchDescription getSearchDescription(cv::Mat depthData, double timeDiff) {
		UINT16 minDepth;
		UINT16 maxDepth;
		float minRadius;
		float maxRadius;
		cv::Point2f origin;

		switch (m_state) {
		case STATES::INITIALIZING: {
			minDepth = m_initDepth_mm - m_radius_mm;
			maxDepth = m_initDepth_mm + m_radius_mm;
			minRadius = getObjectHeight_px(m_vfov_rad, m_radius_mm, maxDepth, depthData.rows) / (1 + m_radiusTolerance);
			maxRadius = getObjectHeight_px(m_vfov_rad, m_radius_mm, minDepth, depthData.rows) * (1 + m_radiusTolerance);
			origin = m_initOrigin;
			break;
		}
		case STATES::TRACKING: {
			m_filter.transitionMatrix.at<float>(2) = timeDiff;
			m_filter.transitionMatrix.at<float>(9) = timeDiff;
			m_filter.transitionMatrix.at<float>(30) = timeDiff;

			cv::Mat prediction = m_filter.predict();

			float predX = prediction.at<float>(0);
			float predY = prediction.at<float>(1);
			float predRad = prediction.at<float>(4);

			// Get the depth readings for the area which should contain the ball
			int predictionDepth = (int)getMatMedian(depthData(getCenteredRect({ (int)predX, (int)predY }, std::max((int)predRad / 2, 1), std::max((int)predRad / 2, 1), 0, depthData.cols, 0, depthData.rows)));

			deb_point = cv::Point((int)predX, (int)predY);

			minDepth = predictionDepth - m_radius_mm;
			maxDepth = predictionDepth + m_radius_mm;
			minRadius = predRad / (1 + m_radiusTolerance);
			maxRadius = predRad * (1 + m_radiusTolerance);
			origin = cv::Point2f(predX, predY);

			break;
		}
		}

		return{ minDepth,
			maxDepth,
			minRadius,
			maxRadius,
			origin };
	}


	// Adds observations to the Kalman filter
	void correctFilter(std::pair<cv::Point2f, float> ball) {
		m_markerMeasurement.at<float>(0) = ball.first.x;
		m_markerMeasurement.at<float>(1) = ball.first.y;
		m_markerMeasurement.at<float>(2) = ball.second;
		m_markerMeasurement.at<float>(3) = 0;

		m_filter.correct(m_markerMeasurement);
	}

	// Uses the identified marker position and the time since the previous
	// to update internal state (including updating the Kalman filter)
	void processResults(std::pair<cv::Point2f, float> ball, double timeDiff) {
		if (ball.second == -1) { // Not detected
			switch (m_state) {
			case STATES::INITIALIZING: {
				m_initCounter = 0;
				break;
			}
			case STATES::TRACKING: {
				m_missCounter++;
				if (m_missCounter < 5) {
					//cv::Mat prediction = m_filter.predict();
					//correctFilter({ {prediction.at<float>(0), prediction.at<float>(1)}, prediction.at<float>(4) });
					//std::cout << "NOT FOUND, GUESSING!\n";
				} else {
					//std::cout << "Resetting\n";
					m_state = STATES::INITIALIZING;
					m_initCounter = 0;
				}
				break;
			}
			}

			return;
		}

		switch (m_state) {
		case STATES::INITIALIZING: {
			if (m_initCounter == 0) {
				initMarkerKalmanFilter(ball);
			} else {
				m_filter.transitionMatrix.at<float>(2) = timeDiff;
				m_filter.transitionMatrix.at<float>(9) = timeDiff;
				m_filter.transitionMatrix.at<float>(30) = timeDiff;

				cv::Mat prediction = m_filter.predict();
				correctFilter(ball);
			}

			m_initCounter++;
			if (m_initCounter >= 5) {
				m_state = STATES::TRACKING;
				m_missCounter = 0;
			}
			break;
		}
		case STATES::TRACKING: {
			m_missCounter = 0;

			correctFilter(ball);
			break;
		}
		}
	}
};
