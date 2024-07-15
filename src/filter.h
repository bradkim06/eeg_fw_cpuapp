#ifndef __FILTER_H__
#define __FILTER_H__

#include <zephyr/kernel.h>
#include <arm_math.h>
#include <arm_const_structs.h>

float32_t filteringEEGData(float32_t input, int channel);

#endif
