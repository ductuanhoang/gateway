/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef H_USER_BUTTONS_
#define H_USER_BUTTONS_
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum
    {
        E_USER_BUTTON_HOLD
    } e_user_button;

    typedef void (*button_callback_t)(int, int);

    void user_button_init(void);

    void user_button_callback_init(button_callback_t callback);

    void user_control_relay(uint8_t state);
#ifdef __cplusplus
}
#endif

#endif // H_USER_BUTTONS_
