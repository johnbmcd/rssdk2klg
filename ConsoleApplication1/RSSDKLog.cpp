#include "stdafx.h"

#include "RSSDKLog.h"

#include <iostream>
#include <fstream>
#include <vector>

RSSDKLog::RSSDKLog(const std::wstring &filename):
	filename(filename),
	sm(PXCSenseManager::CreateInstance()),
	cm(sm->QueryCaptureManager()),
	curr_frame(0)
{
	cm->SetFileName(filename.c_str(), false);
	sm->EnableStream(PXCCapture::STREAM_TYPE_ANY, 0, 0);
	sm->Init();

	sm->QueryCaptureManager()->SetRealtime(false);

	sm->QueryCaptureManager()->SetPause(true);

	nframes = cm->QueryNumberOfFrames();

	setFrame(curr_frame);
}

inline int32_t RSSDKLog::currframe() const
{
	return int32_t(curr_frame);
}

bool RSSDKLog::nextFrame() {
	if (curr_frame + 1 >= nframes) return false;

	++curr_frame;
	setFrame(curr_frame);

	return true;
}

bool RSSDKLog::setFrame(int32_t i) {
	if (i >= nframes || i < 0) return false;

	if (sample != NULL) {
		resampled_depth->ReleaseAccess(&rsddata);
		resampled_depth->Release();

		depth->ReleaseAccess(&ddata);
		color->ReleaseAccess(&cdata);

		sm->ReleaseFrame();
	}

	cm->SetFrameByIndex(i);
	sm->FlushFrame();

	// Ready for the frame to be ready
	pxcStatus sts = sm->AcquireFrame(true);

	if (sts < PXC_STATUS_NO_ERROR) exit(-1);

	// Retrieve the sample and work on it. The image is in sample->color.
	sample = sm->QuerySample();

	color = sample->color;
	depth = sample->depth;

	color->AcquireAccess(PXCImage::ACCESS_READ, &cdata);
	depth->AcquireAccess(PXCImage::ACCESS_READ, &ddata);

	info = color->QueryInfo();
	infod = depth->QueryInfo();

	resampled_depth = projection->CreateDepthImageMappedToColor(depth, color);
	resampled_depth->AcquireAccess(PXCImage::ACCESS_READ, &rsddata);

	return true;
}

bool RSSDKLog::getImageData(unsigned char * data)
{
	if (cdata.format == PXCImage::PIXEL_FORMAT_RGB32) {
		//Write out image data
		size_t png_len = 0;


		for (int px = 0; px < info.width*info.height; ++px) {
			data[px * 3] = cdata.planes[0][px * 4];
			data[px * 3 + 1] = cdata.planes[0][px * 4 + 1];
			data[px * 3 + 2] = cdata.planes[0][px * 4 + 2];
		}
		return true;
	}
	else
		return false;
}

unsigned char * RSSDKLog::getResampleDepthData()
{
	if (resampled_depth != nullptr)
		return rsddata.planes[0];
	return nullptr;
}

void RSSDKLog::initProjection()
{
	projection = sm->QueryCaptureManager()->QueryDevice()->CreateProjection();
}

void RSSDKLog::getCalibration()
{
	calibration = projection->QueryCalibration();
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
}

RSSDKLog::~RSSDKLog()
{
	projection->Release();

	sm->ReleaseFrame();
	sm->Release();
}
