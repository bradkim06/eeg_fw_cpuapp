#include "filter.h"

#define SAMPLE_RATE 250.0f // EEG 샘플링 레이트 (Hz)
#define HIGHPASS_CUTOFF 0.5f // 하이패스 차단 주파수 (Hz)
#define LOWPASS_CUTOFF 30.0f // 로우패스 차단 주파수 (Hz)
#define FILTER_ORDER 8 // 필터 차수 (2의 배수여야 함)
#define BLOCK_SIZE 1 // 처리할 블록 크기

// 필터 인스턴스
arm_biquad_casd_df1_inst_f32 highpass_instance;
arm_biquad_casd_df1_inst_f32 lowpass_instance;

// 필터 상태 변수
static float32_t highpass_state[4 * FILTER_ORDER];
static float32_t lowpass_state[4 * FILTER_ORDER];

// 필터 계수
static float32_t highpass_coeffs[5 * FILTER_ORDER];
static float32_t lowpass_coeffs[5 * FILTER_ORDER];

// 임시 버퍼
static float32_t temp_buffer[BLOCK_SIZE];

// 하이패스 필터 계수 계산 함수
void calculate_highpass_coeffs(float32_t cutoff_freq, float32_t sample_rate,
			       float32_t *coeffs)
{
	float32_t w0 = 2.0f * PI * cutoff_freq / sample_rate;
	float32_t alpha =
		sinf(w0) / (2.0f * 0.707f); // Q factor = 0.707 (Butterworth)

	float32_t a0 = 1.0f + alpha;
	float32_t a1 = -2.0f * cosf(w0);
	float32_t a2 = 1.0f - alpha;
	float32_t b0 = (1.0f + cosf(w0)) / 2.0f;
	float32_t b1 = -(1.0f + cosf(w0));
	float32_t b2 = (1.0f + cosf(w0)) / 2.0f;

	// 정규화
	b0 /= a0;
	b1 /= a0;
	b2 /= a0;
	a1 /= a0;
	a2 /= a0;

	// CMSIS-DSP 형식으로 계수 설정
	coeffs[0] = b0;
	coeffs[1] = b1;
	coeffs[2] = b2;
	coeffs[3] = -a1;
	coeffs[4] = -a2;

	// 4차 필터를 위해 계수 복사
	for (int i = 1; i < FILTER_ORDER / 2; i++) {
		for (int j = 0; j < 5; j++) {
			coeffs[5 * i + j] = coeffs[j];
		}
	}
}

// 로우패스 필터 계수 계산 함수
void calculate_lowpass_coeffs(float32_t cutoff_freq, float32_t sample_rate,
			      float32_t *coeffs)
{
	float32_t w0 = 2.0f * PI * cutoff_freq / sample_rate;
	float32_t alpha = sinf(w0) / (2.0f * 0.707f);

	float32_t a0 = 1.0f + alpha;
	float32_t a1 = -2.0f * cosf(w0);
	float32_t a2 = 1.0f - alpha;
	float32_t b0 = (1.0f - cosf(w0)) / 2.0f;
	float32_t b1 = 1.0f - cosf(w0);
	float32_t b2 = (1.0f - cosf(w0)) / 2.0f;

	// 정규화 및 CMSIS-DSP 형식으로 계수 설정
	coeffs[0] = b0 / a0;
	coeffs[1] = b1 / a0;
	coeffs[2] = b2 / a0;
	coeffs[3] = -a1 / a0;
	coeffs[4] = -a2 / a0;

	// 4차 필터를 위해 계수 복사
	for (int i = 1; i < FILTER_ORDER / 2; i++) {
		for (int j = 0; j < 5; j++) {
			coeffs[5 * i + j] = coeffs[j];
		}
	}
}

int initFilters(void)
{
	// 하이패스 필터 초기화
	calculate_highpass_coeffs(HIGHPASS_CUTOFF, SAMPLE_RATE,
				  highpass_coeffs);
	arm_biquad_cascade_df1_init_f32(&highpass_instance, FILTER_ORDER / 2,
					highpass_coeffs, highpass_state);

	// 로우패스 필터 초기화
	calculate_lowpass_coeffs(LOWPASS_CUTOFF, SAMPLE_RATE, lowpass_coeffs);
	arm_biquad_cascade_df1_init_f32(&lowpass_instance, FILTER_ORDER / 2,
					lowpass_coeffs, lowpass_state);

	return 0;
}

void filteringEEGData(float32_t *input, float32_t *output, uint32_t blockSize)
{
	// 하이패스 필터링
	arm_biquad_cascade_df1_f32(&highpass_instance, input, temp_buffer,
				   blockSize);

	// 로우패스 필터링 (밴드패스 효과)
	arm_biquad_cascade_df1_f32(&lowpass_instance, temp_buffer, output,
				   blockSize);
}

SYS_INIT(initFilters, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
