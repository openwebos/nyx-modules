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

#ifndef CHARGER_H_
#define CHARGER_H_

nyx_error_t _charger_init(void);
nyx_error_t _charger_read_status(nyx_charger_status_t *status);
nyx_error_t _charger_enable_charging(nyx_charger_status_t *status);
nyx_error_t _charger_disable_charging(nyx_charger_status_t *status);
nyx_error_t _charger_query_charger_event(nyx_charger_event_t *event);

#endif
