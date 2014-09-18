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
 * @file charger.c
 */

#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <libudev.h>
#include <utils.h>

#include <nyx/nyx_module.h>
#include <nyx/module/nyx_utils.h>

#define STATUS_LEN 64
#define PATH_LEN 128

GIOChannel *channel;

struct udev *udev;
struct udev_monitor *mon;

extern nyx_device_t *nyxDev;
extern void *charger_status_callback_context;
extern void *state_change_callback_context;
extern nyx_device_callback_function_t charger_status_callback;
extern nyx_device_callback_function_t state_change_callback;

nyx_battery_status_t *curr_battery_state;
char* battery_status = NULL;

char batt_present_path[PATH_LEN] = {0,};
char batt_status_path[PATH_LEN] = {0,};
char charger_usb_sysfs_online_path[PATH_LEN] = {0,};
char charger_ac_sysfs_online_path[PATH_LEN] = {0,};
char charger_touch_sysfs_online_path[PATH_LEN] = {0,};
char charger_wireless_sysfs_online_path[PATH_LEN] = {0,};

static nyx_charger_event_t current_event = NYX_NO_NEW_EVENT;
nyx_charger_status_t gChargerStatus =
{
	.charger_max_current = 0,
	.connected = 0,
	.powered = 0,
	.dock_serial_number = {0},
	.is_charging = 0,
};

nyx_error_t _charger_read_status(nyx_charger_status_t *status)
{
	/* before we start to update the charger status we reset it completely */
	memset(&gChargerStatus, 0, sizeof(nyx_charger_status_t));

	/* function returns -1 on invalid file path, so check for 1, instead of true */
	if (nyx_utils_read_value(charger_usb_sysfs_online_path) == 1)
	{
		gChargerStatus.connected |= NYX_CHARGER_PC_CONNECTED;
		gChargerStatus.powered |= NYX_CHARGER_USB_POWERED;
	}
	else if (nyx_utils_read_value(charger_ac_sysfs_online_path) == 1)
	{
		gChargerStatus.connected |= NYX_CHARGER_WALL_CONNECTED;
		gChargerStatus.powered |= NYX_CHARGER_DIRECT_POWERED;
	}

	if ((nyx_utils_read_value(charger_usb_sysfs_online_path) == 1) || (nyx_utils_read_value(charger_ac_sysfs_online_path) == 1) ||
	    (nyx_utils_read_value(charger_touch_sysfs_online_path) == 1) || (nyx_utils_read_value(charger_wireless_sysfs_online_path) == 1))
	{
		gChargerStatus.is_charging = 1;
	}

	if (status)
	{
		memcpy(status, &gChargerStatus, sizeof(nyx_charger_status_t));
	}

	return NYX_ERROR_NONE;
}

bool _battery_read_status()
{
	if (curr_battery_state && battery_status)
	{
		memset(curr_battery_state, 0, sizeof(nyx_battery_status_t));
		memset(battery_status, 0, sizeof(battery_status));
		char* status = (char*)malloc(STATUS_LEN);

		if (g_file_test(batt_present_path, G_FILE_TEST_EXISTS))
	    	{
			curr_battery_state->present = ((nyx_utils_read_value(batt_present_path)) == 1) ? true : false;
	    	}
		if ((g_file_test(batt_status_path, G_FILE_TEST_EXISTS)) && (FileGetString(batt_status_path, status, STATUS_LEN)!=-1))
		{
			strcpy(battery_status,status);
		}
		free(status);
	}
	else
	{
		return false;
	}
}

bool _has_charger_state_changed(char* old_state, char* new_state)
{
	if (new_state && !old_state && (strcmp(new_state, "Full") == 0))
	{
		current_event &= ~NYX_CHARGE_RESTART;
		current_event |= NYX_CHARGE_COMPLETE;
		return true;
	}

	if (old_state && new_state && (strcmp(old_state,new_state) != 0))
	{
		if ((strcmp(old_state, "Charging") == 0) && (strcmp(new_state, "Full") == 0))
		{
			current_event &= ~NYX_CHARGE_RESTART;
			current_event |= NYX_CHARGE_COMPLETE;
		}
		else if ((strcmp(old_state, "Full") == 0) && (strcmp(new_state, "Charging") == 0))
		{
			current_event &= ~NYX_CHARGE_COMPLETE;
			current_event |= NYX_CHARGE_RESTART;
		}
		else
		{
			return false;
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool _has_battery_state_changed(int old_state, int new_state)
{
	if (old_state != new_state)
        {
		if (new_state)
		{
			current_event &= ~NYX_BATTERY_ABSENT;
			current_event |= NYX_BATTERY_PRESENT;
		}
		else
		{
			current_event &= ~NYX_BATTERY_PRESENT;
			current_event |= NYX_BATTERY_ABSENT;
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool _has_charger_connected_state_changed(bool old_state, bool new_state)
{
	if (old_state != new_state)
	{
		if (new_state)
		{
			current_event &= ~NYX_CHARGER_DISCONNECTED;
			current_event |= NYX_CHARGER_CONNECTED;
		}
		else
		{
			current_event &= ~NYX_CHARGER_CONNECTED;
			current_event |= NYX_CHARGER_DISCONNECTED;
		}
		return true;
	}
	else
	{
		return false;
	}
}

gboolean _handle_power_supply_event(GIOChannel *channel, GIOCondition condition, gpointer data)
{
	struct udev_device *dev;
	bool fire_charger_status_cb = false;
	bool fire_state_change_cb = false;

	if ((condition & G_IO_IN) == G_IO_IN)
	{
		dev = udev_monitor_receive_device(mon);
		if (dev)
		{
			/* something related to power supply has changed; set the modified event and notify connected clients so
			 * they can query the new status */

			/* Check for event changes and initiate state callback for particular events as below:
			 * NYX_CHARGE_COMPLETE if battery/status from NULL/Charging to Full, NYX_CHARGE_RESTART if battery/status from Full to Charging,
			 * NYX_CHARGER_CONNECTED if USB,AC or any other charger online is from 0 to 1,
			 * NYX_CHARGER_DISCONNECTED if any charger online from 1 to 0,
			 * NYX_CHARGER_FAULT if online=1 and battery/status=Not Charging/Discharging? - TODO: not implemented since we are not sure of the state change for this event
			 * NYX_BATTERY_PRESENT if battery is present (0-1)
			 * NYX_BATTERY_ABSENT if battery is absent (1-0)
			 * NYX_BATTERY_CRITICAL_VOLTAGE if Battery voltage below threshold - TODO: not implemented since we do not get kobject for voltage changes
			 * NYX_BATTERY_TEMPERATURE_LIMIT if Battery temperature below/above limits - TODO: not implemented since we do not get kobject for temperature changes
			 */

			bool prev_charging = gChargerStatus.is_charging;
			_charger_read_status(NULL);
			if (_has_charger_connected_state_changed(prev_charging, gChargerStatus.is_charging))
			{
				fire_charger_status_cb = true;
				fire_state_change_cb = true;
			}

			/* Keep a note of previous values */
			char* prev_batt_status = g_strdup(battery_status);
			int prev_batt_present = curr_battery_state->present;

			_battery_read_status();
			if ((_has_charger_state_changed(prev_batt_status, battery_status)) || (_has_battery_state_changed(prev_batt_present,curr_battery_state->present)))
			{
				fire_state_change_cb = true;
			}
			g_free(prev_batt_status);

			if (fire_charger_status_cb && charger_status_callback)
			{
				charger_status_callback(nyxDev, NYX_CALLBACK_STATUS_DONE, charger_status_callback_context);
				fire_charger_status_cb = false;
			}
			if (fire_state_change_cb && state_change_callback)
			{
				state_change_callback(nyxDev, NYX_CALLBACK_STATUS_DONE, state_change_callback_context);
				fire_state_change_cb = false;
			}
		}
	}
	return TRUE;
}

void _charger_init_events()
{
	_has_charger_state_changed(NULL, battery_status);
	_has_battery_state_changed(0, curr_battery_state->present);
	_has_charger_connected_state_changed(0, gChargerStatus.is_charging);
}

void _detect_charger_sysfs_paths()
{
	char* battery_sysfs_path = find_power_supply_sysfs_path("Battery");
	char* charger_usb_sysfs_path = find_power_supply_sysfs_path("USB");
	char* charger_ac_sysfs_path = find_power_supply_sysfs_path("Mains");
	char* charger_touch_sysfs_path = find_power_supply_sysfs_path("Touch");
	char* charger_wireless_sysfs_path = find_power_supply_sysfs_path("Wireless");

	if (charger_usb_sysfs_path)
	{
		snprintf (charger_usb_sysfs_online_path, PATH_LEN, "%s/online", charger_usb_sysfs_path);
	}
	if (charger_ac_sysfs_path)
	{
		snprintf (charger_ac_sysfs_online_path, PATH_LEN, "%s/online", charger_ac_sysfs_path);
	}
	if (charger_touch_sysfs_path)
	{
		snprintf (charger_touch_sysfs_online_path, PATH_LEN, "%s/online", charger_touch_sysfs_path);
	}
	if (charger_wireless_sysfs_path)
	{
		snprintf (charger_wireless_sysfs_online_path, PATH_LEN, "%s/online", charger_wireless_sysfs_path);
	}
	if (battery_sysfs_path)
	{
		snprintf (batt_present_path, PATH_LEN, "%s/present", battery_sysfs_path);
		snprintf (batt_status_path, PATH_LEN, "%s/status", battery_sysfs_path);
	}
}

nyx_error_t _charger_init(void)
{
	int fd;

	udev = udev_new();
	if (!udev)
	{
		nyx_error("Could not initialize udev component; charger status updates will not be available");
		return NYX_ERROR_GENERIC;
	}

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

	/* Initialize charger sysfs paths */
	_detect_charger_sysfs_paths();
	/* Initialize battery and charger status */
	_charger_read_status(NULL);
	curr_battery_state = (nyx_battery_status_t *) malloc(sizeof(nyx_battery_status_t));
	battery_status = (char *)malloc(STATUS_LEN);
	_battery_read_status();

	/* Initialize events */
	_charger_init_events();

	/* Setup io watch for uevents */
	channel = g_io_channel_unix_new(fd);
	g_io_add_watch(channel, G_IO_IN | G_IO_HUP | G_IO_NVAL, _handle_power_supply_event, NULL);

	return NYX_ERROR_NONE;
}

nyx_error_t _charger_enable_charging(nyx_charger_status_t *status)
{
	memcpy(status, &gChargerStatus, sizeof(nyx_charger_status_t));

	return NYX_ERROR_NONE;
}

nyx_error_t _charger_disable_charging(nyx_charger_status_t *status)
{
	memcpy(status, &gChargerStatus, sizeof(nyx_charger_status_t));

	return NYX_ERROR_NONE;
}

nyx_error_t _charger_query_charger_event(nyx_charger_event_t *event)
{
	*event = current_event;

	return NYX_ERROR_NONE;
}
