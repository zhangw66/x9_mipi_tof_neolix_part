#pragma once 
#include "libTof.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <android/log.h>

#define NATIVE_WRAPPER_DEBUG
/** 16-bit unsigned integer. */ 
typedef	unsigned short		UInt16_t;
typedef UInt16_t DepthPixel_t;
typedef int BOOL;
#define FALSE 0
#define TRUE 1
#define ROUND_UP(x, align) (x+(align-1))&(~(align-1))
//typedef void (*getDepthFunc_t)(void *, int);
typedef struct  {
	float x;
	float y;
	float z;
	float noise;
} sunnySpectrePCL_t;

typedef struct {
	sunnySpectrePCL_t *pPCL;
	unsigned int pcl_data_size;
	DepthPixel_t * pDepthData;
	unsigned int  depth_data_size;
	short * pVideoData;
	unsigned int  video_data_size;
	FrameDataRgb_t *pGrayData;
	unsigned int  gray_data_size;
} tofSensorAllData_t;

typedef enum {
	PREVIEW_START = 0,
	PREVIEW_STOP
} PREVIEW_STATUS;
typedef void (*getDepthFunc_t)(tofSensorAllData_t&);


namespace sunny {
	void setPreviewStatus(PREVIEW_STATUS sta);
	void registerJNIGetDepthCB(getDepthFunc_t getDepth);
	int connect_mipitof(DeviceInfo_t &devInfo);
	int disconnect_mipitof();
}

