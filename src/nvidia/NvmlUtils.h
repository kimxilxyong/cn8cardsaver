


#ifndef __NVMLUTILS_H__
#define __NVMLUTILS_H__

#ifdef __linux__
#include <X11/Xlib.h>
#else
#include "3rdparty/nvapi/nvapi.h"
#endif
#include "workers/CudaThread.h"
#include "workers/CoolingContext.h"

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

#ifdef __linux__
    static int GetTickCount(void);
#endif	

private:
#ifndef __linux__
	static NvPhysicalGpuHandle physHandle[NVAPI_MAX_PHYSICAL_GPUS];
#endif

};


#endif // __NVMLUTILS_H__