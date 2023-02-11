#pragma once

#include "esphome/core/component.h"

uint16_t computeCRC16(uint16_t fcs, const uint8_t* cp, uint32_t len);
