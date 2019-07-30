#ifndef _HASH_H
#define _HASH_H

#include "octypes.h"

void Hash(OCUUIdentity *id, const char *uid);
void Hash(OCUUIdentity *id, uint8_t *ipui, uint8_t *emc);

#endif