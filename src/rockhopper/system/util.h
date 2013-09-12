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

/*
*******************************************************************
* @file util.h
*******************************************************************
*/

#ifndef _UTIL_H_
#define _UTIL_H_

#include <glib.h>
#include "json_utils.h"


void disable_lifetime_timer();

void reset_lifetime_timer();

#define LSREPORT(lse) g_critical("in %s: %s => %s", __func__, \
                                  (lse).func, (lse).message)

#define SHOW_STDERR(standard_error) \
    if (standard_error != NULL) { \
        if (strlen(standard_error) > 0) { \
            g_critical("%s: stderr: %s", __func__, standard_error); \
        } \
        g_free(standard_error); \
        standard_error = NULL; \
    }

#define SHOW_ERROR(error) \
    if (error != NULL) { \
        g_critical("%s: error=>%s", __func__, error->message); \
        g_error_free(error); \
        error = NULL; \
    }


#define LSTRACE_LSMESSAGE(message) \
    do { \
        const gchar * payloadstr = LSMessageGetPayload(message); \
        json_t* payload = json_parse_document(payloadstr); \
        g_debug("%s(%s)", __func__, (NULL == payloadstr) ? "{}" : payloadstr); \
        const char *errorCode; \
        const char *errorText; \
        if (json_get_string(payload, "errorCode", &errorCode)) { \
            g_warning("%s: called with errorCode = %s\n", __FUNCTION__, errorCode); \
        } \
        if (json_get_string(payload, "errorText", &errorText)) { \
            g_warning("%s: called with errorText = %s\n", __FUNCTION__, errorText); \
        } \
        if (payload) \
            json_free_value(&payload); \
    } while (0)


/**
 *  write to syslog a list of apps that have open file descriptors inside some
 *  directory.  Typically this will be used to "blame" folks keeping files
 *  open in /media/internal that prevent us from cleanly unmounting the
 *  partition there when going into brick mode.
 *
 * @param dirPath        fully-qualified path describing directory to search
 *                       for files.
 */
void log_blame(const char *dirPath);


#endif
