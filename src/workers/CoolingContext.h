


#ifndef __COOLINGCONTEXT_H__
#define __COOLINGCONTEXT_H__

#ifdef __linux__
//#include <X11/Xlib.h>
#else
#include "3rdparty/nvapi/nvapi.h"
#endif


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
	void *dpy;
#endif
} CoolingContext;


#endif // __NVMLUTILS_H__