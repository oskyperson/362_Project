#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void buzzer_init(uint32_t gpio_pin);

void buzzer_play_tone(uint32_t freq_hz, uint32_t duration_ms);

void buzzer_play_sequence(const uint32_t *freqs,
                          const uint32_t *durations_ms,
                          size_t count);

void buzzer_stop(void);

void buzzer_update(void);

bool buzzer_is_playing(void);

#ifdef __cplusplus
}
#endif

#endif
