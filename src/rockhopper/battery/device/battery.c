/* @@@LICENSE
*
*      Copyright (c) 2012 Simon Busch <morphis@gravedo.de>
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
* @file battery.c
*
* @brief Interface for reading all the battery values from sysfs node.
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
#include "utils.h"

#include <nyx/module/nyx_log.h>
#include <glib.h>
#include <libudev.h>

#define CHARGE_MIN_TEMPERATURE_C 0
#define CHARGE_MAX_TEMPERATURE_C 57
#define BATTERY_MAX_TEMPERATURE_C  60

#define PATH_LEN 128

char* battery_sysfs_path = NULL;
GIOChannel *channel;

nyx_battery_ctia_t battery_ctia_params;
nyx_battery_status_t *curr_state;

struct udev *udev;
struct udev_monitor *mon;

extern nyx_device_t *nyxDev;
extern void *battery_callback_context;
extern nyx_device_callback_function_t battery_callback;

char batt_capacity_path[PATH_LEN] = {0,};
char batt_energy_now_path[PATH_LEN] = {0,};
char batt_energy_full_path[PATH_LEN] = {0,};
char batt_energy_full_design_path[PATH_LEN] = {0,};
char batt_charge_now_path[PATH_LEN] = {0,};
char batt_charge_full_path[PATH_LEN] = {0,};
char batt_charge_full_design_path[PATH_LEN] = {0,};
char batt_temperature_path[PATH_LEN] = {0,};
char batt_voltage_path[PATH_LEN] = {0,};
char batt_current_path[PATH_LEN] = {0,};
char batt_present_path[PATH_LEN] = {0,};

nyx_battery_ctia_t *get_battery_ctia_params(void)
{
	battery_ctia_params.charge_min_temp_c = 0;
	battery_ctia_params.charge_max_temp_c = CHARGE_MAX_TEMPERATURE_C;
	battery_ctia_params.battery_crit_max_temp = BATTERY_MAX_TEMPERATURE_C;
	battery_ctia_params.skip_battery_authentication = true;

	return &battery_ctia_params;
}

/**
 * @brief Read battery percentage
 *
 * @retval Battery percentage (integer)
 */
int battery_percent(void)
{
	int now, full;
	int capacity;

	/* try capacity node first but keep in mind it's not supported by all power class devices */
	if (!g_file_test(batt_capacity_path, G_FILE_TEST_EXISTS) || ((capacity = nyx_utils_read_value(batt_capacity_path)) < 0))
	{
		/* capacity node is not available so next try is energy_now/energy_full */
		if (g_file_test(batt_energy_now_path, G_FILE_TEST_EXISTS) && g_file_test(batt_energy_full_path, G_FILE_TEST_EXISTS))
		{
			if ((now = nyx_utils_read_value(batt_energy_now_path)) < 0)
			{
				return -1;
			}
			if ((full = nyx_utils_read_value(batt_energy_full_path)) < 0)
			{
				return -1;
			}
			capacity = (now / full);
		}
		/* as last try we can use charge_full or charge_now */
		else if (g_file_test(batt_charge_full_path, G_FILE_TEST_EXISTS) && g_file_test(batt_charge_now_path, G_FILE_TEST_EXISTS))
		{
			if ((full = nyx_utils_read_value(batt_charge_full_path)) < 0)
			{
				return -1;
			}
			if ((now = nyx_utils_read_value(batt_charge_now_path)) < 0)
			{
				return -1;
			}
			capacity = (now / full);
		}
		else
		{
			return -1;
		}
	}
	return capacity;
}
/**
 * @brief Read battery temperature
 *
 * @retval Battery temperature (integer)
 */
int battery_temperature(void)
{
	int temp;

	if (!g_file_test(batt_temperature_path, G_FILE_TEST_EXISTS) ||  ((temp = nyx_utils_read_value(batt_temperature_path)) < 0))
	{
		return -1;
	}
	return temp;
}

/**
 * @brief Read battery voltage
 *
 * @retval Battery voltage (integer)
 */

int battery_voltage(void)
{
	int voltage;

	if (!g_file_test(batt_voltage_path, G_FILE_TEST_EXISTS) || ((voltage = nyx_utils_read_value(batt_voltage_path)) < 0))
	{
		return -1;
	}
	return voltage;
}

/**
 * @brief Read the amount of current being drawn by the battery.
 *
 * @retval Current (integer)
 */
int battery_current(void)
{
	signed int current;

	if (!g_file_test(batt_current_path, G_FILE_TEST_EXISTS) || ((current = nyx_utils_read_value(batt_current_path)) < 0))
	{
		return -1;
	}
	return current;
}

/**
 * @brief Read average current being drawn by the battery.
 *
 * @retval Current (integer)
 */

int battery_avg_current(void)
{
	return -1;
}

/**
 * @brief Read battery full capacity
 *
 * @retval Battery capacity (double)
 */
double battery_full40(void)
{
	int charge_full;

	if (!g_file_test(batt_charge_full_path, G_FILE_TEST_EXISTS) || ((charge_full = nyx_utils_read_value(batt_charge_full_path)) < 0))
	{
		if (!g_file_test(batt_charge_full_design_path, G_FILE_TEST_EXISTS) || ((charge_full = nyx_utils_read_value(batt_charge_full_design_path)) < 0))
		{
			return -1;
		}
	}

	/* Divide the value by 1000 to convert from uAh to mAh */
	return (double) charge_full/1000;
}

/**
 * @brief Read battery current raw capacity
 *
 * @retval Battery capacity (double)
 */

double battery_rawcoulomb(void)
{
	return -1;
}

/**
 * @brief Read battery current capacity
 *
 * @retval Battery capacity (double)
 */

double battery_coulomb(void)
{
	int charge_now;

	if (!g_file_test(batt_charge_now_path, G_FILE_TEST_EXISTS) || ((charge_now = nyx_utils_read_value(batt_charge_now_path)) < 0))
	{
		return -1;
	}

	/* Divide the value by 1000 to convert from uAh to mAh */
	return (double) charge_now/1000;
}

/**
 * @brief Read battery age
 *
 * @retval Battery age (double)
 */
double battery_age(void)
{
	return -1;
}


bool battery_is_present(void)
{
	int present;

	if (!g_file_test(batt_present_path, G_FILE_TEST_EXISTS) || ((present = nyx_utils_read_value(batt_present_path)) < 0))
	{
		return false;
	}

	bool retval = (present == 1) ? true : false;
	return retval;
}

gboolean _handle_event(GIOChannel *channel, GIOCondition condition, gpointer data)
{
	struct udev_device *dev;

	if ((condition  & G_IO_IN) == G_IO_IN)
	{
		dev = udev_monitor_receive_device(mon);
		if (dev)
		{
			/*Initiate callback only if battery percentage or present parameters change*/
			int prev_percentage = curr_state->percentage;
			bool prev_present = curr_state->present;

			battery_read_status(curr_state);
			if ((curr_state->present != prev_present) || (curr_state->percentage != prev_percentage))
			{
				battery_callback(nyxDev, NYX_CALLBACK_STATUS_DONE, battery_callback_context);
			}
		}
	}
	return TRUE;
}

void _detect_battery_sysfs_paths()
{
	battery_sysfs_path = find_power_supply_sysfs_path("Battery");
	if (battery_sysfs_path)
	{
		snprintf (batt_capacity_path, PATH_LEN, "%s/capacity", battery_sysfs_path);
		snprintf (batt_energy_now_path, PATH_LEN, "%s/energy_now", battery_sysfs_path);
		snprintf (batt_energy_full_path, PATH_LEN, "%s/energy_full", battery_sysfs_path);
		snprintf (batt_charge_now_path, PATH_LEN, "%s/charge_now", battery_sysfs_path);
		snprintf (batt_charge_full_path, PATH_LEN, "%s/charge_full", battery_sysfs_path);
		snprintf (batt_charge_full_design_path, PATH_LEN, "%s/charge_full_design", battery_sysfs_path);
		snprintf (batt_temperature_path, PATH_LEN, "%s/temp", battery_sysfs_path);
		snprintf (batt_voltage_path, PATH_LEN, "%s/voltage_now", battery_sysfs_path);
		snprintf (batt_current_path, PATH_LEN, "%s/current_now", battery_sysfs_path);
		snprintf (batt_present_path, PATH_LEN, "%s/present", battery_sysfs_path);
	}
}

nyx_error_t battery_read_init(void)
{
	int fd;
	fd_set readfds;

	udev = udev_new();
	if (!udev)
	{
		nyx_error("Could not initialize udev component; battery status updates will not be available");
		return;
	}

	/*Initialize the sysfs paths*/
	_detect_battery_sysfs_paths();

	curr_state = (nyx_battery_status_t *) malloc(sizeof(nyx_battery_status_t));
	memset(curr_state, 0, sizeof(nyx_battery_status_t));
	battery_read_status(curr_state);

	mon = udev_monitor_new_from_netlink(udev, "kernel");
	if (mon == NULL)
	{
		nyx_error("Failed to create udev monitor for kernel events");
		return NYX_ERROR_GENERIC;
	}
	if (udev_monitor_filter_add_match_subsystem_devtype(mon, "power_supply", NULL) < 0)
	{
		nyx_error("Failed to setup udev filter for power_supply subsytem events");
		return NYX_ERROR_GENERIC;
	}

	if (udev_monitor_enable_receiving(mon) < 0)
	{
		nyx_error("Failed to enable receiving kernel events for power_supply subsytem\n");
		return NYX_ERROR_GENERIC;
	}

	fd = udev_monitor_get_fd(mon);
	channel = g_io_channel_unix_new(fd);
	g_io_add_watch(channel, G_IO_IN | G_IO_HUP | G_IO_NVAL, _handle_event, NULL);
	g_io_channel_unref(channel);

	return NYX_ERROR_NONE;
}

bool battery_is_authenticated(const char *pair_challenge, const char *pair_response)
{
	/* not supported */
	return true;
}

bool battery_authenticate(void)
{
	/* not supported */
	return true;
}

void battery_set_wakeup_percent(int percentage)
{
	/* not supported */
	return;
}
