#pragma once

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sodium/utils.h>
#include <tox/tox.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DHT_node
{
    const char* ip;
    uint16_t port;
    const char key_hex[TOX_PUBLIC_KEY_SIZE*2 + 1];
} DHT_node;

typedef struct Tox_State
{
    char* id;
    char* name;
} Tox_State;

Tox* create_tox();
void start_tox (Tox* tox);
void stop_tox (Tox* tox);

void bootstrap (Tox* tox);

#ifdef __cplusplus
}
#endif