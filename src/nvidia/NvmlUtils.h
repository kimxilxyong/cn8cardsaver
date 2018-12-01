


#ifndef __NVMLUTILS_H__
#define __NVMLUTILS_H__

#ifdef __linux__
#include <X11/Xlib.h>
#else
#include "3rdparty/nvapi/nvapi.h"
#endif
#include "workers/CudaThread.h"

typedef struct _CoolingContext {
	int SleepFactor = 0;
	int LastTemp = 0;
	int LastTick = 0;
	int CurrentTemp = 0;
	int CurrentFanLevel = 0;	// fan speed in percent
	int ScreenCount = 0;
	bool NeedsCooling = false;
	bool FanIsAutomatic = false;
	bool IsFanControlEnabled = false;
	int pciBus = -1;
	int Card = -1;
#ifdef __linux__
	Display *dpy;
#endif
} CoolingContext;

class NvmlUtils
{
public:

	static bool NVCtrlInit(CoolingContext *cool, CudaThread * thread);
    static bool NVCtrlClose(CoolingContext *cool);
	static bool Temperature(CoolingContext *cool);
	static bool DoCooling(CoolingContext *cool);
	static bool Get_DeviceID_by_PCI( CoolingContext *cool, CudaThread * thread);
	
	static bool SetFanPercent(CoolingContext *cool, int percent);
	static bool SetFanPercentLinux(CoolingContext *cool, int percent);
	static bool SetFanPercentWindows(CoolingContext *cool, int percent);

	static bool GetFanPercent(CoolingContext *cool, int *percent);
	static bool GetFanPercentLinux(CoolingContext *cool, int *percent);
	static bool GetFanPercentWindows(CoolingContext *cool, int *percent);

private:
	static NvPhysicalGpuHandle physHandle[NVAPI_MAX_PHYSICAL_GPUS];
};


#endif // __NVMLUTILS_H__