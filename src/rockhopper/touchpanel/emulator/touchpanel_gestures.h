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

#ifndef __TOUCHPANEL_GESTURES_H
#define __TOUCHPANEL_GESTURES_H

#include <limits.h>
#include <glib.h>
#include <linux/input.h>
#include <stdbool.h>

typedef         __u8            uint8_t;
typedef         __u16           uint16_t;
typedef         __u32           uint32_t;


#define EV_FINGERID 0x07

typedef struct time_stamp
{
	struct timespec time;   /**< internal time stamp format */
} time_stamp_t;

typedef enum
{
    LOCALE_LEFT_RIGHT = 0,
    LOCALE_RIGHT_LEFT,
    NUM_LOCALES
} locale_type_t;

typedef struct interrupt_on_touch_settings
{
	bool enabled;
	int scanRate;           /**< HZ */
	int wakeThreshold;
	int noTouchThreshold;   /**< ms */
	int watchdogTimeout;    /**< ms */
} interrupt_on_touch_settings_t;


typedef struct general_settings
{
	int coordBufSize;           /**< size of coordinate circular buffer to calculate
                                     things such as avg velocity */
	int fingerDownThreshold;            /**< threshold to accept finger as down -- access atomically */

	int positionFilter;
} general_settings_t;

typedef struct coord
{
	int x;                     /**< ptr to array of x coords */
	int y;                     /**< ptr to array of y coords */
	time_stamp_t timeStamp;   /**< time of the coordinates */
} coord_t;

typedef struct coord_buf
{
	coord_t *pCoords;       /**< array of coords */
	int head;               /**< index of start of items in the array */
	int tail;               /**< index of end of items in the array  */
	int numItems;           /**< number of items in the array */
	int size;               /**< total size of coord array */
} coord_buf_t;

typedef enum
{
    UNUSED = -1,
    START_STATE = 0,                        /**< no fingers down */
    FINGER_DOWN_STATE,                      /**< a finger is down */
    FINGER_DOWN_AFTER_QUICK_LAUNCH,         /**< a quick launch was detected and we're
                                               waiting for the finger to come up */
} gesture_state_t;

#define NUM_DIMENSIONS  2
#define X_DIM   0
#define Y_DIM   1

typedef struct gesture_state_data
{
	gesture_state_t state;
	int start[NUM_DIMENSIONS];
	bool insideTapRadius;
	time_stamp_t startTime;
} gesture_state_data_t;

typedef struct
{
	struct timeval time;  /**< time event was generated */
	uint16_t type;        /**< type of event, EV_ABS, EV_MSC, etc. */
	uint16_t code;        /**< event code, ABS_X, ABS_Y, etc. */
	int32_t value;        /**< event value: coordinate, intensity,etc. */
} input_event_t;

typedef struct finger
{
	coord_buf_t coords;
	time_stamp_t timestamp;
	uint32_t id;
	gesture_state_data_t state;
	int minDist;
	int minDistId;
	int lastWeight;
	int numEvents;
	input_event_t *events;
} finger_t;



void init_gesture_state_machine(const general_settings_t *pGeneralSettings,
                                int maxFingers);
void deinit_gesture_state_machine(void);
void gesture_state_machine(int *pXCoords, int *pYCoords,
                           const int *pFingerWeights,
                           int fingerCount, const time_stamp_t *pTime,
                           input_event_t *events, int *numEvents);

#endif  /* __TOUCHPANEL_GESTURES_PRV_H */
