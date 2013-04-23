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

#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <errno.h>
#include <poll.h>
#include <glib.h>
#include <stdio.h>

#include <nyx/nyx_module.h>

#include "keys_common.h"

int keypad_event_fd;

NYX_DECLARE_MODULE(NYX_DEVICE_KEYS, "Keys");

/**
 * This is modeled after the linux input event interface events.
 * See linux/input.h for the original definition.
 */
typedef struct InputEvent
{
    struct timeval time;  /**< time event was generated */
    uint16_t type;        /**< type of event, EV_ABS, EV_MSC, etc. */
    uint16_t code;        /**< event code, ABS_X, ABS_Y, etc. */
    int32_t value;        /**< event value: coordinate, intensity,etc. */
} InputEvent_t;


static nyx_event_keys_t* keys_event_create()
{
    nyx_event_keys_t* event_ptr = (nyx_event_keys_t*) calloc(
            sizeof(nyx_event_keys_t), 1);

    if (NULL == event_ptr)
        return event_ptr;

    ((nyx_event_t*) event_ptr)->type = NYX_EVENT_KEYS;

    return event_ptr;
}

nyx_error_t keys_release_event(nyx_device_t* d, nyx_event_t* e)
{
    if (NULL == d)
        return NYX_ERROR_INVALID_HANDLE;
    if (NULL == e)
        return NYX_ERROR_INVALID_HANDLE;

    nyx_event_keys_t* a = (nyx_event_keys_t*) e;

    free(a);
    return NYX_ERROR_NONE;
}

static int
init_keypad(void)
{
#ifdef KEYPAD_INPUT_DEVICE
    keypad_event_fd = open(KEYPAD_INPUT_DEVICE, O_RDWR);
    if(keypad_event_fd < 0) {
  	nyx_error("Error in opening keypad event file");
  	return -1;
    }
    return 0;
#else
    return -1;
#endif
}

nyx_error_t nyx_module_open(nyx_instance_t i, nyx_device_t** d)
{
    keys_device_t* keys_device = (keys_device_t*) calloc(sizeof(keys_device_t),
               1);

    if (G_UNLIKELY(!keys_device)) {
    		return NYX_ERROR_OUT_OF_MEMORY;
    }
    init_keypad();

    nyx_module_register_method(i, (nyx_device_t*) keys_device,
            NYX_GET_EVENT_SOURCE_MODULE_METHOD, "keys_get_event_source");
    nyx_module_register_method(i, (nyx_device_t*) keys_device,
            NYX_GET_EVENT_MODULE_METHOD, "keys_get_event");
    nyx_module_register_method(i, (nyx_device_t*) keys_device,
            NYX_RELEASE_EVENT_MODULE_METHOD, "keys_release_event");

    *d = (nyx_device_t*) keys_device;

    return NYX_ERROR_NONE;

fail_unlock_settings:

    return NYX_ERROR_GENERIC;
}

nyx_error_t nyx_module_close(nyx_device_t* d)
{
    keys_device_t* keys_device = (keys_device_t*) d;
    if (NULL == d)
        return NYX_ERROR_INVALID_HANDLE;

    if (keys_device->current_event_ptr)
        keys_release_event(d, (nyx_event_t*) keys_device->current_event_ptr);

    nyx_debug("Freeing keys %p", d);
    free(d);

    return NYX_ERROR_NONE;
}

nyx_error_t keys_get_event_source(nyx_device_t* d, int* f)
{
    if (NULL == d)
        return NYX_ERROR_INVALID_HANDLE;
    if (NULL == f)
        return NYX_ERROR_INVALID_VALUE;
    *f = keypad_event_fd;

    return NYX_ERROR_NONE;
}

struct pollfd fds[1];

int
read_input_event(InputEvent_t* pEvents, int maxEvents)
{
    int numEvents = 0;
    int rd = 0;

    if(pEvents == NULL)
		return -1;

    fds[0].fd = keypad_event_fd;
    fds[0].events = POLLIN;

    int ret_val = poll(fds,1,0);
    if(ret_val <= 0)
    {
    	return 0;
    }

    if(fds[0].revents & POLLIN) {
		/* keep looping if get EINTR */
		for (;;)
		{
			rd = read(fds[0].fd, pEvents, sizeof(InputEvent_t) * maxEvents);

			if (rd > 0)
			{
				numEvents += rd / sizeof(InputEvent_t);
				break;
			}
			else if (rd<0 && errno!=EINTR)
			{
				nyx_error("Failed to read events from keypad event file");
				return -1;
			}
		}
    }

    return numEvents;
}

#define MAX_EVENTS      64

nyx_error_t keys_get_event(nyx_device_t* d, nyx_event_t** e)
{
	static InputEvent_t raw_events[MAX_EVENTS];

    static int event_count = 0;
    static int event_iter = 0;

    int rd = 0;

    keys_device_t* keys_device = (keys_device_t*) d;

    /*
     * Event bookkeeping...
     */
    if(!event_iter) {
    	event_count = read_input_event(raw_events, MAX_EVENTS);
       	keys_device->current_event_ptr = NULL;
    }

    if (keys_device->current_event_ptr == NULL) {
        /*
         * let's allocate new event and hold it here.
         */
        keys_device->current_event_ptr = keys_event_create();
    }

    for (; event_iter < event_count;) {
        InputEvent_t* input_event_ptr;
        input_event_ptr = &raw_events[event_iter];
        event_iter++;
        if (input_event_ptr->type == EV_KEY) {
            keys_device->current_event_ptr->key_type = NYX_KEY_TYPE_STANDARD;
            keys_device->current_event_ptr->key = lookup_key(keys_device,
                    input_event_ptr->code, input_event_ptr->value,
                    &keys_device->current_event_ptr->key_type);
        }
        else {
            continue;
        }

        keys_device->current_event_ptr->key_is_press
                = (input_event_ptr->value) ? true : false;
        keys_device->current_event_ptr->key_is_auto_repeat
                = (input_event_ptr->value > 1) ? true : false;

        *e = (nyx_event_t*) keys_device->current_event_ptr;
        keys_device->current_event_ptr = NULL;

        /*
         * Generated event, bail out and let the caller know.
         */
        if (NULL != *e) {
            break;
        }
    }

    if(event_iter >= event_count) {
		event_iter = 0;
    }
    
    return NYX_ERROR_NONE;
}

