#ifndef __FILTER_H__
#define __FILTER_H__

#include <zephyr/kernel.h>
#include <arm_math.h>
#include <arm_const_structs.h>

void filteringEEGData(float32_t *input, float32_t *output, uint32_t blockSize);

#endif
