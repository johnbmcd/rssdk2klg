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
	inline int32_t numframes() const { return nframes; };
	inline int64_t getTimeStamp() const { return color->QueryTimeStamp(); };

	inline float fx() const { return strcalibration.focalLength.x; };
	inline float fy() const { return strcalibration.focalLength.y; };
	inline float u() const { return strcalibration.principalPoint.x; };
	inline float v() const { return strcalibration.principalPoint.y; };

	bool nextFrame();
	bool setFrame(int32_t i);

	bool getImageData(unsigned char *data);
	unsigned char * getResampleDepthData();

protected:
	void initProjection();
	void initCalibration();

	std::wstring filename;

	PXCSenseManager* sm;
	PXCCaptureManager* cm;

	pxcI32 nframes;
	pxcI32 curr_frame;

	PXCImage *color = nullptr;
	PXCImage *depth = nullptr;
	PXCImage *resampled_depth = nullptr;
	PXCCapture::Sample* sample = nullptr;

	PXCImage::ImageInfo info;
	PXCImage::ImageInfo infod;

	PXCImage::ImageData cdata; //colour data
	PXCImage::ImageData ddata; //depth data
	PXCImage::ImageData rsddata; // resampled depth data

	PXCProjection *projection;

	PXCCalibration *calibration;
	PXCCalibration::StreamCalibration strcalibration;
	PXCCalibration::StreamTransform strtransformation;
};

