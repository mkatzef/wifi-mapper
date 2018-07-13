/*
 * The module responsible for formatting position and colour information as point cloud files.
 *
 * Written by Marc Katzef
 */
 
#pragma once

#include <vector>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include "stdafx.h"
#include <strsafe.h>

#include <iostream>
#include <fstream>
#include <sstream>


// A container for position and colour channels
struct ColoredPoint {
	float x;
	float y;
	float z;
	UINT8 r;
	UINT8 g;
	UINT8 b;
};


// An object containing many ColoredPoints, capable of writing them to file
class PointCloud {
public:
	PointCloud() = default;

	// Adds a point at the given position, with the given colour values
	void AddPoint(cv::Point3f point, UINT8 r, UINT8 g, UINT8 b) {
		ColoredPoint newPoint{ point.x, point.y, point.z, r, g, b };
		m_points.emplace_back(newPoint);
	}

	// Writes to PCD file (using WriteToFile). Keeps trying until it is performed
	// successfully (a workaraound to prevent dropbox from interfering mid-write).
	void WriteToFileSafe(std::string filename) {
		bool written = false;
		while (!written) {
			try {
				WriteToFile(filename);
				written = true;
			} catch (const std::exception &e) {
				std::cerr << "Failed to write to: \"" << filename << "\"\nDetails:\n";
				std::cerr << e.what() << "\n";
				std::cout << "New file name: ";
				std::cin >> filename;
				std::cout << "\n";
			}
		}
	}

	// Writes point cloud contents to an ASCII PCD file.
	void WriteToFile(std::string filename) {
		// make an enormous string (bad practise, but allows for atomic write)
		std::stringstream outSStream;
		size_t count = m_points.size();

		outSStream << "VERSION .7\n"
			"FIELDS x y z rgb\n"
			"SIZE 4 4 4 4\n"
			"TYPE F F F U\n"
			"COUNT 1 1 1 1\n"
			"WIDTH " << count << "\n"
			"HEIGHT 1\n"
			"VIEWPOINT 0 0 0 1 0 0 0\n"
			"POINTS " << count << "\n"
			"DATA ascii\n";

		for (size_t i = 0; i < count; i++) {
			ColoredPoint p = m_points[i];
			uint color = p.r;
			color = (color << 8) + p.g;
			color = (color << 8) + p.b;

			outSStream << p.x << " " << p.y << " " << p.z << " " << color << "\n";
		}

		std::ofstream outFile(filename);
		outFile << outSStream.str();
		outFile.close();
	}

private:
	std::vector<ColoredPoint> m_points;
};