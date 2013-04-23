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
* @file fake_battery.c
*
* @brief Interface for reading all the battery registers in case of qemux86 or using fake batteries.
*
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>

#include "batterylib.h"
#include "battery_read.h"
#include "fileutils.h"

#include <nyx/module/nyx_log.h>

#define SYSFS_DEVICE "/tmp/powerd/fake/battery/"

#define LOG_DOMAIN "fake_battery: "

#define   BATTERY_PERCENT	"getpercent"
#define   BATTERY_TEMPERATURE	"gettemp"
#define   BATTERY_VOLTS	"getvoltage"
#define   BATTERY_CURRENT	"getcurrent"
#define   BATTERY_AVG_CURRENT	"getavgcurrent"
#define   BATTERY_FULL_40	"getfull40"
#define   BATTERY_RAW_COULOMB	"getrawcoulomb"
#define   BATTERY_COULOMB	"getcoulomb"
#define   BATTERY_AGE	"getage"

#define CHARGE_MIN_TEMPERATURE_C 0
#define CHARGE_MAX_TEMPERATURE_C 57
#define BATTERY_MAX_TEMPERATURE_C  60

nyx_battery_ctia_t battery_ctia_params;

nyx_battery_ctia_t *get_battery_ctia_params(void)
{
	battery_ctia_params.charge_min_temp_c=0;
	battery_ctia_params.charge_max_temp_c=CHARGE_MAX_TEMPERATURE_C;
	battery_ctia_params.battery_crit_max_temp=BATTERY_MAX_TEMPERATURE_C;
	battery_ctia_params.skip_battery_authentication=true;

	return &battery_ctia_params;
}

/**
 * @brief Read battery percentage
 *
 * @retval Battery percentage (integer)
 */
int battery_percent(void)
{
    int val;
    int ret;
   	ret = FileGetInt(SYSFS_DEVICE BATTERY_PERCENT, &val);
    if (ret) return -1;
    return val;
}
/**
 * @brief Read battery temperature
 *
 * @retval Battery temperature (integer)
 */
int battery_temperature(void)
{
    int val;
    int ret;
    ret = FileGetInt(SYSFS_DEVICE BATTERY_TEMPERATURE, &val);
    if (ret) return -1;
    return val;
}

/**
 * @brief Read battery voltage
 *
 * @retval Battery voltage (integer)
 */

int battery_voltage(void)
{
    int val = 0;
    int ret;

    ret = FileGetInt(SYSFS_DEVICE BATTERY_VOLTS, &val);
    val = val/1000;

    if (ret) return -1;
    return val;
}

/**
 * @brief Read the amount of current being drawn by the battery.
 *
 * @retval Current (integer)
 */
int battery_current(void)
{
    int val = 0;
    int ret;

    ret = FileGetInt(SYSFS_DEVICE BATTERY_CURRENT, &val);
    val = val/1000;

    if (ret) return -1;
    return val;
}

/**
 * @brief Read average current being drawn by the battery.
 *
 * @retval Current (integer)
 */

int battery_avg_current(void)
{
    int val;
    int ret;
    ret = FileGetInt(SYSFS_DEVICE BATTERY_AVG_CURRENT, &val);
    if (ret) return -1;
    return val / 1000;
}

/**
 * @brief Read battery full capacity
 *
 * @retval Battery capacity (double)
 */
double battery_full40(void)
{
    double val;
    if (FileGetDouble(SYSFS_DEVICE BATTERY_FULL_40, &val))
        return -1;
    return val;
}

/**
 * @brief Read battery current raw capacity
 *
 * @retval Battery capacity (double)
 */

double battery_rawcoulomb(void)
{
    double val;
    if (FileGetDouble(SYSFS_DEVICE BATTERY_RAW_COULOMB, &val))
        return -1;
    return val;
}

/**
 * @brief Read battery current capacity
 *
 * @retval Battery capacity (double)
 */

double battery_coulomb(void)
{
    double val;
    int ret;

  	ret = FileGetDouble(SYSFS_DEVICE BATTERY_COULOMB, &val);
    if (ret) return -1;
    return val;
}

/**
 * @brief Read battery age
 *
 * @retval Battery age (double)
 */
double battery_age(void)
{
    double val;
    if (FileGetDouble(SYSFS_DEVICE BATTERY_AGE, &val))
        return -1;
    return val;
}


bool battery_is_present(void)
{
	int voltage = battery_voltage();
	return (voltage > 0);
}

bool battery_is_authenticated(const char *pair_challenge, const char *pair_response)
{
	return true;
}

void battery_read_init(void)
{
	system("sh /usr/sbin/fake_battery_values.sh");
}

bool battery_authenticate(void)
{
        return true;
}

void battery_set_wakeup_percent(int percentage)
{
	return;
}

