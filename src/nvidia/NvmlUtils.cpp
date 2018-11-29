/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018      SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2018 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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
#include <unistd.h>

//#include <CL/cl_ext.h>
#include <X11/Xlib.h>
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>

#include "nvidia/NvmlUtils.h"
#include "nvidia/NvmlApi.h"
#include "nvidia/Health.h"

#include "common/log/Log.h"
#include "workers/Workers.h"
#include "workers/CudaThread.h"


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

    cool->Card = NvmlUtils::Get_DeviceID_by_PCI(cool, thread);

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
	return ADL2_Main_Control_Destroy(context);
#endif	
}


int NvmlUtils::Get_DeviceID_by_PCI( CoolingContext *cool, CudaThread * thread)
{
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
          
            LOG_INFO("FOUND GPU %i threadLocal m_id %i, nvmlId %i index %i pciDeviceID %02x pciBusID %02x pciDomainID %02x", i, thread->index(), thread->nvmlId(), thread->index(), thread->pciDeviceID(), thread->pciBusID(), thread->pciDomainID());
            break;
        }
    }
    return result;
}


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
	return SetFanPercentWindowsw(cool, percent);
#endif	
}

bool NvmlUtils::SetFanPercentLinux(CoolingContext *cool, int percent)
{
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
}

bool NvmlUtils::SetFanPercentWindows(CoolingContext *cool, int percent)
{
	return false;
}

bool NvmlUtils::GetFanPercent(CoolingContext *cool, int *percent)
{
	
	if (!cool->IsFanControlEnabled) {
		return false;
	}

#ifdef __linux__
	return GetFanPercentLinux(cool, percent);
#else
	return GetFanPercentWindowsw(cool, percent);
#endif	
}
bool NvmlUtils::GetFanPercentLinux(CoolingContext *cool, int *percent)
{
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
}
bool NvmlUtils::GetFanPercentWindows(CoolingContext *cool, int *percent)
{
    return false;
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
}

bool NvmlUtils::DoCooling(CoolingContext *cool)
{
	const int StartSleepFactor = 500;
    const float IncreaseSleepFactor = 1.5;
	const int FanFactor = 5;
    const int FanAutoDefault = 50;

	//LOG_INFO("AdlUtils::Temperature(context, DeviceID, deviceIdx) %i", deviceIdx);
	
	if (NvmlUtils::Temperature(cool) == false) {
		return false;
	}
    
    GetFanPercentLinux(cool, NULL);

	if (cool->CurrentTemp > Workers::maxtemp()) {
		if (!cool->NeedsCooling) {
			cool->SleepFactor = StartSleepFactor;
			LOG_INFO( YELLOW("Card %u Temperature %u is over %i, reduced mining, Sleeptime %i"), cool->Card, cool->CurrentTemp, Workers::maxtemp(), cool->SleepFactor);
		}
		cool->NeedsCooling = true;
	}
	if (cool->NeedsCooling) {
		if (cool->CurrentTemp < Workers::maxtemp() - Workers::falloff()) {
			LOG_INFO( YELLOW("Card %u Temperature %i is below %i, increase mining, Sleeptime was %u"), cool->Card, cool->CurrentTemp, Workers::maxtemp() - Workers::falloff(), cool->SleepFactor);
			cool->LastTemp = cool->CurrentTemp;
            if (cool->SleepFactor <= StartSleepFactor) {
			    cool->NeedsCooling = false;
            }
			//cool->SleepFactor = StartSleepFactor;
            cool->SleepFactor = cool->SleepFactor / IncreaseSleepFactor;

			// Decrease fan speed
			if (cool->CurrentFanLevel > 0)
				cool->CurrentFanLevel = cool->CurrentFanLevel - FanFactor;
			NvmlUtils::SetFanPercent(cool, cool->CurrentFanLevel);
		}
		if (cool->LastTemp < cool->CurrentTemp) {
			cool->SleepFactor = cool->SleepFactor * IncreaseSleepFactor;
			if (cool->SleepFactor > 10000) {
				cool->SleepFactor = 10000;
			}
			//LOG_INFO("Card %u Temperature %i iSleepFactor %i LastTemp %i NeedCooling %i ", deviceIdx, temp, cool->SleepFactor, cool->LastTemp, cool->NeedCooling);
		}
		cool->LastTemp = cool->CurrentTemp;
	}
	if (cool->NeedsCooling) {
		int iReduceMining = 10;

		// Increase fan speed
		if (cool->CurrentFanLevel < 100)
			cool->CurrentFanLevel = cool->CurrentFanLevel + (FanFactor*3);
		NvmlUtils::SetFanPercent(cool, cool->CurrentFanLevel);

		//LOG_INFO("Card %u Temperature %i iReduceMining %i iSleepFactor %i LastTemp %i NeedCooling %i ", deviceIdx, temp, iReduceMining, cool->SleepFactor, cool->LastTemp, cool->NeedCooling);

		do {
			std::this_thread::sleep_for(std::chrono::milliseconds(cool->SleepFactor));
			iReduceMining = iReduceMining - 1;
		} while ((iReduceMining > 0) && (Workers::sequence() > 0));
	}
	else {
		// Decrease fan speed
		if (!cool->FanIsAutomatic) {
			if (cool->CurrentFanLevel > FanAutoDefault) {
				cool->CurrentFanLevel = cool->CurrentFanLevel - (FanFactor);
				NvmlUtils::SetFanPercent(cool, cool->CurrentFanLevel);
			}
			else {
				if (cool->CurrentFanLevel < FanAutoDefault) {
					// Set back to automatic fan control
					cool->CurrentFanLevel = 0;
					NvmlUtils::SetFanPercent(cool, cool->CurrentFanLevel);
				}	
			}
		}
	}
	return true;
}


