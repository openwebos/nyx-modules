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

/**
 * @file batterylib.h
 */

#ifndef _BATTERYLIB_H_
#define _BATTERYLIB_H_

#include <nyx/nyx_module.h>
#include <nyx/module/nyx_utils.h>

int battery_init(void);
bool battery_authenticate(void);
void battery_set_wakeup_percent(int);

#endif // _BATTERYLIB_H_
