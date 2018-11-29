


#ifndef __NVMLUTILS_H__
#define __NVMLUTILS_H__

#include <X11/Xlib.h>
#include "workers/CudaThread.h"

typedef struct _CoolingContext {
	int SleepFactor = 0;
	int LastTemp = 0;
	int CurrentTemp = 0;
	int CurrentFanLevel = 0;	// fan speed in percent
	int ScreenCount = 0;
	bool NeedsCooling = false;
	bool FanIsAutomatic = false;
	bool IsFanControlEnabled = false;
	int pciBus = -1;
	int Card = -1;
	Display *dpy;
} CoolingContext;

class NvmlUtils
{
public:

	
	static bool NVCtrlInit(CoolingContext *cool, CudaThread * thread);
    static bool NVCtrlClose(CoolingContext *cool);
	static bool Temperature(CoolingContext *cool);
	static bool DoCooling(CoolingContext *cool);
	static int Get_DeviceID_by_PCI( CoolingContext *cool, CudaThread * thread);
	
	static bool SetFanPercent(CoolingContext *cool, int percent);
	static bool SetFanPercentLinux(CoolingContext *cool, int percent);
	static bool SetFanPercentWindows(CoolingContext *cool, int percent);

	static bool GetFanPercent(CoolingContext *cool, int *percent);
	static bool GetFanPercentLinux(CoolingContext *cool, int *percent);
	static bool GetFanPercentWindows(CoolingContext *cool, int *percent);
};


#endif // __NVMLUTILS_H__