/* cn8cardsaver
 * Copyright 2018-2019 KimIL       <https://github.com/kimxilxyong>, <kimxilxyong@gmail.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <uv.h>
#include <cmath>
#include <thread>

#ifdef __linux__
#include <unistd.h>
#include <X11/Xlib.h>
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>
#else
#include "nvidia/NVApi.h"
#endif

#include "nvidia/NvmlUtils.h"
#include "nvidia/NvmlApi.h"

#include "nvidia/Health.h"

#include "common/log/Log.h"
#include "workers/Workers.h"
#include "workers/CudaThread.h"

#ifndef __linux__
NvPhysicalGpuHandle NvmlUtils::physHandle[NVAPI_MAX_PHYSICAL_GPUS];
#endif

#ifdef __linux__
bool NvmlUtils::NVCtrlInit(CoolingContext *cool, CudaThread * thread)
{
 
    Bool r;
    int event_base, error_base, major, minor, screens, i;
    char *str;
        
    /*
     * open a connection to the X server indicated by the DISPLAY
     * environment variable
     */
    
    cool->dpy = XOpenDisplay(NULL);
    if (!cool->dpy) {
        LOG_ERR("Cannot open display '%s'.\n", XDisplayName(NULL));
        return 1;
    }
    
    /*
     * check if the NV-CONTROL X extension is present on this X server
     */

    r = XNVCTRLQueryExtension(cool->dpy, &event_base, &error_base);
    if (r != true) {
        LOG_ERR("The NV-CONTROL X extension does not exist on '%s'.\n", XDisplayName(NULL));
        return r;
    }

    /*
     * query the major and minor extension version
     */

    r = XNVCTRLQueryVersion(cool->dpy, &major, &minor);
    if (r != True) {
        LOG_ERR("The NV-CONTROL X extension does not exist on '%s'.\n", XDisplayName(NULL));
        return r;
    }

    /*
     * print statistics thus far
     */

    LOG_INFO("NV-CONTROL X extension present");
    LOG_INFO("  version        : %d.%d", major, minor);
    LOG_INFO("  event base     : %d", event_base);
    LOG_INFO("  error base     : %d", error_base);
    
    /*
     * loop over each screen, and determine if each screen is
     * controlled by the NVIDIA X driver (and thus supports the
     * NV-CONTROL X extension); then, query the string attributes on
     * the screen.
     */

    cool->ScreenCount = ScreenCount(cool->dpy);

    if (!NvmlUtils::Get_DeviceID_by_PCI(cool, thread)) {
		LOG_ERR("Failed to find Card ID for PCI " YELLOW("%04x:%02x:%02x"), thread->pciDomainID(), thread->pciBusID(), thread->pciDeviceID());
		return false;
	}

    //qint32 Nvidia::getFanLevel(const qint32 gpu) const

    int value = 0;

    //Nvidia::FanControl Nvidia::getFanControl(const qint32 gpu) const
    //LOG_INFO("Try getting NV_CTRL_GPU_COOLER_MANUAL_CONTROL");
    value = 0;
    r = XNVCTRLQueryTargetAttribute(
                cool->dpy,
                NV_CTRL_TARGET_TYPE_GPU,
                cool->Card, // gpu
                0,
                NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
                &value);
    if (r == True)
    {
        if (value == NV_CTRL_GPU_COOLER_MANUAL_CONTROL_TRUE) {
            //LOG_INFO("NV_CTRL_GPU_COOLER_MANUAL_CONTROL Manual");
            cool->FanIsAutomatic = false;
        }
        else {
            //LOG_INFO("NV_CTRL_GPU_COOLER_MANUAL_CONTROL Auto");
            cool->FanIsAutomatic = true;
        }
    }

    // try to set fan control to manual
    r = XNVCTRLSetTargetAttributeAndGetStatus(
                cool->dpy,
                NV_CTRL_TARGET_TYPE_GPU,
                cool->Card, // gpu
                0,
                NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
                NV_CTRL_GPU_COOLER_MANUAL_CONTROL_TRUE);
    if (r == false)
    {
        LOG_ERR("NV_CTRL_GPU_COOLER_MANUAL_CONTROL_TRUE failed");       
    }
    sleep(1);

    // set back the fan control to automatic if it was automatic
    if (cool->FanIsAutomatic) {
        r = XNVCTRLSetTargetAttributeAndGetStatus(
                    cool->dpy,
                    NV_CTRL_TARGET_TYPE_GPU,
                    cool->Card,  // gpu
                    0,
                    NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
                    NV_CTRL_GPU_COOLER_MANUAL_CONTROL_FALSE);
        if (r == false)
        {
            LOG_ERR("NV_CTRL_GPU_COOLER_MANUAL_CONTROL_FALSE failed");        
        }
    }

    cool->IsFanControlEnabled = r;
    return r;
}
#else
bool NvmlUtils::NVCtrlInit(CoolingContext *cool, CudaThread * thread)
{
	NvAPI_Status ret = NVAPI_OK;

	NVApiInit();

	ret = NvAPI_Initialize();
	if (!ret == NVAPI_OK) {
		NvAPI_ShortString string;
		NvAPI_GetErrorMessage(ret, string);
		LOG_ERR("Failed NVAPI NvAPI_Initialize: %s", string);
		return false;
	}

	NvAPI_ShortString ver;
	NvAPI_GetInterfaceVersionString(ver);
	LOG_INFO("NVAPI Version: %s", ver);

	NvU32 cnt = NVAPI_MAX_PHYSICAL_GPUS;
	ret = NvAPI_EnumPhysicalGPUs(NvmlUtils::physHandle, &cnt);
	if (!ret == NVAPI_OK) {
		NvAPI_ShortString string;
		NvAPI_GetErrorMessage(ret, string);
		LOG_ERR("Failed NVAPI NvAPI_EnumPhysicalGPUs: %", string);
		return false;
	}

	NvU32 BusID = 0;

	// Get PCI info for each card
	bool found = false;
	for (int i = 0; i < 64; i++) {
		ret = NvAPI_GPU_GetBusId(NvmlUtils::physHandle[i], &BusID);
		if (ret == NVAPI_OK) {

			if (thread->pciBusID() == BusID) {
				LOG_INFO("PCIBusID %x CardID %i", BusID, i);
				cool->Card = i;
				found = true;
				break;
			}
		}
	}
	if (!found) {
		LOG_ERR("NvAPI_GPU_GetBusId Unable to find Card " YELLOW("%04x:%02x:%02x"), thread->pciDomainID(), thread->pciBusID(), thread->pciDeviceID());
		return false;
	}

	cool->IsFanControlEnabled = true;
	return true;
	/*
	NvAPI_ShortString name;

	NV_GPU_THERMAL_SETTINGS thermal;

	ret = NvAPI_GPU_GetFullName(phys[0], name);
	if (!ret == NVAPI_OK) {
		NvAPI_ShortString string;
		NvAPI_GetErrorMessage(ret, string);
		printf("NVAPI NvAPI_GPU_GetFullName: %s\n", string);
	}
	ret = NvAPI_GPU_GetFullName(phys[1], name);
	if (!ret == NVAPI_OK) {
		NvAPI_ShortString string;
		NvAPI_GetErrorMessage(ret, string);
		printf("NVAPI NvAPI_GPU_GetFullName: %s\n", string);
	}

	printf("Name: %s\n", name);
	thermal.version = NV_GPU_THERMAL_SETTINGS_VER;
	ret = NvAPI_GPU_GetThermalSettings(phys[0], 0, &thermal);

	if (!ret == NVAPI_OK) {
		NvAPI_ShortString string;
		NvAPI_GetErrorMessage(ret, string);
		printf("NVAPI NvAPI_GPU_GetThermalSettings: %s\n", string);
	}

	printf("Temp: %l C\n", thermal.sensor[0].currentTemp);
	*/


	
	return false;
}
#endif

bool  NvmlUtils::NVCtrlClose(CoolingContext *cool)
{
#ifdef __linux__
	if (!cool->FanIsAutomatic) {
		// Set back to automatic fan control
		cool->CurrentFanLevel = 0;
		NvmlUtils::SetFanPercent(cool, cool->CurrentFanLevel);
	}
    // close the display connection
    XCloseDisplay(cool->dpy);
    return true;
#else	
	NvAPI_Status ret = NVAPI_OK;
	NVApiClose();
	ret = NvAPI_Unload();
	return ret == NVAPI_OK ? true : false;
#endif	
}

#ifdef __linux__
bool NvmlUtils::Get_DeviceID_by_PCI( CoolingContext *cool, CudaThread * thread)
{
	bool found = false;
    int result = -1;
    int nv_ctrl_pci_bus;
    int nv_ctrl_pci_device;
    int nv_ctrl_pci_domain;
    int nv_ctrl_pci_func;

    for (int i = 0; i < 16; i++) {
        XNVCTRLQueryTargetAttribute(cool->dpy, NV_CTRL_TARGET_TYPE_GPU, i, 0, NV_CTRL_PCI_DOMAIN, &nv_ctrl_pci_domain);
        XNVCTRLQueryTargetAttribute(cool->dpy, NV_CTRL_TARGET_TYPE_GPU, i, 0, NV_CTRL_PCI_BUS, &nv_ctrl_pci_bus);
        XNVCTRLQueryTargetAttribute(cool->dpy, NV_CTRL_TARGET_TYPE_GPU, i, 0, NV_CTRL_PCI_DEVICE, &nv_ctrl_pci_device);

        if (nv_ctrl_pci_bus == thread->pciBusID() && nv_ctrl_pci_device == thread->pciDeviceID() && nv_ctrl_pci_domain == thread->pciDomainID()) {
            result = i;
            
			LOG_INFO("FOUND GPU %i - PCI " YELLOW("%04x:%02x:%02x"), i, thread->pciDomainID(), thread->pciBusID(), thread->pciDeviceID());

			cool->Card = i;
			found = true;
            break;
        }
    }
    return found;
}
#else
bool NvmlUtils::Get_DeviceID_by_PCI(CoolingContext *cool, CudaThread * thread)
{
	NvAPI_Status ret = NVAPI_OK;
	NvU32 BusID = 0;
	bool found = false;
	
	// Get PCI info for each card
	for (int i = 0; i < 64; i++) {
		ret = NvAPI_GPU_GetBusId(physHandle[i], &BusID);
		if (ret == NVAPI_OK) {

			if (thread->pciBusID() == BusID) {				
				LOG_INFO("FOUND GPU %i - PCI " YELLOW("%04x:%02x:%02x"), i, thread->pciDomainID(), thread->pciBusID(), thread->pciDeviceID());
				cool->Card = i;
				found = true;
				break;
			}
		}
	}
	return found;
}
#endif

bool NvmlUtils::SetFanPercent(CoolingContext *cool, int percent)
{
	
	if (!cool->IsFanControlEnabled) {
		return false;
	}
	
	if (percent < 0) percent = 0;
	if (percent > 100) percent = 100;

#ifdef __linux__
	return SetFanPercentLinux(cool, percent);
#else
	return NvmlUtils::SetFanPercentWindows(cool, percent);
#endif	
}

bool NvmlUtils::SetFanPercentLinux(CoolingContext *cool, int percent)
{
#ifdef __linux__
	bool result;

	cool->IsFanControlEnabled = true;

	if (percent == 0) {
	    bool r = XNVCTRLSetTargetAttributeAndGetStatus(
            cool->dpy,
            NV_CTRL_TARGET_TYPE_GPU,
            cool->Card, // gpu
            0,
            NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
            NV_CTRL_GPU_COOLER_MANUAL_CONTROL_FALSE);
        if (r != true)
        {
            LOG_WARN("NV_CTRL_GPU_COOLER_MANUAL_CONTROL_FALSE failed");       
        }        
		cool->FanIsAutomatic = true;
        LOG_INFO("Card %i set fan to automatic", cool->Card);
		result = true;
	}
	else {
	    bool r;
        if (cool->FanIsAutomatic) {
            r = XNVCTRLSetTargetAttributeAndGetStatus(
                cool->dpy,
                NV_CTRL_TARGET_TYPE_GPU,
                cool->Card, // gpu
                0,
                NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
                NV_CTRL_GPU_COOLER_MANUAL_CONTROL_TRUE);
            if (r != true) {
                LOG_WARN("NV_CTRL_GPU_COOLER_MANUAL_CONTROL_TRUE failed");       
            }
            cool->FanIsAutomatic = false;
            LOG_INFO("Card %i set fan to manual", cool->Card);
        }

		//int speed = (percent * 255) / 100;
        int speed = percent;

        r = XNVCTRLSetTargetAttributeAndGetStatus(
                    cool->dpy,
                    NV_CTRL_TARGET_TYPE_COOLER,
                    cool->Card, // gpu
                    0,
                    NV_CTRL_THERMAL_COOLER_LEVEL,
                    speed);
        if (r == false)
        {
            LOG_WARN("Failed to set NV_CTRL_THERMAL_COOLER_LEVEL to %i OK", speed);        
        }
        //LOG_INFO("Card %i set fan to level %i", cool->Card, speed);
		
        result = r;
	}
	return result;
#else
	return false;
#endif
}

bool NvmlUtils::SetFanPercentWindows(CoolingContext *cool, int percent)
{
#ifdef __linux__
	return false;
#else
	/*
	if (percent == 0) {
		cool->FanIsAutomatic = true;
	}
	else {
		cool->FanIsAutomatic = false;
	}
	*/
	return NVAPI_SetFanPercent(physHandle[cool->Card], percent);
#endif
}

bool NvmlUtils::GetFanPercent(CoolingContext *cool, int *percent)
{
	
	if (!cool->IsFanControlEnabled) {
		return false;
	}
	
#ifdef __linux__
	return GetFanPercentLinux(cool, percent);
#else
	return GetFanPercentWindows(cool, percent);
#endif	
	LOG_INFO("Card %i Fan %i", cool->Card, *percent);
}
bool NvmlUtils::GetFanPercentLinux(CoolingContext *cool, int *percent)
{
#ifdef __linux__
    int value = 0;
    const Bool ret = XNVCTRLQueryTargetAttribute(
                cool->dpy,
                NV_CTRL_TARGET_TYPE_COOLER,
                cool->Card,
                0,
                NV_CTRL_THERMAL_COOLER_LEVEL,
                &value);
    if (ret == True) {
        cool->CurrentFanLevel = value;
        if (percent != NULL) {
            *percent = value;
        }
    }
    return ret;
#else
	return false;
#endif
}

bool NvmlUtils::GetFanPercentWindows(CoolingContext *cool, int *percent)
{
#ifdef __linux__
	return false;
#else	
	int level = 0;
	int policy = 0;

	bool r = NVAPI_GetFanPercent(physHandle[cool->Card], &level, &policy);
	if (r) {
		cool->CurrentFanLevel = level;
		if (percent != NULL) {
			*percent = level;
		}
		if (policy == NV_GPU_COOLER_POLICY_AUTO) {
			cool->FanIsAutomatic = true;
		}
		else {
			//coolerLvl.cooler[0].policy = NV_GPU_COOLER_POLICY_MANUAL;// 1;
			cool->FanIsAutomatic = false;
		}
		//LOG_INFO("CurrentFanLevel %i Card %i", cool->CurrentFanLevel, cool->Card);
	}
	return r;
#endif	
	/*
	NvAPI_Status ret = NVAPI_OK;
	NvU32 fanTacho;

	ret = NvAPI_GPU_GetTachReading(physHandle[cool->Card], &fanTacho);
	if (ret == NVAPI_OK) {

		cool->CurrentFanLevel = fanTacho;
		if (percent != NULL) {
			*percent = fanTacho;
		}
		LOG_INFO("Fan: %i Card %i", cool->CurrentFanLevel, cool->Card);
		return true;
	}
	else {
		NvAPI_ShortString string;
		NvAPI_GetErrorMessage(ret, string);
		LOG_ERR("NVAPI NvAPI_GPU_GetTachReading: %s\n", string);
		return false;
	}
	*/
	/*
	NvAPI_Status ret = NVAPI_OK;
	NV_GPU_THERMAL_SETTINGS thermal;

	thermal.version = NV_GPU_THERMAL_SETTINGS_VER;
	ret = NvAPI_GPU_GetThermalSettings(physHandle[cool->Card], 0, &thermal);
	if (!ret == NVAPI_OK) {
		NvAPI_ShortString string;
		NvAPI_GetErrorMessage(ret, string);
		LOG_ERR("Failed NVAPI NvAPI_GPU_GetThermalSettings: %s\n", string);
		return false;
	}

	cool->CurrentTemp = thermal.sensor[0].currentTemp;
	return false;
	*/
}

bool NvmlUtils::Temperature(CoolingContext *cool)
{	
	//if (ADL_OK != ADL2_OverdriveN_Temperature_Get(context, deviceIdx, 1, &temp)) {
	//	LOG_ERR("Failed to get ADL2_OverdriveN_Temperature_Get");
	//}
    /*Health health;
    if (NvmlApi::health(cool->Card,  health)) {
        cool->CurrentTemp = health.temperature;
        return true;
    }
    return false;
   */

#ifdef __linux__
    if (cool->Card < 0) {
        LOG_ERR("Failed to read NV_CTRL_THERMAL_SENSOR_READING for Card %i", cool->Card);
        return false;
    }
    int value = 0;
    Bool r = XNVCTRLQueryTargetAttribute(
                cool->dpy,
                NV_CTRL_TARGET_TYPE_THERMAL_SENSOR,
                cool->Card,
                0,
                NV_CTRL_THERMAL_SENSOR_READING,
                &value);
    if (r) {
        cool->CurrentTemp = value;
        //LOG_DEBUG("Card %i Temperature %i", cool->Card, cool->CurrentTemp);
    }
    else {
        LOG_ERR("Failed to read NV_CTRL_THERMAL_SENSOR_READING for Card %i", cool->Card);
    }

    return r;
#else
	NvAPI_Status ret = NVAPI_OK;
	NV_GPU_THERMAL_SETTINGS thermal;

	thermal.version = NV_GPU_THERMAL_SETTINGS_VER;
	ret = NvAPI_GPU_GetThermalSettings(physHandle[cool->Card], 0, &thermal);
	if (!ret == NVAPI_OK) {
		NvAPI_ShortString string;
		NvAPI_GetErrorMessage(ret, string);
		LOG_ERR("Failed NVAPI NvAPI_GPU_GetThermalSettings: %s\n", string);
		return false;
	}
	//for (int i = 0; i < thermal.count; i++) {
	//	LOG_INFO("NvAPI_GPU_GetThermalSettings Card %i Sensor %i Temp %i", cool->Card, i, thermal.sensor[i].currentTemp);
	//}
	cool->CurrentTemp = thermal.sensor[0].currentTemp;
	//LOG_INFO("NvAPI_GPU_GetThermalSettings Temp: %i C Card %i Target %i", cool->CurrentTemp, cool->Card, thermal.sensor[0].target);

	return true;
#endif
}

#ifdef __linux__
int NvmlUtils::GetTickCount(void) 
{
  struct timespec now;
  if (clock_gettime(CLOCK_MONOTONIC, &now))
    return 0;
  return now.tv_sec * 1000.0 + now.tv_nsec / 1000000.0;
}
#endif

bool NvmlUtils::DoCooling(CoolingContext *cool)
{
	const int StartSleepFactor = 500;
    const float IncreaseSleepFactor = 1.5f;
	const int FanFactor = 5;
    const int FanAutoDefault = 50;
	const int TickDiff = GetTickCount() - cool->LastTick;

	if (TickDiff < 1000) {
		return true;
	}
	cool->LastTick = GetTickCount();

	//LOG_INFO("AdlUtils::Temperature(context, DeviceID, deviceIdx) %i", deviceIdx);
	
	if (Temperature(cool) == false) {
		LOG_ERR("Failed to get Temperature for card %i", cool->Card);
		return false;
	}
    
	if (!NvmlUtils::GetFanPercent(cool, NULL)) {
		LOG_ERR("Failed to get Fan speed for card %i", cool->Card);
		return false;
	}

	if (cool->CurrentTemp > Workers::maxtemp()) {
		if (!cool->NeedsCooling) {
			cool->SleepFactor = StartSleepFactor;
			LOG_INFO( YELLOW("Card %u Temperature %u is over %i, reduced mining, Sleeptime %i"), cool->Card, cool->CurrentTemp, Workers::maxtemp(), cool->SleepFactor);
		}
		cool->NeedsCooling = true;
	}
	if (cool->NeedsCooling) {
		if (cool->CurrentTemp < (Workers::maxtemp() - Workers::falloff())) {
			LOG_INFO( YELLOW("Card %u Temperature %i is below %i, increase mining, Sleeptime was %u"), cool->Card, cool->CurrentTemp, Workers::maxtemp() - Workers::falloff(), cool->SleepFactor);
			cool->LastTemp = cool->CurrentTemp;
            if (cool->SleepFactor <= StartSleepFactor) {
				cool->SleepFactor = 0;
			    cool->NeedsCooling = false;
            }
			//cool->SleepFactor = StartSleepFactor;
            cool->SleepFactor = (int)((float)cool->SleepFactor / IncreaseSleepFactor);

			// Decrease fan speed
			if (cool->CurrentFanLevel > 0)
				cool->CurrentFanLevel = cool->CurrentFanLevel - FanFactor;
			SetFanPercent(cool, cool->CurrentFanLevel);

			LOG_INFO( YELLOW("Card %u Sleeptime is now %u"), cool->Card, cool->SleepFactor);
		}
		else {
			if (cool->LastTemp < cool->CurrentTemp) {
				cool->SleepFactor = (int)((float)cool->SleepFactor * IncreaseSleepFactor);
				if (cool->SleepFactor > 10000) {
					cool->SleepFactor = 10000;
				}
				LOG_INFO("Card %u Temperature %i SleepFactor %i LastTemp %i NeedCooling %i ", cool->Card, cool->CurrentTemp, cool->SleepFactor, cool->LastTemp, cool->NeedsCooling);
			}
		}
		
	}
	if (cool->NeedsCooling) {
		int iReduceMining = 10;

		// Increase fan speed
		if (cool->CurrentFanLevel < 100)
			cool->CurrentFanLevel = cool->CurrentFanLevel + (FanFactor*3);
		SetFanPercent(cool, cool->CurrentFanLevel);

		//LOG_INFO("Card %u Temperature %i iReduceMining %i iSleepFactor %i LastTemp %i NeedCooling %i ", deviceIdx, temp, iReduceMining, cool->SleepFactor, cool->LastTemp, cool->NeedCooling);

		do {
			std::this_thread::sleep_for(std::chrono::milliseconds(cool->SleepFactor));
			iReduceMining = iReduceMining - 1;
		} while ((iReduceMining > 0) && (Workers::sequence() > 0));
	}
	else {
		// Decrease fan speed if temp keeps dropping
        if (cool->LastTemp > cool->CurrentTemp) {
            if (!cool->FanIsAutomatic) {
                if (cool->CurrentFanLevel > FanAutoDefault) {
                    cool->CurrentFanLevel = cool->CurrentFanLevel - (FanFactor);
                    SetFanPercent(cool, cool->CurrentFanLevel);
                }
                else {
                    if (cool->CurrentFanLevel < FanAutoDefault) {
                        // Set back to automatic fan control
                        cool->CurrentFanLevel = 0;
                        SetFanPercent(cool, cool->CurrentFanLevel);
                    }	
                }
            }
        }
	}
    //LOG_INFO( YELLOW("Card %u Sleeptime on exit %u"), cool->Card, cool->SleepFactor);
    cool->LastTemp = cool->CurrentTemp;

	return true;
}


