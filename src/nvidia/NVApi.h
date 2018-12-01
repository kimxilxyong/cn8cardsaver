/*
NVApi defines
*/

#ifndef __NVAPI_H__
#define __NVAPI_H__

#include "common/log/Log.h"

#define NV_GPU_COOLER_POLICY_MANUAL 1
#define NV_GPU_COOLER_POLICY_AUTO   16

bool NVApiInit();
bool NVAPI_SetFanPercent(NvPhysicalGpuHandle handle, int percent);
bool NVAPI_GetFanPercent(NvPhysicalGpuHandle handle, int *percent, int *policy);
bool NVApiClose();


#endif __NVAPI_H__