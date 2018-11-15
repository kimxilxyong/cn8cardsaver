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

//#include <CL/cl_ext.h>

#include "nvidia/NvmlUtils.h"
#include "nvidia/NvmlApi.h"
#include "nvidia/Health.h"

#include "common/log/Log.h"
#include "workers/Workers.h"
#include "workers/CudaThread.h"


bool NvmlUtils::Temperature(int id, CoolingContext *cool)
{	
	//if (ADL_OK != ADL2_OverdriveN_Temperature_Get(context, deviceIdx, 1, &temp)) {
	//	LOG_ERR("Failed to get ADL2_OverdriveN_Temperature_Get");
	//}
    Health health;
    if (NvmlApi::health(id, health)) {
        cool->Temp = health.temperature;
        return true;
    }
    return false;
}

bool NvmlUtils::DoCooling(int id, CoolingContext *cool, CudaThread * thread, std::vector<xmrig::IThread*> &threads)
{
	const int StartSleepFactor = 10;

	if (NvmlUtils::Temperature(id, cool)) {

        //LOG_INFO("Card %u Temperature %i iSleepFactor %i LastTemp %i NeedCooling %i ", deviceIdx, cool->Temp, cool->SleepFactor, cool->LastTemp, cool->NeedsCooling);

	    if (cool->Temp > Workers::maxtemp()) {
            if (!cool->NeedsCooling) {
                cool->SleepFactor = StartSleepFactor;
                LOG_INFO("Card %u temperature %u is over %i, reduced mining, sleep time %i", id, cool->Temp, Workers::maxtemp(), cool->SleepFactor);
            }
            cool->NeedsCooling = true;
        }
        if (cool->NeedsCooling) {
            if (cool->Temp < Workers::maxtemp() - Workers::falloff()) {
                LOG_INFO("Card %u temperature %i is below %i, do full mining, sleeptime was %u", id, cool->Temp, Workers::maxtemp() - Workers::falloff(), cool->SleepFactor);
                cool->LastTemp = cool->Temp;
                cool->NeedsCooling = false;
                cool->SleepFactor = StartSleepFactor;
            }
            if (cool->LastTemp <= cool->Temp) {
                cool->SleepFactor = cool->SleepFactor * 2;
                if (cool->SleepFactor > 10000) {
                    cool->SleepFactor = 10000;
                    LOG_WARN("Card %u NvmlId %i Temperature %i iSleepFactor %i LastTemp %i NeedCooling %i lower temp %i sleeping too long", id, thread->nvmlId(), cool->Temp, cool->SleepFactor, cool->LastTemp, cool->NeedsCooling, Workers::maxtemp() - Workers::falloff());
                }
                
            }
            cool->LastTemp = cool->Temp;
        }
        if (cool->NeedsCooling) {
            int iReduceMining = 10;
            //LOG_INFO("Card %u Temperature %i iReduceMining %i iSleepFactor %i LastTemp %i NeedCooling %i ", deviceIdx, temp, iReduceMining, cool->SleepFactor, cool->LastTemp, cool->NeedCooling);

            do {
                std::this_thread::sleep_for(std::chrono::milliseconds(cool->SleepFactor));
                iReduceMining = iReduceMining - 1;
            } while ((iReduceMining > 0) && (Workers::sequence() > 0));
        }
        else {
            cool->SleepFactor = 0;
        }
        return true;
    }
	return false;
}



