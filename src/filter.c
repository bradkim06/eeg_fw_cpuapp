#include "filter.h"

#define BLOCK_SIZE 1
#define HIGHPASS_FILTER_ORDER 401 // Reduced order for each filter
#define LOWPASS_FILTER_ORDER 401 // Reduced order for each filter
#define HIGHPASS_FILTER_LEN (HIGHPASS_FILTER_ORDER + 1)
#define LOWPASS_FILTER_LEN (LOWPASS_FILTER_ORDER + 1)
#define SAMPLING_RATE 250
#define HIGH_CUTOFF 2.0f
#define LOW_CUTOFF 40.0f

#define ADS1299_CHANNELS 2

// FIR filter instances
arm_fir_instance_f32 hp_instance;
arm_fir_instance_f32 lp_instance;

// Filter coefficients and state buffers
static float32_t hp_coeffs[HIGHPASS_FILTER_LEN];
static float32_t lp_coeffs[LOWPASS_FILTER_LEN];
static float32_t hp_state[ADS1299_CHANNELS]
			 [BLOCK_SIZE + HIGHPASS_FILTER_LEN - 1];
static float32_t lp_state[ADS1299_CHANNELS][BLOCK_SIZE + LOWPASS_FILTER_LEN - 1];

void calculate_hp_coeffs(float32_t *coeffs, uint16_t order, float32_t cutoff,
			 float32_t sampling_rate)
{
	float32_t fc = cutoff / sampling_rate;

	for (int n = 0; n <= order; n++) {
		if (n == order / 2) {
			coeffs[n] = 1.0f - 2.0f * fc;
		} else {
			float32_t nm = (float32_t)n - (float32_t)order / 2.0f;
			coeffs[n] =
				-arm_sin_f32(2.0f * PI * fc * nm) / (PI * nm);
		}
		// Apply Blackman window
		coeffs[n] *= (0.42f -
			      0.5f * arm_cos_f32(2.0f * PI * (float32_t)n /
						 (float32_t)order) +
			      0.08f * arm_cos_f32(4.0f * PI * (float32_t)n /
						  (float32_t)order));
	}

	// Normalize coefficients
	float32_t sum = 0.0f;
	for (int n = 0; n <= order; n++) {
		sum += coeffs[n];
	}
	for (int n = 0; n <= order; n++) {
		coeffs[n] /= sum;
	}
}

void calculate_lp_coeffs(float32_t *coeffs, uint16_t order, float32_t cutoff,
			 float32_t sampling_rate)
{
	float32_t fc = cutoff / sampling_rate;

	for (int n = 0; n <= order; n++) {
		if (n == order / 2) {
			coeffs[n] = 2.0f * fc;
		} else {
			float32_t nm = (float32_t)n - (float32_t)order / 2.0f;
			coeffs[n] =
				arm_sin_f32(2.0f * PI * fc * nm) / (PI * nm);
		}
		// Apply Blackman window
		coeffs[n] *= (0.42f -
			      0.5f * arm_cos_f32(2.0f * PI * (float32_t)n /
						 (float32_t)order) +
			      0.08f * arm_cos_f32(4.0f * PI * (float32_t)n /
						  (float32_t)order));
	}

	// Normalize coefficients
	float32_t sum = 0.0f;
	for (int n = 0; n <= order; n++) {
		sum += coeffs[n];
	}
	for (int n = 0; n <= order; n++) {
		coeffs[n] /= sum;
	}
}

int initFilters(void)
{
	// Calculate highpass filter coefficients
	calculate_hp_coeffs(hp_coeffs, HIGHPASS_FILTER_ORDER, HIGH_CUTOFF,
			    SAMPLING_RATE);

	// Calculate lowpass filter coefficients
	calculate_lp_coeffs(lp_coeffs, LOWPASS_FILTER_ORDER, LOW_CUTOFF,
			    SAMPLING_RATE);

	// Initialize highpass and lowpass filters for each channel
	for (int i = 0; i < ADS1299_CHANNELS; i++) {
		arm_fir_init_f32(&hp_instance, HIGHPASS_FILTER_LEN, hp_coeffs,
				 hp_state[i], BLOCK_SIZE);
		arm_fir_init_f32(&lp_instance, LOWPASS_FILTER_LEN, lp_coeffs,
				 lp_state[i], BLOCK_SIZE);
	}

	return 0;
}

float32_t filteringEEGData(float32_t input, int channel)
{
	float32_t hp_output, lp_output;

	// Apply highpass filter
	arm_fir_f32(&hp_instance, &input, &hp_output, BLOCK_SIZE);

	// Apply lowpass filter
	arm_fir_f32(&lp_instance, &hp_output, &lp_output, BLOCK_SIZE);

	return lp_output;
}

SYS_INIT(initFilters, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
