#pragma once
#ifndef _ALARMS_H_
#define _ALARMS_H_

#include "stdint.h"

void set_alarms(bool alarms[6]);
void set_u8_alarms(uint8_t alarms);
void reset_alarms();

#endif