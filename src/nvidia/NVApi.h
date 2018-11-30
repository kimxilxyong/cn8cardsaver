/*
NVApi defines
*/

#ifndef __NVAPI_H__
#define __NVAPI_H__

#include "common/log/Log.h"


bool NVApiInit();
bool NVAPI_SetFanPercent(NvPhysicalGpuHandle handle, int percent);
bool NVAPI_GetFanPercent(NvPhysicalGpuHandle handle, int *percent);
bool NVApiClose();


#endif __NVAPI_H__