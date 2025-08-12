#pragma once
#ifndef _VALIDATE_SIGNATURE_H_
#define _VALIDATE_SIGNATURE_H_

#include "stdint.h"
#include "stdio.h"

bool verifySignature(const uint8_t* firmwareData, size_t firmwareLen,
  const uint8_t* sigData, size_t sigLen);

#endif