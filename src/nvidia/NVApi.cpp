

#include <uv.h>
#include <cmath>
#include <thread>
#include <windows.h>
//#include <iostream>
#include "3rdparty/nvapi/nvapi.h"
#include "nvidia/NVApi.h"
//#include "common/log/Log.h"

// magic numbers, do not change them
#define NVAPI_MAX_PHYSICAL_GPUS   64
#define NVAPI_MAX_USAGES_PER_GPU  34

#define NVAPI_MAX_COOLERS_PER_GPU   20
typedef struct {
	NvU32 version;
	NvU32 count;
	struct {
		NvS32 type;
		NvS32 controller;
		NvS32 defaultMin;
		NvS32 defaultMax;
		NvS32 currentMin;
		NvS32 currentMax;
		NvS32 currentLevel;
		NvS32 defaultPolicy;
		NvS32 currentPolicy;
		NvS32 target;
		NvS32 controlType;
		NvS32 active;
	} cooler[NVAPI_MAX_COOLERS_PER_GPU];
} NV_GPU_COOLER_SETTINGS_V2;
typedef NV_GPU_COOLER_SETTINGS_V2   NV_GPU_COOLER_SETTINGS;
#define NV_GPU_COOLER_SETTINGS_VER_2    MAKE_NVAPI_VERSION(NV_GPU_COOLER_SETTINGS_V2,2)
#define NV_GPU_COOLER_SETTINGS_VER      NV_GPU_COOLER_SETTINGS_VER_2

typedef struct {
	NvU32 version;
	struct {
		NvS32 level;
		NvS32 policy;
	} cooler[NVAPI_MAX_COOLERS_PER_GPU];
} NV_GPU_COOLER_LEVELS_V1;
typedef NV_GPU_COOLER_LEVELS_V1 NV_GPU_COOLER_LEVELS;
#define NV_GPU_COOLER_LEVELS_VER_1  MAKE_NVAPI_VERSION(NV_GPU_COOLER_LEVELS_V1,1)
#define NV_GPU_COOLER_LEVELS_VER    NV_GPU_COOLER_LEVELS_VER_1

// function pointer types
typedef int *(*NvAPI_QueryInterface_t)(unsigned int offset);
typedef int(*NvAPI_Initialize_t)();
typedef int(*NvAPI_EnumPhysicalGPUs_t)(int **handles, int *count);
typedef int(*NvAPI_GPU_GetUsages_t)(int *handle, unsigned int *usages);

typedef int(*NvAPI_GPU_SetCoolerLevel_t)(int *handle, NvU32 coolerIndex, NV_GPU_COOLER_LEVELS* coolerLevel);
typedef int(*NvAPI_GPU_GetCoolersSettings_t)(int *handle, NvU32 coolerIndex, NV_GPU_COOLER_SETTINGS* coolerSettings);


NvAPI_GPU_SetCoolerLevel_t     NvAPI_GPU_SetCoolerLevel = NULL;
NvAPI_GPU_GetCoolersSettings_t NvAPI_GPU_GetCoolersSettings = NULL;

HMODULE hmod = NULL;

bool NVApiClose()
{
	if (FreeLibrary(hmod)) {
		hmod = NULL;
		return true;
	}
	return false;
}

bool NVApiInit()
{
	if (hmod == NULL)
		hmod = LoadLibraryA("nvapi64.dll");
	if (hmod == NULL)
	{
		LOG_ERR("Couldn't find nvapi64.dll");
		return false;
	}

	// nvapi.dll internal function pointers
	NvAPI_QueryInterface_t         NvAPI_QueryInterface = NULL;
	NvAPI_Initialize_t             NvAPI_Initialize = NULL;
	NvAPI_EnumPhysicalGPUs_t       NvAPI_EnumPhysicalGPUs = NULL;
	NvAPI_GPU_GetUsages_t          NvAPI_GPU_GetUsages = NULL;
	
	
	
	// nvapi_QueryInterface is a function used to retrieve other internal functions in nvapi.dll
	NvAPI_QueryInterface = (NvAPI_QueryInterface_t)GetProcAddress(hmod, "nvapi_QueryInterface");

	// some useful internal functions that aren't exported by nvapi.dll
	NvAPI_Initialize = (NvAPI_Initialize_t)(*NvAPI_QueryInterface)(0x0150E828);
	NvAPI_EnumPhysicalGPUs = (NvAPI_EnumPhysicalGPUs_t)(*NvAPI_QueryInterface)(0xE5AC921F);
	NvAPI_GPU_GetUsages = (NvAPI_GPU_GetUsages_t)(*NvAPI_QueryInterface)(0x189A1FDF);

	NvAPI_GPU_GetCoolersSettings = (NvAPI_GPU_GetCoolersSettings_t)(*NvAPI_QueryInterface)(0xDA141340);
	NvAPI_GPU_SetCoolerLevel = (NvAPI_GPU_SetCoolerLevel_t)(*NvAPI_QueryInterface)(0x891FA0AE);




	if (NvAPI_Initialize == NULL || NvAPI_EnumPhysicalGPUs == NULL ||
		NvAPI_EnumPhysicalGPUs == NULL || NvAPI_GPU_GetUsages == NULL ||
		NvAPI_GPU_GetCoolersSettings == NULL || NvAPI_GPU_SetCoolerLevel == NULL)
	{
		LOG_ERR("Couldn't get functions in nvapi64.dll");
		return false;
	}

	// initialize NvAPI library, call it once before calling any other NvAPI functions
	(*NvAPI_Initialize)();

	int          gpuCount = 0;
	int         *gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { NULL };
	unsigned int gpuUsages[NVAPI_MAX_USAGES_PER_GPU] = { 0 };

	// gpuUsages[0] must be this value, otherwise NvAPI_GPU_GetUsages won't work
	gpuUsages[0] = (NVAPI_MAX_USAGES_PER_GPU * 4) | 0x10000;

	int ret = (*NvAPI_EnumPhysicalGPUs)(gpuHandles, &gpuCount);
	if (ret != NVAPI_OK)
	{
		LOG_ERR("Failed NvAPI_EnumPhysicalGPUs GPU Count %i", gpuCount);
		return false;
	}

	return true;

	// Set fan level /////
	int percent = 89;
	NV_GPU_COOLER_LEVELS coolerLvl;
	coolerLvl.cooler[0].policy = 1;
	coolerLvl.version = NV_GPU_COOLER_LEVELS_VER;
	coolerLvl.cooler[0].level = percent;

	ret = (*NvAPI_GPU_SetCoolerLevel)(gpuHandles[0], 0, &coolerLvl);
	if (ret == NVAPI_OK)
	{
		int level = coolerLvl.cooler[0].level;
		LOG_INFO("NvAPI_GPU_SetCoolerLevel level %i", level);
	}


	// Get Fan level /////
	NV_GPU_COOLER_SETTINGS coolerSettings;
	coolerSettings.version = NV_GPU_COOLER_SETTINGS_VER;

	for (int i = 0; i < 100; i++) {
		int ret = (*NvAPI_GPU_GetCoolersSettings)(gpuHandles[0], 0, &coolerSettings);
		if (ret == NVAPI_OK)
		{
			int level = coolerSettings.cooler[0].currentLevel;
			LOG_INFO("GetCoolersSettings level %i", level);
		}
		return 0;
	}



	// print GPU usage every second
	for (int i = 0; i < 100; i++)
	{
		(*NvAPI_GPU_GetUsages)(gpuHandles[0], gpuUsages);
		int usage = gpuUsages[3];
		LOG_INFO("GPU Usage: %i", usage);
		Sleep(1000);
	}

	return 0;
}

bool NVAPI_SetFanPercent(NvPhysicalGpuHandle handle, int percent)
{
	// Set fan level /////
	NV_GPU_COOLER_LEVELS coolerLvl;
	coolerLvl.version = NV_GPU_COOLER_LEVELS_VER;
	if (percent == 0) {
		coolerLvl.cooler[0].policy = NV_GPU_COOLER_POLICY_AUTO;  // 16;
	}
	else {
		coolerLvl.cooler[0].policy = NV_GPU_COOLER_POLICY_MANUAL;// 1;
	}
	coolerLvl.cooler[0].level = percent;

	int ret = (*NvAPI_GPU_SetCoolerLevel)((int *)handle, 0, &coolerLvl);
	if (ret != NVAPI_OK)
	{
		int level = coolerLvl.cooler[0].level;
		LOG_ERR("Failed NvAPI_GPU_SetCoolerLevel level %i", level);
		return false;
	}
	else {
		//LOG_INFO("NVAPI_SetFanPercent level %i policy %i", coolerLvl.cooler[0].level, coolerLvl.cooler[0].policy);
	}
	return true;
}

bool NVAPI_GetFanPercent(NvPhysicalGpuHandle handle, int *percent, int *policy)
{
	// Get Fan level /////
	NV_GPU_COOLER_SETTINGS coolerSettings;
	coolerSettings.version = NV_GPU_COOLER_SETTINGS_VER;

	int ret = (*NvAPI_GPU_GetCoolersSettings)((int *)handle, 0, &coolerSettings);
	if (ret != NVAPI_OK) {
		int level = coolerSettings.cooler[0].currentLevel;
		LOG_ERR("Failed GetCoolersSettings level %i", level);
	}
	else {
		if (percent != NULL) {
			*percent = coolerSettings.cooler[0].currentLevel;
			//LOG_INFO("NvAPI_GPU_GetCoolersSettings count %i level %i target %i policy %i", coolerSettings.count, coolerSettings.cooler[0].currentLevel, coolerSettings.cooler[0].target, coolerSettings.cooler[0].currentPolicy);

			if (policy != NULL) {
				*policy = coolerSettings.cooler[0].currentPolicy;
			}
				
			return true;
		}
		
	}
	return false;

}