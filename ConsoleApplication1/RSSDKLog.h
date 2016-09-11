#pragma once
#include "pxcsensemanager.h"

#include <iostream>

class RSSDKLog
{
public:
	RSSDKLog(const std::wstring &filename);
	virtual ~RSSDKLog();

	inline int32_t currframe() const;
	inline int32_t width() const { return info.width; };
	inline int32_t height() const { return info.height; };

	bool nextFrame();
	bool setFrame(int32_t i);

protected:
	void initProjection();
	void getCalibration();

	std::wstring filename;

	PXCSenseManager* sm;
	PXCCaptureManager* cm;

	pxcI32 nframes;
	pxcI32 curr_frame;

	PXCImage *color = nullptr;
	PXCImage *depth = nullptr;
	PXCCapture::Sample* sample = nullptr;

	PXCImage::ImageInfo info;
	PXCImage::ImageInfo infod;

	PXCProjection *projection;

	PXCCalibration *calibration;
	PXCCalibration::StreamCalibration strcalibration;
	PXCCalibration::StreamTransform strtransformation;
};

