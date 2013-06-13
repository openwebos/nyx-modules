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
*******************************************
* @file rtc.h
*******************************************
*/


#ifndef _RTC_H_
#define _RTC_H_

#include <linux/rtc.h>

typedef void (*RtcAlarmFunc)(void);

bool rtc_open();
void rtc_close();
bool rtc_add_watch(RtcAlarmFunc func);
bool rtc_clear_watch(void);
bool rtc_wait_alarm();
bool rtc_check_alarm();
bool rtc_alarm_expired(void);
int32_t rtc_getfd();
bool rtc_set_alarm_diff(time_t diff);
bool rtc_set_alarm(struct rtc_wkalrm *alarm);
bool rtc_set_alarm_time(time_t expiry);
bool rtc_clear_alarm();
bool rtc_read_alarm(struct rtc_wkalrm *alarm);
bool rtc_read_alarm_time(time_t *time);
time_t rtc_time(time_t *time);
bool rtc_read(struct tm *rtc_tm);
bool rtc_write(struct tm *tm_time);
bool wall_rtc_diff(time_t *ret_delta);

#endif
