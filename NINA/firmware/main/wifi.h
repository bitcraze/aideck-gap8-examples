/*
 * Copyright (C) 2020 Bitcraze AB
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stdbool.h>

#ifndef __WIFI_H__
#define __WIFI_H__

/* The SSID of the AI-deck when in AP mode */
#define WIFI_SSID "Bitcraze AI-deck example"

/* The port used for the example */
#define PORT 5000

/* The different modes the WiFi can be initialized as */
typedef enum { 
    AIDECK_WIFI_MODE_SOFTAP, /* Act as access-point */
    AIDECK_WIFI_MODE_STATION /* Connect to an access-point */
} WiFiMode_t;

/* Initialize the WiFi */
void wifi_init(WiFiMode_t mode, const char * ssid, const char * key);

/* Wait (and block) until a connection comes in */
void wifi_wait_for_socket_connected();

/* Bind socket for incomming connections */
void wifi_bind_socket();

/* Check if a client is connected */
bool wifi_is_socket_connected();

/* Wait (and block) for a client to disconnect*/
void wifi_wait_for_disconnect();

/* Send a packet to the client from buffer of size */
void wifi_send_packet(const char * buffer, size_t size);

#endif