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

#ifndef KEYS_COMMON_H_
#define KEYS_COMMON_H_

typedef struct {
    nyx_device_t _parent;
    nyx_event_keys_t* current_event_ptr;
} keys_device_t;

int lookup_key(keys_device_t* d, uint16_t keyCode, int32_t keyValue,
        nyx_key_type_t* key_type_out_ptr);

#endif
