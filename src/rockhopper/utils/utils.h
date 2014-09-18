/* @@@LICENSE
*
*      Copyright (c) 2014 LG Electronics, Inc.
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
 * @file utils.h
 */

#ifndef UTILS_H_
#define UTILS_H_

int FileGetString(const char *path, char *ret_string, size_t maxlen);
int FileGetDouble(const char *path, double *ret_data);
char* find_power_supply_sysfs_path(const char *device_type);

#endif // UTILS_H_
