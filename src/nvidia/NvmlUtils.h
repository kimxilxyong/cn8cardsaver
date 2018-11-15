


#ifndef __NVMLUTILS_H__
#define __NVMLUTILS_H__

#include "workers/CudaThread.h"

typedef struct _CoolingContext {
	int SleepFactor = 0;
	int LastTemp = 0;
	int Temp = 0;
	bool NeedsCooling = false;
	int card = -1;
} CoolingContext;

class NvmlUtils
{
public:

	//static GpuContext *m_ctx;
	static bool Temperature(int id, CoolingContext *cool);
	static bool DoCooling(int id, CoolingContext *cool, CudaThread * thread, std::vector<xmrig::IThread*> &threads);
};


#endif // __NVMLUTILS_H__