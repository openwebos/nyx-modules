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


#include <stdlib.h>
#include <limits.h>
#include <glib-2.0/glib.h>

#include <nyx/module/nyx_log.h>

#include "touchpanel_gestures.h"
#include "touchpanel_common.h"

static GList *sFingers = NULL;
static GQueue availableFingers = G_QUEUE_INIT;

static uint32_t curFingerId = 0;

int gesture_state_machine_finger(finger_t *finger, input_event_t *events,
                                 int *numEvents);

static const general_settings_t *spGeneralSettings = NULL;

/**
 *******************************************************************************
 * @brief Allocate and initialize the buffer that keeps a coordinate history
 *
 * @param  ppCoordBuf   IN/OUT  ptr to the coordinate buffer struct
 * @param  bufSize      IN      size of the buffer
 *
 * @retval  0 on success
 * @retval -1 on failure
 *******************************************************************************
 */
int
create_coord_buffer(coord_buf_t *pCoordBuf, int bufSize)
{

	if (NULL == pCoordBuf)
	{
		nyx_error("NULL parameter passed");
		return -1;
	}

	pCoordBuf->pCoords = (coord_t *)malloc(sizeof(coord_t) * bufSize);

	if (NULL == pCoordBuf->pCoords)
	{
		nyx_error("Failed to allocate memory");
		return -1;
	}

	pCoordBuf->size = bufSize;
	pCoordBuf->head = 0;
	pCoordBuf->tail = 0;
	pCoordBuf->numItems = 0;

	return 0;
}


void
free_coord_buffer(coord_buf_t *pCoordBuf)
{
	free(pCoordBuf->pCoords);
}


void
reset_coord_buffer(coord_buf_t *pCoordBuf)
{
	pCoordBuf->head = 0;
	pCoordBuf->tail = 0;
	pCoordBuf->numItems = 0;
}

void
update_coord_buffer(coord_buf_t *pCoordBuf, int xCoord, int yCoord,
                    const time_stamp_t *pTime)
{
	if (pCoordBuf->head == pCoordBuf->tail &&
	        pCoordBuf->numItems == pCoordBuf->size)
	{
		/* full buffer, so make space for new item and overwrite old one */
		pCoordBuf->head = (pCoordBuf->head + 1) % pCoordBuf->size;
	}

	int index = pCoordBuf->tail;
	coord_t *pCurCoord = &((pCoordBuf->pCoords)[index]);

	if (spGeneralSettings->positionFilter && pCoordBuf->numItems)
	{
		int prev;

		if (index)
		{
			prev = index - 1;
		}

		else
		{
			prev = pCoordBuf->size - 1;
		}

		if (xCoord < pCoordBuf->pCoords[prev].x)
		{
			xCoord++;
		}

		else if (xCoord > pCoordBuf->pCoords[prev].x)
		{
			xCoord--;
		}

		if (yCoord < pCoordBuf->pCoords[prev].y)
		{
			yCoord++;
		}

		else if (yCoord > pCoordBuf->pCoords[prev].y)
		{
			yCoord--;
		}
	}

	pCurCoord->timeStamp = *pTime;
	pCurCoord->x = xCoord;
	pCurCoord->y = yCoord;

	if (pCoordBuf->numItems < pCoordBuf->size)
	{
		pCoordBuf->numItems++;
	}

	pCoordBuf->tail = (pCoordBuf->tail + 1) % pCoordBuf->size;
}

void get_last_coords(const coord_buf_t *pCoordBuf, int *xCoord, int *yCoord,
                     time_stamp_t *timestamp)
{
	int previndex = (pCoordBuf->head + pCoordBuf->numItems - 1) % pCoordBuf->size;
	coord_t *coord = &pCoordBuf->pCoords[previndex];

	if (xCoord)
	{
		*xCoord = coord->x;
	}

	if (yCoord)
	{
		*yCoord = coord->y;
	}

	if (timestamp)
	{
		*timestamp = coord->timeStamp;
	}
}

void init_gesture_state_machine(const general_settings_t *pGeneralSettings,
                                int maxFingers)
{
	int i;

	spGeneralSettings = pGeneralSettings;

	for (i = 0 ; i < maxFingers * 2; i++)
	{
		finger_t *finger = malloc(sizeof(finger_t));
		create_coord_buffer(&finger->coords, pGeneralSettings->coordBufSize);
		finger->state.state = UNUSED;
		g_queue_push_tail(&availableFingers, finger);
	}
}


void
deinit_gesture_state_machine(void)
{
	finger_t *finger = NULL;

	while ((finger = g_queue_pop_head(&availableFingers)) != NULL)
	{
		free_coord_buffer(&finger->coords);
		free(finger);
	}

	finger = NULL;
	GList *l = sFingers;

	while (l != NULL)
	{
		if (l->data)
		{
			finger = (finger_t *)l->data;
			free_coord_buffer(&finger->coords);
		}

		l = g_list_remove(l, l->data);

		if (finger)
		{
			free(finger);
			finger = NULL;
		}
	}
}

void
reset_state_data(gesture_state_data_t *pStateData)
{
	pStateData->state = START_STATE;
	pStateData->insideTapRadius = true;
}

static void add_new_finger(int x, int y, int weight,
                           const time_stamp_t *pCurTime)
{
	finger_t *finger = g_queue_pop_head(&availableFingers);

	if (!finger)
	{
		nyx_info("No available finger buffers, rejecting finger\n");
		return;
	}

	//  ASSERT(finger->state.state == UNUSED);
	reset_state_data(&finger->state);
	finger->id = curFingerId++;
	finger->timestamp = *pCurTime;
	//hal_info"NEW: %ld,%ld\n",finger->id.time.tv_sec,finger->id.time.tv_nsec);
	finger->minDist = 0;
	finger->minDistId = 0;
	finger->lastWeight = weight;
	reset_coord_buffer(&finger->coords);
	update_coord_buffer(&finger->coords, x, y, pCurTime);
	nyx_info("Finger down at %d,%d\n", x, y);
	sFingers = g_list_prepend(sFingers, finger);
}


#define MAX_EVENTS_PER_UPDATE 100

/*
 * Finger tracking:
 * The hardware does not do any fingertracking, so we do it all here.
 */
void
gesture_state_machine(int *pXCoords, int *pYCoords, const int *pFingerWeights,
                      int numFingers, const time_stamp_t *pCurTime,
                      input_event_t *events, int *numEvents)
{
	/* Update Fingers */
	int j;
	int timestmpcnt = 0;
	GList *list;

	//For each new finger
	for (j = 0; j < numFingers; j++)
	{
		int minDist = INT_MAX;
		GList *minId = NULL;
		list = g_list_first(sFingers);

		//Try and match it against one of the existing ones
		while (list)
		{
			int xPrev, yPrev;
			int matched = 0;
			finger_t *finger = (finger_t *)list->data;

			get_last_coords(&finger->coords, &xPrev, &yPrev, NULL);
			int dx = pXCoords[j] - xPrev;
			int dy = pYCoords[j] - yPrev;
			int dist = (dx * dx + dy * dy);

			//Another finger from the input list is already a better match.
			if (dist > finger->minDist)
			{
				list = g_list_next(list);
				continue;
			}

			if (dist < minDist)
			{
				matched = 1;
			}

			if (matched)
			{
				minId = list;
				minDist = dist;
			}

			list = g_list_next(list);
		}

		if (minId)
		{
			finger_t *finger = (finger_t *)minId->data;
			finger->minDistId = j;
			finger->minDist = minDist;
		}
	}

	//Okay, at this point, for each existing finger, (sFingers)
	//We have set minDistId to the index of the finger in the input array
	//Or it's set to INT_MAX if it didn't match any of the new fingers.

	//Iterate through the fingerList, and update each of the fingers that has a match with new coordinates.
	list = g_list_first(sFingers);

	while (list)
	{
		finger_t *finger = (finger_t *)list->data;

		//Finger released
		if (finger->minDist == INT_MAX)
		{
			list = g_list_next(list);
			continue;
		}

		nyx_info("New coord (at: %d), %d,%d weight: %d, distance: %d\n",
		         finger->minDistId, pXCoords[finger->minDistId], pYCoords[finger->minDistId],
		         pFingerWeights[finger->minDistId], finger->minDist);

		//Let's ignore the coordinate if there was a huge difference in weight
		//This is a common scenario when the user is releasing his finger.
		if (finger->lastWeight / 2 < pFingerWeights[finger->minDistId])
		{
			update_coord_buffer(&finger->coords, pXCoords[finger->minDistId],
			                    pYCoords[finger->minDistId], pCurTime);
		}
		else
		{
			nyx_info("Ignoring coordinate\n");
		}

		finger->lastWeight = pFingerWeights[finger->minDistId];
		//remove finger from pool of "new" fingers.
		pXCoords[finger->minDistId] = pYCoords[finger->minDistId] = 0;

		finger->minDist = 0;
		finger->minDistId = 0;
		list = g_list_next(list);
	}

	//Now go through the list and find any new unmatched fingers
	for (j = 0; j < numFingers; j++)
	{
		time_stamp_t ts = *pCurTime;

		/* When dragging over the gesture button, we get spurious
		 * "extra" fingers Disregard these events if we already
		 * matched a finger in the gesture area
		 */
		if (pXCoords[j] == 0 && pYCoords[j] == 0)
		{
			continue;
		}

		if (pFingerWeights[j] < g_atomic_int_get(
		            &spGeneralSettings->fingerDownThreshold))
		{
			nyx_info("Discarding finger with too low weight (%d)\n", pFingerWeights[j]);
			continue;
		}

		nyx_info("pFingerWeight: %d (%d)\n", pFingerWeights[j], j);

		nyx_info("j: %d, %d) New finger @ %d,%d\n", j, numFingers, pXCoords[j],
		         pYCoords[j]);

		ts.time.tv_nsec += timestmpcnt;
		timestmpcnt += 1000000;
		add_new_finger(pXCoords[j], pYCoords[j], pFingerWeights[j], &ts);
	}

	/* All fingers has been matched, now let's process the changes */
	list = g_list_first(sFingers);

	while (list)
	{
		finger_t *finger = (finger_t *)list->data;

		//-1 means to move the list element into the available list
		if (gesture_state_machine_finger(finger, events, numEvents) == -1)
		{
			finger->state.state = UNUSED;
			list = g_list_next(list);
			sFingers = g_list_remove(sFingers, finger);
			g_queue_push_tail(&availableFingers, finger);
		}
		else
		{
			list = g_list_next(list);
		}
	}

	if (0 < *numEvents)
	{
		//ASSERT(numEvents < MAX_EVENTS_PER_UPDATE);
		/* add EV_SYN event */
		set_event_params(&events[(*numEvents)++], (time_stamp_t *) pCurTime, EV_SYN, 0,
		                 0);
	}
}

int gesture_state_machine_finger(finger_t *finger, input_event_t *events,
                                 int *numEvents)
{
	int x, y;
	time_stamp_t timestamp;

	get_last_coords(&finger->coords, &x, &y, &timestamp);

	finger->numEvents = *numEvents;
	finger->events = events;

	set_event_params(&finger->events[finger->numEvents++], &timestamp, EV_FINGERID,
	                 0 , finger->id);

	switch (finger->state.state)
	{
		case START_STATE:
		{
			finger->state.start[X_DIM] = x;
			finger->state.start[Y_DIM] = y;
			finger->state.startTime = timestamp;
			finger->state.state = FINGER_DOWN_STATE;
			set_event_params(&finger->events[finger->numEvents++], &timestamp, EV_KEY,
			                 BTN_TOUCH, 1);
		}
		break;

		case FINGER_DOWN_STATE:
			break;

		case FINGER_DOWN_AFTER_QUICK_LAUNCH:
		{
			/* keep reporting pen moves until the finger comes up
			 * LunaSysMgr will handle the finger until release
			 */
		}
		break;

		case UNUSED:
			//ASSERT(0);
			break;
	}

	set_event_params(&finger->events[finger->numEvents++], &timestamp, EV_ABS,
	                 ABS_X, x);
	set_event_params(&finger->events[finger->numEvents++], &timestamp, EV_ABS,
	                 ABS_Y, y);
	*numEvents = finger->numEvents;

	if (finger->minDist > 0)
	{
		//send finger release event
		nyx_info("Finger up at %d,%d\n", x, y);
		set_event_params(&finger->events[finger->numEvents++], &timestamp, EV_KEY,
		                 BTN_TOUCH, 0);
		*numEvents = finger->numEvents;
		return -1;
	}
	else
	{
		finger->minDist = INT_MAX;
	}

	return 0;
}
