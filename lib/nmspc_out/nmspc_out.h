#pragma once
#ifndef _NMSPC_OUT_H_
#define _NMSPC_OUT_H_

#include "stdio.h"
namespace outs {
  void init();
  void set_state(uint8_t _pin, bool value);
};

#endif