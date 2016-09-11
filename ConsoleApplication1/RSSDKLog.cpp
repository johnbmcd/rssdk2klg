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

	pxcI32 nframes = cm->QueryNumberOfFrames();

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
}

bool RSSDKLog::setFrame(int32_t i) {
	if (i >= nframes) return false;

	cm->SetFrameByIndex(i);
	sm->FlushFrame();

	// Ready for the frame to be ready
	pxcStatus sts = sm->AcquireFrame(true);

	if (sts < PXC_STATUS_NO_ERROR) exit(-1);

	// Retrieve the sample and work on it. The image is in sample->color.
	sample = sm->QuerySample();

	color = sample->color;
	depth = sample->depth;

	info = color->QueryInfo();
	infod = depth->QueryInfo();
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
	sm->ReleaseFrame();
}
