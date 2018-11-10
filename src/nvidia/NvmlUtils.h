


#ifndef __NVMLUTILS_H__
#define __NVMLUTILS_H__


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
	static bool DoCooling(int deviceIdx, CoolingContext *cool);
};


#endif // __NVMLUTILS_H__