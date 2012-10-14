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
 * @file chargerlib.c
 */

#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#include <nyx/nyx_module.h>
#include <nyx/module/nyx_utils.h>

#include "chargerlib.h"

nyx_device_t *nyxDev=NULL;
void *charger_status_callback_context = NULL;
void *state_change_callback_context = NULL;
nyx_device_callback_function_t charger_status_callback;
nyx_device_callback_function_t state_change_callback;

NYX_DECLARE_MODULE(NYX_DEVICE_CHARGER, "Charger");

nyx_error_t nyx_module_open (nyx_instance_t i, nyx_device_t** d)
{
	if (nyxDev) {
		nyx_info("Charger device already open.");
		return NYX_ERROR_NONE;
	}

	nyxDev = (nyx_device_t*)calloc(sizeof(nyx_device_t), 1);
	if (NULL == nyxDev) {
		return NYX_ERROR_OUT_OF_MEMORY;
	}

	nyx_module_register_method(i, (nyx_device_t*)nyxDev, NYX_CHARGER_QUERY_CHARGER_STATUS_MODULE_METHOD,
		"charger_query_charger_status");

	nyx_module_register_method(i, (nyx_device_t*)nyxDev, NYX_CHARGER_REGISTER_CHARGER_STATUS_CALLBACK_MODULE_METHOD,
		"charger_register_charger_status_callback");

	nyx_module_register_method(i, (nyx_device_t*)nyxDev, NYX_CHARGER_ENABLE_CHARGING_MODULE_METHOD,
		"charger_enable_charging");

	nyx_module_register_method(i, (nyx_device_t*)nyxDev, NYX_CHARGER_DISABLE_CHARGING_MODULE_METHOD,
		"charger_disable_charging");

	nyx_module_register_method(i, (nyx_device_t*)nyxDev, NYX_CHARGER_REGISTER_STATE_CHANGE_CALLBACK_MODULE_METHOD,
		"charger_register_state_change_callback");

	nyx_module_register_method(i, (nyx_device_t*)nyxDev, NYX_CHARGER_QUERY_CHARGER_EVENT_MODULE_METHOD,
		"charger_query_charger_event");

	*d = (nyx_device_t*)nyxDev;

	return _charger_init();
}

nyx_error_t nyx_module_close (nyx_device_t* d)
{
	return NYX_ERROR_NONE;
}

nyx_error_t charger_query_charger_status(nyx_device_handle_t handle, nyx_charger_status_t *status)
{
	if (handle != nyxDev) {
		return NYX_ERROR_INVALID_HANDLE;
	}

	if (!status) {
		return NYX_ERROR_INVALID_VALUE;
	}

	return _charger_read_status(status);
}

nyx_error_t charger_register_charger_status_callback (nyx_device_handle_t handle, nyx_device_callback_function_t callback_func, void *context)
{
	if (handle != nyxDev) {
		return NYX_ERROR_INVALID_HANDLE;
	}
	if (!callback_func) {
		return NYX_ERROR_INVALID_VALUE;
	}

	charger_status_callback = callback_func;
	charger_status_callback_context= context;

	return NYX_ERROR_NONE;
}

nyx_error_t charger_enable_charging(nyx_device_handle_t handle, nyx_charger_status_t *status)
{
	if (handle != nyxDev) {
		return NYX_ERROR_INVALID_HANDLE;
	}

	if (!status) {
		return NYX_ERROR_INVALID_VALUE;
	}

	return _charger_enable_charging(status);
}

nyx_error_t charger_disable_charging(nyx_device_handle_t handle, nyx_charger_status_t *status)
{
	if (handle != nyxDev) {
		return NYX_ERROR_INVALID_HANDLE;
	}

	if (!status) {
		return NYX_ERROR_INVALID_VALUE;
	}

	return _charger_disable_charging(status);
}

nyx_error_t charger_register_state_change_callback(nyx_device_handle_t handle, nyx_device_callback_function_t callback_func, void *context)
{
	if (handle != nyxDev) {
		return NYX_ERROR_INVALID_HANDLE;
	}
	if (!callback_func) {
		return NYX_ERROR_INVALID_VALUE;
	}

	state_change_callback = callback_func;
	state_change_callback_context = context;

	return NYX_ERROR_NONE;
}

nyx_error_t charger_query_charger_event(nyx_device_handle_t handle, nyx_charger_event_t *event)
{
	if (handle != nyxDev) {
		return NYX_ERROR_INVALID_HANDLE;
	}

	return _charger_query_charger_event(event);
}
