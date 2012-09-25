/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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
 * @file battery_read.h
 */

#ifndef _BATTERY_READ_H_
#define _BATTERY_READ_H_
#define BATT_CHALLENGE_MAX_LEN		16
#define BATT_RESPONSE_MAX_LEN		40

int battery_percent(void);
int battery_temperature(void);
int battery_voltage(void);
int battery_current(void);
int battery_avg_current(void);
double battery_full40(void);
double battery_rawcoulomb(void);
double battery_coulomb(void);
double battery_age(void);
bool battery_is_present(void);
bool battery_is_authenticated(const char *pair_challenge, const char *pair_response);
void battery_read_init(void);

#endif /* _BATTERY_READ_H_ */
