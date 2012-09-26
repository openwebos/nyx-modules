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


#include <assert.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/input.h>
#include <linux/fb.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <glib.h>
#include <errno.h>
#include <poll.h>

#include <nyx/nyx_module.h>
#include <nyx/module/nyx_event_touchpanel_internal.h>
#include <nyx/common/nyx_macros.h>
#include <nyx/module/nyx_utils.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>
#include <dirent.h>
#include <fcntl.h>

#include "touchpanel_gestures.h"

/* Later versions of nyx_utils.h no longer define this macro */
#undef return_if
#define return_if(condition, args...)                         \
  do {                                                        \
    if (G_UNLIKELY(condition)) {                              \
      return args;                                            \
    }                                                         \
  } while(0)

typedef struct {
	nyx_device_t _parent;
	nyx_event_touchpanel_t* current_event_ptr;
	int32_t mode;
} touchpanel_device_t;

NYX_DECLARE_MODULE(NYX_DEVICE_TOUCHPANEL, "Touchpanel");

#define MAX_HIDD_EVENTS		(4096 / sizeof(input_event_t))

typedef struct  {
	size_t input_filled;
	size_t input_read;
	input_event_t input[MAX_HIDD_EVENTS];
} event_list_t;


event_list_t touchpanel_event_list;
int touchpanel_event_fd = -1;

static void touch_item_reset(nyx_touchpanel_event_item_t *t) {
	t->finger = 0;
	t->state = NYX_TOUCHPANEL_STATE_UNDEFINED;
	t->x = 0;
	t->y = 0;
	t->gestureKey = -1;
    	t->xVelocity = 0;
    	t->yVelocity = 0;
	t->weight = (double) NAN;
}

static nyx_event_touchpanel_t* touch_event_create()
{
	nyx_event_touchpanel_t* event_ptr;
	event_ptr =
		(nyx_event_touchpanel_t*) calloc(sizeof(nyx_event_touchpanel_t), 1);

	if (NULL == event_ptr) {
		return event_ptr;
	}

	event_ptr->type = NYX_TOUCHPANEL_EVENT_TYPE_TOUCH;
	event_ptr->item_count = 0;
	return event_ptr;
}

nyx_error_t touchpanel_release_event(nyx_device_t* d, nyx_event_t* e)
{
	if (NULL == d) {
		return NYX_ERROR_INVALID_HANDLE;
	}
	if (NULL == e) {
		return NYX_ERROR_INVALID_HANDLE;
	}

	nyx_event_touchpanel_t* a = (nyx_event_touchpanel_t*) e;
	free(a);
	return NYX_ERROR_NONE;
}

static nyx_touchpanel_event_item_t* touch_event_get_next_item(
	nyx_event_touchpanel_t* i_event_ptr)
{
	nyx_touchpanel_event_item_t* item_ptr = NULL;
	assert(NULL != i_event_ptr);

	if (i_event_ptr->item_count < NYX_MAX_TOUCH_EVENTS) {
		item_ptr = &i_event_ptr->item_array[i_event_ptr->item_count++];
	}
	else {
		nyx_error("tried allocating too many touch items: event %p, item cnt %d, max %d\n",
			  i_event_ptr, i_event_ptr->item_count, NYX_MAX_TOUCH_EVENTS);
	}

	return item_ptr;
}

static nyx_touchpanel_event_item_t* touch_event_get_current_item(
	nyx_event_touchpanel_t* i_event_ptr)
{
	nyx_touchpanel_event_item_t* item_ptr = NULL;
	assert(NULL != i_event_ptr);

	if (i_event_ptr->item_count > 0) {
		item_ptr = &i_event_ptr->item_array[i_event_ptr->item_count - 1];
	}
	else {
		nyx_error("No touch items available! event %p\n", i_event_ptr);
	}
	return item_ptr;
}

static inline int64_t get_ts_tval(struct timeval *tv)
{
	return tv->tv_sec * 1000000000LL + tv->tv_usec * 1000;
}


/*
 * FIXME: The following two definitions are a temporary hack to work around
 * the fact that the XML file parsing reads in all the cypress-library
 * settings that we don't need or want (for dependency reasons)
 */
static general_settings_t sGeneralSettings =
{
    	.coordBufSize = 6,
	.fingerDownThreshold = 0
};

static float scaleX, scaleY;

/* Using hardcoded values for now ,since ioctl FBIOGET_VSCREENINFO 
   on /dev/fb0 is returning invalid values in qemux86 */

#define SCREEN_HORIZONTAL_RES	1024
#define SCREEN_VERTICAL_RES	768

static int
init_touchpanel(void)
{
	struct input_absinfo abs;
	int  maxX, maxY, ret = -1;
    	
	touchpanel_event_fd = open("/dev/input/touchscreen0", O_RDWR);
	if(touchpanel_event_fd < 0) {
		nyx_error("Error in opening touchpanel event device");
		return -1;
	}

	ret = ioctl(touchpanel_event_fd, EVIOCGABS(0), &abs);
	if(ret < 0) {
		nyx_error("Error in fetching screen horizontal limits");
		goto error;
	}
	maxX = abs.maximum;

	ret = ioctl(touchpanel_event_fd, EVIOCGABS(1), &abs);
	if(ret < 0) {
		nyx_error("Error in fetching screen vertical limits");
		goto error;
	}
	maxY = abs.maximum;

    	init_gesture_state_machine(&sGeneralSettings, 1);

	scaleX = (float)SCREEN_HORIZONTAL_RES / (float)maxX;
	scaleY = (float)SCREEN_VERTICAL_RES / (float)maxY;

	return 0;
error:
	if(touchpanel_event_fd >= 0)
		close(touchpanel_event_fd);
	return ret;
}


nyx_error_t nyx_module_open(nyx_instance_t i, nyx_device_t** d)
{

	touchpanel_device_t* touchpanel_device = (touchpanel_device_t *) calloc(
				sizeof(touchpanel_device_t), 1);

    	if (G_UNLIKELY(!touchpanel_device)) {
		return NYX_ERROR_OUT_OF_MEMORY;
	}


	int ret = nyx_module_register_method(i, (nyx_device_t*) touchpanel_device,
			NYX_GET_EVENT_SOURCE_MODULE_METHOD, "touchpanel_get_event_source");

	nyx_module_register_method(i, (nyx_device_t*) touchpanel_device,
		NYX_GET_EVENT_MODULE_METHOD, "touchpanel_get_event");
	nyx_module_register_method(i, (nyx_device_t*) touchpanel_device,
		NYX_RELEASE_EVENT_MODULE_METHOD, "touchpanel_release_event");
	nyx_module_register_method(i, (nyx_device_t*) touchpanel_device,
	        NYX_SET_OPERATING_MODE_MODULE_METHOD, "touchpanel_set_operating_mode");
	nyx_module_register_method(i, (nyx_device_t*) touchpanel_device,
        	NYX_TOUCHPANEL_SET_ACTIVE_SCAN_RATE_MODULE_METHOD, "touchpanel_set_active_scan_rate");
    	nyx_module_register_method(i, (nyx_device_t*) touchpanel_device,
            	NYX_TOUCHPANEL_SET_IDLE_SCAN_RATE_MODULE_METHOD, "touchpanel_set_idle_scan_rate");
    	nyx_module_register_method(i, (nyx_device_t*) touchpanel_device,
            	NYX_TOUCHPANEL_GET_IDLE_SCAN_RATE_MODULE_METHOD, "touchpanel_get_idle_scan_rate");
    	nyx_module_register_method(i, (nyx_device_t*) touchpanel_device,
            	NYX_TOUCHPANEL_GET_ACTIVE_SCAN_RATE_MODULE_METHOD, "touchpanel_get_active_scan_rate");
    	nyx_module_register_method(i, (nyx_device_t*) touchpanel_device,
            	NYX_TOUCHPANEL_SET_MODE_MODULE_METHOD, "touchpanel_set_mode");
    	nyx_module_register_method(i, (nyx_device_t*) touchpanel_device,
            	NYX_TOUCHPANEL_GET_MODE_MODULE_METHOD, "touchpanel_get_mode");

	*d = (nyx_device_t*) touchpanel_device;

	if(init_touchpanel() < 0)
		goto fail_unlock_settings;

	return NYX_ERROR_NONE;

fail_unlock_settings:
	return NYX_ERROR_GENERIC;
}

nyx_error_t nyx_module_close(nyx_device_t *d) {

	touchpanel_device_t* touchpanel_device = (touchpanel_device_t*) d;

	if (touchpanel_device->current_event_ptr) {
		touchpanel_release_event(d,
				(nyx_event_t*) touchpanel_device->current_event_ptr);
	}

	nyx_debug("Freeing touchpanel %p", d);

        deinit_gesture_state_machine();
	free(d);
 	
	if(touchpanel_event_fd >= 0) {
                close(touchpanel_event_fd);
		touchpanel_event_fd = -1;
	}


	return NYX_ERROR_NONE;
}

nyx_error_t touchpanel_get_event_source(nyx_device_t* d, int* f) {

	if (NULL == d)
		return NYX_ERROR_INVALID_HANDLE;

	if (NULL == f)
		return NYX_ERROR_INVALID_VALUE;

	*f = touchpanel_event_fd;

	return NYX_ERROR_NONE;
}

nyx_error_t touchpanel_set_operating_mode(nyx_device_t* d, nyx_operating_mode_t m)
{
	return NYX_ERROR_NOT_IMPLEMENTED;
}

void
get_time_stamp(time_stamp_t* pTime)
{
    	struct timeval tv;
    	(void)gettimeofday(&tv, NULL);

     	pTime->time.tv_sec = tv.tv_sec;
     	pTime->time.tv_nsec = tv.tv_usec * 1000;
}


int cachedX, cachedY;

static void
generate_mouse_gesture(int touchButtonState)
{
	int32_t xOrd[2], yOrd[2], wOrd[2], fingers;
    	time_stamp_t eventTime;
    	int num_events=0;

    	get_time_stamp(&eventTime);
    	xOrd[0] = cachedX;
    	yOrd[0] = cachedY;
    	wOrd[0] = touchButtonState ? 1 : 0;
    	fingers = touchButtonState ? 1 : 0;

    	xOrd[1] = 0;
    	yOrd[1] = 0;
    	wOrd[1] = 0;

    	gesture_state_machine(xOrd, yOrd, wOrd, fingers, &eventTime,touchpanel_event_list.input,&num_events);
    	touchpanel_event_list.input_filled=num_events * sizeof(input_event_t);
    	touchpanel_event_list.input_read=0;
}


/**
 * An EV_SYN event that is a flag to indicate that we've just started a plugin
 * and anything expecting us to be in a certain state should clear its state
 */
#define SYN_START       8


static void handle_new_event(input_event_t *event)
{
	static int touchButtonState = 0;

	// Truncate scaled X & Y coordinate values
	if ((event->type == EV_ABS) && (event->code == ABS_X))
			cachedX = (int) (event->value * scaleX);
	
	else if ((event->type == EV_ABS) && (event->code == ABS_Y))
			cachedY = (int) (event->value * scaleY);
	
	else if ((event->type == EV_KEY) && (event->code == BTN_TOUCH))
	{	
            // save touchButtonState (up or down)
		touchButtonState = event->value;
	        if (touchButtonState == 0)
	        {
	                 /* generate another event with the coordinates and time of the
	                 * release point so that we can calculate how long the mouse
	                 * button has been down in the same spot and not create flicks
	                 * if it has been down for long enough
	                 */
	        	generate_mouse_gesture(1);
         	}
	 }
	 else if (event->type == EV_SYN)
	 {
	       	generate_mouse_gesture(touchButtonState);
	 }
	
	 if ((event->type == EV_REL && event->code == REL_WHEEL) ||
            (event->type == EV_KEY && (event->code == BTN_MIDDLE || event->code == BTN_SIDE ||
                                         event->code == BTN_EXTRA || event->code == BTN_FORWARD ||
                                         event->code == BTN_BACK || event->code == BTN_TASK))) {
             memcpy(&touchpanel_event_list.input[0],event,sizeof(input_event_t));
             // Forward an EV_SYN after the key event, to make sure it is processed immediately.
             input_event_t syn_event;
             syn_event.type = EV_SYN;
             syn_event.code = SYN_START;
             syn_event.value = 0;

             memcpy(&touchpanel_event_list.input[1],&syn_event,sizeof(input_event_t));

             touchpanel_event_list.input_filled=2 * sizeof(input_event_t);
             touchpanel_event_list.input_read=0;
        }
	return;
}

static struct pollfd fds[1];

static int
read_input_event(void)
{
	int numEvents = 0;
	int rd = 0;
	input_event_t pEvent;
	
	fds[0].fd = touchpanel_event_fd;
	fds[0].events = POLLIN;

	int ret_val = poll(fds,1,0);
	if(ret_val <= 0)
	{
    		return 0;
	}

	if(fds[0].revents & POLLIN) {
		rd = read(fds[0].fd, &pEvent, sizeof(input_event_t));

		if (rd<0 && errno!=EINTR)
		{
			nyx_error("Failed to read events from keypad event file");
			return -1;
		}

		handle_new_event(&pEvent);
    	}
    	return numEvents;
}

nyx_error_t touchpanel_get_event(nyx_device_t* d, nyx_event_t** e)
{
	int event_count = 0;
	int event_iter = 0;
	static int read_input = 0;

	nyx_event_t* p_generated = NULL;
	touchpanel_device_t* touch_device = (touchpanel_device_t*) d;

    	/*
    	 * Event bookkeeping...
    	 */
    	if(!read_input) {
    		read_input_event();
    		touch_device->current_event_ptr = NULL;
    		read_input = 1;
    	}

    	/*
     	* Event bookkeeping...
     	*/
	event_count = touchpanel_event_list.input_filled / sizeof(input_event_t);
	event_iter = touchpanel_event_list.input_read / sizeof(input_event_t);

   	if(event_iter == event_count)
   		read_input = 0;

   	 if (touch_device->current_event_ptr == NULL) {
        	/*
         	* let's allocate new event and hold it here.
         	*/
    		touch_device->current_event_ptr = touch_event_create();
    	}

    	touch_device->current_event_ptr->_parent.type = NYX_EVENT_TOUCHPANEL;

	for (; event_iter < event_count; event_iter++) {
		input_event_t* input_event_ptr;
		nyx_touchpanel_event_item_t* item_ptr;
		input_event_ptr = &touchpanel_event_list.input[event_iter];

		touchpanel_event_list.input_read += sizeof(input_event_t);

		switch (input_event_ptr->type) {
		case EV_FINGERID:
			item_ptr = touch_event_get_next_item(
					touch_device->current_event_ptr);

			if (NULL == item_ptr) {
				p_generated = (nyx_event_t*) touch_device->current_event_ptr;
				touch_device->current_event_ptr = touch_event_create();
				item_ptr = touch_event_get_next_item(
						touch_device->current_event_ptr);
			}

			if (NULL != item_ptr) {
				touch_item_reset(item_ptr);
				item_ptr->finger = input_event_ptr->value * 1000
						+ input_event_ptr->code;
				item_ptr->timestamp = get_ts_tval(&(input_event_ptr->time));
			}
			break;
		case EV_ABS:
			item_ptr = touch_event_get_current_item(
					touch_device->current_event_ptr);

			if (NULL != item_ptr) {
				if (ABS_X == input_event_ptr->code) {
					item_ptr->x = input_event_ptr->value;
				} else if (ABS_Y == input_event_ptr->code) {
					item_ptr->y = input_event_ptr->value;
				} else {
					nyx_error("Unexpected code 0x%x\n",input_event_ptr->code);
				}
			}
			break;
		case EV_KEY:
			item_ptr = touch_event_get_current_item(
					touch_device->current_event_ptr);

			if (NULL != item_ptr) {
				if (BTN_TOUCH == input_event_ptr->code) {
					if (1 == input_event_ptr->value) {
						item_ptr->state = NYX_TOUCHPANEL_STATE_DOWN;
					} else {
						item_ptr->state = NYX_TOUCHPANEL_STATE_UP;
					}
				}
			}
			break;
		case EV_SYN:
			p_generated = (nyx_event_t*) touch_device->current_event_ptr;
			touch_device->current_event_ptr = NULL;

			break;
		default:
			nyx_warn("Invalid event type (0x%x)", input_event_ptr->type);
			break;
		}
		// Generated event, bail out and let the caller know.
		if (NULL != p_generated) {
			break;
		}
	}
	*e = p_generated;

    return NYX_ERROR_NONE;
}

nyx_error_t touchpanel_set_active_scan_rate(nyx_device_t* d, unsigned int r)
{
	return NYX_ERROR_NOT_IMPLEMENTED;
}

nyx_error_t touchpanel_set_idle_scan_rate(nyx_device_t* d, unsigned int r)
{
	return NYX_ERROR_NOT_IMPLEMENTED;
}

nyx_error_t touchpanel_get_active_scan_rate(nyx_device_t* d, unsigned int* r)
{
	return NYX_ERROR_NOT_IMPLEMENTED;
}

nyx_error_t touchpanel_get_idle_scan_rate(nyx_device_t* d, unsigned int* r)
{
	return NYX_ERROR_NOT_IMPLEMENTED;
}

nyx_error_t touchpanel_set_mode(nyx_device_t* d, int m)
{
	return NYX_ERROR_NOT_IMPLEMENTED;
}

nyx_error_t touchpanel_get_mode(nyx_device_t* d, int *m)
{
	return NYX_ERROR_NOT_IMPLEMENTED;
}
