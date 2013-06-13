/* @@@LICENSE
*
*      Copyright (c) 2010-2013 LG Electronics, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#include "touchpanel_gestures.h"
#include "touchpanel_common.h"

#include <nyx/module/nyx_log.h>

void
set_event_params(input_event_t* pEvent, time_stamp_t* pTime, uint16_t type,
               uint16_t code, int32_t value)
{
    if (NULL == pEvent || NULL == pTime)
    {
        nyx_error("NULL parameter passed\n");
        return; 
    }

    ((struct timeval*)pTime)->tv_sec = ((time_stamp_t*)&pEvent->time)->time.tv_sec;
    ((struct timeval*)pTime)->tv_usec = ((time_stamp_t*)&pEvent->time)->time.tv_nsec / 1000;

    pEvent->type = type;
    pEvent->code = code;
    pEvent->value = value;
}
