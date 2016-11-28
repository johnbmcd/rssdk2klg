// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "pxcsensemanager.h"
#include "miniz.c"

#include <iostream>
#include <fstream>
#include <vector>

#include <opencv2\opencv.hpp>

#include "RSSDKLog.h"

void logData(std::ofstream &logfile,
	int64_t timestamp,
	int32_t depthSize,
	int32_t imageSize,
	const char * depthData,
	const char * rgbData) {
	logfile.write((const char *)&timestamp, sizeof(int64_t));
	logfile.write((const char *)&depthSize, sizeof(int32_t));
	logfile.write((const char *)&imageSize, sizeof(int32_t));
	logfile.write(depthData, depthSize);
	logfile.write(rgbData, imageSize);
}

struct MatchPathSeparator
{
	bool operator()(char ch) const
	{
		return ch == '\\' || ch == '/';
	}
};

struct ExtensionSeparator
{
	bool operator()(char ch) const
	{
		return ch == '.';
	}
};

class FilenameParser {
public:
	FilenameParser(std::string filename) :
		path_(filename.begin(),
			std::find_if(filename.rbegin(), filename.rend(), MatchPathSeparator()).base()),
		basename_(std::find_if(filename.rbegin(), filename.rend(), MatchPathSeparator()).base(),
			std::find_if(filename.rbegin(), filename.rend(), ExtensionSeparator()).base()-1),
		extension_(std::find_if(filename.rbegin(), filename.rend(), ExtensionSeparator()).base()-1,
			 filename.end())
	{}

	std::wstring path() { return path_;  }
	std::wstring basename() { return basename_; }
	std::wstring extension() { return extension_; }

	std::wstring filename() { return path_ + basename_ + extension_; }

protected:
	std::wstring path_, basename_, extension_;
};

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " filename" << std::endl;
		exit(-1);
	}

	FilenameParser filenameParser(argv[1]);

	std::wcout << "Input filename: " << filenameParser.filename()<< std::endl;
	RSSDKLog rssdklog(filenameParser.filename());
	
	std::wstring klgfilename = filenameParser.path() + filenameParser.basename() + L".klg";
	std::wcout << "Output filename: " << klgfilename << std::endl;

	std::ofstream klgfile(klgfilename, std::ios::binary);
	int32_t nframes = rssdklog.numframes();
	klgfile.write((const char *)&nframes,sizeof(int32_t));
	std::cout << "Number of frames: " << nframes << std::endl;

	int32_t width = rssdklog.width(), height = rssdklog.height();
	std::cout << "Resolution: " << width << ", " << height << std::endl;
	unsigned char * intermediate_buffer = new unsigned char[height * width * 3];

	cv::Mat cv_frame(height, width, CV_8UC3, intermediate_buffer);

	std::cout << "Stream Calibration Parameters: " << std::endl;
	std::cout << "fx : " << rssdklog.fx() << std::endl;
	std::cout << "fy : " << rssdklog.fy() << std::endl;
	std::cout << "u : " << rssdklog.u() << std::endl;
	std::cout << "v : " << rssdklog.v() << std::endl;

	unsigned long resampled_depth_size = width * height * sizeof(uint16_t);
	unsigned long depth_buffer_size = compressBound(resampled_depth_size);
	uint8_t *depth_buffer = new uint8_t[depth_buffer_size];

	//int32_t read_frames = 0;
	std::cout << "Processing ";
	for (int i = 0; i < nframes; ++i) {
		std::cout << ".";
		if (rssdklog.getImageData(intermediate_buffer)) {
			//Write out image data
			size_t png_len = 0;

			std::vector<int> jpeg_params;
			jpeg_params.push_back(CV_IMWRITE_JPEG_QUALITY);
			jpeg_params.push_back(90);

			cv::imshow("frame", cv_frame);
			cv::waitKey(10);
			std::vector<uchar> encoded_frame;

			if (!cv::imencode(".jpg", cv_frame, encoded_frame, jpeg_params)) {
				std::cerr << __FILE__ << ", " << __LINE__ << ": cv::imencode failed" << std::endl;
				exit(-1);
			}

			unsigned char *resampled_depth_data = rssdklog.getResampleDepthData();

			depth_buffer_size = compressBound(resampled_depth_size);
			if (compress2(depth_buffer, &depth_buffer_size, resampled_depth_data, width * height * sizeof(uint16_t), Z_BEST_SPEED) != Z_OK) {
				std::cerr << __FILE__ << ", " << __LINE__ << ": compress2 failed" << std::endl;
				exit(-1);
			}

			const char *encp = reinterpret_cast<const char *>(encoded_frame.data());
			logData(klgfile,
				rssdklog.getTimeStamp(),
				static_cast<uint32_t>(depth_buffer_size),
				encoded_frame.size(),
				(const char *)depth_buffer,
				encp);
		}
	}
	delete[] depth_buffer;

	klgfile.close();

	return 0;
}

