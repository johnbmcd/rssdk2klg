// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "pxcsensemanager.h"
#include "miniz.c"

#include <iostream>
#include <fstream>
#include <vector>

#include <opencv2\opencv.hpp>

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

int main()
{
	PXCSenseManager* sm = PXCSenseManager::CreateInstance();

	pxcCHAR *filename = L"D:\\Users\\John\\Data\\RealSense\\maynooth_office.rssdk";

	PXCCaptureManager *cm = sm->QueryCaptureManager();
	cm->SetFileName(filename, false);
	sm->EnableStream(PXCCapture::STREAM_TYPE_ANY, 0, 0);
	sm->Init();

	PXCCapture::Device *device = cm->QueryDevice();

	int numprofiles = device->QueryStreamProfileSetNum(PXCCapture::STREAM_TYPE_ANY);
	PXCCapture::Device::StreamProfileSet *profiles = new PXCCapture::Device::StreamProfileSet[numprofiles];
	device->QueryStreamProfileSet(profiles);
	delete[] profiles;

	// Set realtime=true and pause=false

	sm->QueryCaptureManager()->SetRealtime(false);

	sm->QueryCaptureManager()->SetPause(true);

	pxcI32 nframes = cm->QueryNumberOfFrames();

	int i = 0;

	PXCImage *color = nullptr, *depth = nullptr;
	PXCCapture::Sample* sample = nullptr;
	while (((sample == NULL)||(sample->color == NULL)||(sample->depth == NULL)) && (i < nframes)) {
		// Set to work on every 3rd frame of data
		cm->SetFrameByIndex(i);
		sm->FlushFrame();

		// Ready for the frame to be ready
		pxcStatus sts = sm->AcquireFrame(true);

		if (sts < PXC_STATUS_NO_ERROR) exit(-1);

		// Retrieve the sample and work on it. The image is in sample->color.
		sample = sm->QuerySample();
		++i;
	}
	--i;

	if ((sample == NULL) || (sample->color == NULL) || (sample->depth == NULL)) {
		std::cout << "No frames found in the reported " << nframes << std::endl;
		exit(-1);
	}
	else {
		std::cout << "First frame at " << i << " containing ";
		if (sample->color!=NULL) std::cout << "colour";
		else {
			std::cout << "no colour";
			exit(-1);
		}
		std::cout << " and ";
		if (sample->depth != NULL) std::cout << "depth";
		else {
			std::cout << "no depth";
			exit(-1);
		}
		std::cout << std::endl;
	}

	color = sample->color;
	depth = sample->depth;

	PXCImage::ImageInfo info = color->QueryInfo();
	PXCImage::ImageInfo infod = depth->QueryInfo();
	sm->ReleaseFrame();
	// Streaming loop
	/*cv::namedWindow("frame", CV_WINDOW_NORMAL);*/

	std::wstring klgfilename = L"D:\\Users\\John\\Data\\RealSense\\maynooth_office.klg";
	std::ofstream klgfile(klgfilename, std::ios::binary);
	klgfile.write((const char *)&nframes,sizeof(int32_t));

	std::cout << "RGB resolution: " << info.width << ", " << info.height << std::endl;
	std::cout << "Depth resolution: " << infod.width << ", " << infod.height << std::endl;

	unsigned char * intermediate_buffer = new unsigned char[info.height*info.width * 3];

	cv::Mat cv_frame(info.height, info.width, CV_8UC3, intermediate_buffer);

	PXCProjection *projection = sm->QueryCaptureManager()->QueryDevice()->CreateProjection();
	PXCCalibration *calibration = projection->QueryCalibration();
	PXCCalibration::StreamCalibration strcalibration;
	PXCCalibration::StreamTransform strtransformation;

	calibration->QueryStreamProjectionParametersEx(PXCCapture::STREAM_TYPE_DEPTH,
		PXCCapture::Device::STREAM_OPTION_STRONG_STREAM_SYNC,
		&strcalibration,
		&strtransformation);

	std::cout << "Stream Calibration Parameters: " << std::endl;
	std::cout << "fx : " << strcalibration.focalLength.x;
	std::cout << "fy : " << strcalibration.focalLength.y;
	std::cout << "u : " << strcalibration.principalPoint.x;
	std::cout << "v : " << strcalibration.principalPoint.y;

	int32_t read_frames = 0;
	for (int i = 0; i < nframes; ++i) {
		std::cout << "Processing frame " << i << std::endl;
		// Set to work on every 3rd frame of data
		sm->QueryCaptureManager()->SetFrameByIndex(i);
		sm->FlushFrame();

		// Ready for the frame to be ready
		pxcStatus sts = sm->AcquireFrame(true);

		if (sts < PXC_STATUS_NO_ERROR) break;

		// Retrieve the sample and work on it. The image is in sample->color.
		PXCCapture::Sample* sample = sm->QuerySample();
		if (sample == NULL) continue;

		PXCImage *color = sample->color;
		if (color == NULL) continue;
		PXCImage::ImageInfo info = color->QueryInfo();
		PXCImage::ImageData imdata;
		color->AcquireAccess(PXCImage::ACCESS_READ, &imdata);

		PXCImage *depth = sample->depth;
		if (depth == NULL) continue;
		PXCImage::ImageInfo infod = depth->QueryInfo();
		PXCImage::ImageData depthdata;
		depth->AcquireAccess(PXCImage::ACCESS_READ, &depthdata);

		++read_frames;

		if (imdata.format == PXCImage::PIXEL_FORMAT_RGB32) {
			//Write out image data
			size_t png_len = 0;


			for (int px = 0; px < info.width*info.height; ++px) {
				intermediate_buffer[px * 3] = imdata.planes[0][px * 4];
				intermediate_buffer[px * 3 + 1] = imdata.planes[0][px * 4 + 1];
				intermediate_buffer[px * 3 + 2] = imdata.planes[0][px * 4 + 2];
			}

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


			unsigned long resampled_depth_size = info.width * info.height * sizeof(uint16_t);
			PXCImage *resampled_depth = projection->CreateDepthImageMappedToColor(depth, color);
			unsigned long depth_buffer_size = compressBound(resampled_depth_size);
			uint8_t *depth_buffer = new uint8_t[depth_buffer_size];

			PXCImage::ImageData resampled_depth_data;

			resampled_depth->AcquireAccess(PXCImage::ACCESS_READ, &resampled_depth_data);

			//memcpy(depth_buffer, resampled_depth, resampled_depth_size);
			if (compress2(depth_buffer, &depth_buffer_size, resampled_depth_data.planes[0], info.width * info.height * sizeof(uint16_t), Z_BEST_SPEED) != Z_OK) {
				std::cerr << __FILE__ << ", " << __LINE__ << ": compress2 failed" << std::endl;
				exit(-1);
			}
			 
			resampled_depth->ReleaseAccess(&resampled_depth_data);

			const char *encp = reinterpret_cast<const char *>(encoded_frame.data());
			logData(klgfile,
				static_cast<int64_t>(color->QueryTimeStamp()),
				static_cast<uint32_t>(depth_buffer_size),
				encoded_frame.size(),
				(const char *)depth_buffer,
				encp);

			delete[] depth_buffer;
			resampled_depth->Release();

			depth->ReleaseAccess(&depthdata);
			color->ReleaseAccess(&imdata);


		}
		// Resume processing the next frame
		sm->ReleaseFrame();
	}

	//klgfile.seekp(0);
	//klgfile.write((char *)&nframes,sizeof(nframes));
	klgfile.close();

	//// Clean up
	projection->Release();
	sm->Release();





	return 0;
}

