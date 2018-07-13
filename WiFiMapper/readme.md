# WiFi Mapper
A program for a PC to track the position of, and collect information from a Wi-Fi scanning device. Requires a Microsoft Kinect Sensor v2 and ESP8266 microcontrollers.

## Files
The notable files contained in this project are: 
* `MarkerTracker.h` - the (header-only) module responsible for tracking a single sphere in a series of depth images (the file containing the implemented tracking algorithm).
* `PointCloud.h` - the (header-only) module responsible for combining position and signal strength data as a [PCD file](http://pointclouds.org/documentation/tutorials/pcd_file_format.php).
* `WiFiMapper.cpp` - the main module of the program (initialises hardware, and uses the remaining modules).
* `WiFiMapper.h` - the header file which defines most of the parameters used in the project. 
* `WiFiMapper.sln` - the Microsoft Visual Studio Solution file which defines how the included modules are built.
* `WiFiReceiver.cpp` - the module responsible for communication with a single ESP8266 microcontroller (in a separate thread).
* `WiFiReceiver.h` - the header file defining the WiFiReceiver class.

### Additional Files
Inside the Clouds subdirectory, the following files exist:
* `color_mapper.py` - a python script to map the collected RSSI values to a wider colour range.
* `pcd_kinematics.py` - a python script which calculates the distance travelled by the scanner in a point cloud, and the average speed at which it was recorded.

## Installation
This project depends on the following software packages:
* Microsoft Visual Studio (available [here](https://www.visualstudio.com/downloads/))
* Kinect for Windows SDK v2.0 (available [here](https://www.microsoft.com/en-us/download/details.aspx?id=44561))
* OpenCV (available [here](https://opencv.org/releases.html))

Please ensure that the following environment variables are set:
* `OPENCV_DIR` (example value: `C:\Program Files\opencv\build\x64\vc14`)
* `KINECTSDK20_DIR` (example value: `C:\Program Files\Microsoft SDKs\Kinect\v2.0_1409\`)
And ensure that `OPENCV_DIR`'s value is added to `Path`.

Also required are the following hardware resources:
* Microsoft Kinect Sensor v2
* A Wi-Fi scanning device consisting of 2 ESP8266 microcontrollers with the (included) SignalStrengthServer program installed (see the included report for details).

## Build
With the required software packages installed, the WiFiMapper executable may be built by:
1. Opening the Microsoft Visual Studio solution file WiFiMapper.sln in Microsoft Visual Studio
2. Setting the current IP address of the two ESP8266 microcontrollers in `WiFiMapper.h`
3. Selecting a build configuration (Debug or Release) Note: 64-bit to use the Kinect for Windows SDK
4. Selecting Build > Build Solution

## Use
Once the solution has built successfully, an executable file may be found as `./x64/[Debug or Release]/WiFiMapper.exe`. This may be run like any other executable file.

On startup, the Kinect Sensor is initialised and used to collect a coloured point cloud of the environment. Once this has completed (taking around 15 seconds), three windows will appear in addition to the console window - one showing the entire depth image, and two showing the regions scanned for visual markers.

Position each of the visual markers in the region indicated by two circles in turn (where left corresponds wo Marker A, and right to Marker B). Once both markers are indicated as tracked, begin collecting Wi-Fi signal strength samples by moving the scanner through the areas of interest.

At any time while one of the three image windows are in focus, pressing the following keys will trigger the following actions:
* `S` - print statistics to the terminal window
* `Q` - quit the program, saving any collected point cloud to the directory `./Clouds`

### Output
The WiFiMapper program generates two types of files (both in the `./Clouds` directory):
* `env_map [timestamp].pcd` - the coloured point cloud of the scanned environment.
* `map [timestamp].pcd` - the point cloud containing the collected RSSI and position information.

## Authors
**Marc Katzef** - mka122@uclive.ac.nz
(Based on the Kinect SDK 2.0 demo programs "Color Basics-D2D" and "Depth Basics-D2D")