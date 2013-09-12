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
 * @file batterylib.c
 *
 * @brief Interface for components talking to the battery module using NYX api.
 */

#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

#include "batterylib.h"
#include "battery_read.h"

nyx_device_t *nyxDev = NULL;

void *battery_callback_context = NULL;
nyx_device_callback_function_t battery_callback;

nyx_battery_ctia_t *get_battery_ctia_params(void);

NYX_DECLARE_MODULE(NYX_DEVICE_BATTERY, "Battery");

nyx_error_t nyx_module_open(nyx_instance_t i, nyx_device_t **d)
{
	if (nyxDev)
	{
		nyx_info("Battery device already open");
		*d = (nyx_device_t *)nyxDev;
		return NYX_ERROR_NONE;
	}

	nyxDev = (nyx_device_t *)calloc(sizeof(nyx_device_t), 1);

	if (NULL == nyxDev)
	{
		return NYX_ERROR_OUT_OF_MEMORY;
	}

	nyx_module_register_method(i, (nyx_device_t *)nyxDev,
	                           NYX_BATTERY_QUERY_BATTERY_STATUS_MODULE_METHOD,
	                           "battery_query_battery_status");

	nyx_module_register_method(i, (nyx_device_t *)nyxDev,
	                           NYX_BATTERY_REGISTER_BATTERY_STATUS_CALLBACK_MODULE_METHOD,
	                           "battery_register_battery_status_callback");

	nyx_module_register_method(i, (nyx_device_t *)nyxDev,
	                           NYX_BATTERY_AUTHENTICATE_BATTERY_MODULE_METHOD,
	                           "battery_authenticate_battery");

	nyx_module_register_method(i, (nyx_device_t *)nyxDev,
	                           NYX_BATTERY_GET_CTIA_PARAMETERS_MODULE_METHOD,
	                           "battery_get_ctia_parameters");

	nyx_module_register_method(i, (nyx_device_t *)nyxDev,
	                           NYX_BATTERY_SET_WAKEUP_PARAMETERS_MODULE_METHOD,
	                           "battery_set_wakeup_percentage");

	*d = (nyx_device_t *)nyxDev;
	battery_read_init();

	return NYX_ERROR_NONE;
}

nyx_error_t nyx_module_close(nyx_device_t *d)
{
	return NYX_ERROR_NONE;
}

void battery_read_status(nyx_battery_status_t *state)
{
	if (state)
	{
		memset(state, 0, sizeof(nyx_battery_status_t));

		state->present = battery_is_present();

		if (state->present)
		{
			state->percentage = battery_percent();
			state->temperature = battery_temperature();
			state->current = battery_current();
			state->voltage = battery_voltage();
			state->avg_current = battery_current();
			state->capacity = battery_coulomb();
			state->capacity_raw = battery_rawcoulomb();
			state->capacity_full40 = battery_full40();
			state->age = battery_age();

			if (state->avg_current >  0)
			{
				state->charging = true;
			}
		}
		else
		{
			state->charging = false;
		}
	}
}

nyx_error_t battery_query_battery_status(nyx_device_handle_t handle,
        nyx_battery_status_t *status)
{
	if (handle != nyxDev)
	{
		return NYX_ERROR_INVALID_HANDLE;
	}

	if (!status)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	battery_read_status(status);

	return NYX_ERROR_NONE;
}

nyx_error_t battery_register_battery_status_callback(nyx_device_handle_t handle,
        nyx_device_callback_function_t callback_func, void *context)
{
	if (handle != nyxDev)
	{
		return NYX_ERROR_INVALID_HANDLE;
	}

	if (!callback_func)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	battery_callback_context = context;
	battery_callback = callback_func;

	return NYX_ERROR_NONE;
}

nyx_error_t battery_authenticate_battery(nyx_device_handle_t batt_device,
        bool *result)
{
	*result = battery_authenticate();
	return NYX_ERROR_NONE;
}


nyx_error_t battery_get_ctia_parameters(nyx_device_handle_t handle,
                                        nyx_battery_ctia_t *param)
{
	if (handle != nyxDev)
	{
		return NYX_ERROR_INVALID_HANDLE;
	}

	if (!param)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	nyx_battery_ctia_t *battery_ctia = get_battery_ctia_params();

	if (!battery_ctia)
	{
		return NYX_ERROR_INVALID_OPERATION;
	}

	memcpy(param, battery_ctia, sizeof(nyx_battery_ctia_t));
	return NYX_ERROR_NONE;
}

nyx_error_t battery_set_wakeup_percentage(nyx_device_handle_t handle,
        int percentage)
{
	if (handle != nyxDev)
	{
		return NYX_ERROR_INVALID_HANDLE;
	}

	if (percentage < 0 || percentage > 100)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	battery_set_wakeup_percent(percentage);
	return NYX_ERROR_NONE;
}

