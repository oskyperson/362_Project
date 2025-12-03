#include "buzzer.h"
#include <stdio.h>

#ifdef PICO_ON_DEVICE
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#endif

static uint32_t g_buzzer_pin = 0;
static bool g_initialized = false;
static volatile bool g_playing = false;

#ifdef PICO_ON_DEVICE
static uint g_slice = 0;
static uint g_channel = 0;
static absolute_time_t g_end_time;
static bool g_has_end_time = false;
static bool g_sequence_active = false;
static const uint32_t *g_seq_freqs = NULL;
static const uint32_t *g_seq_durs = NULL;
static size_t g_seq_count = 0;
static size_t g_seq_index = 0;

static void buzzer_start_tone(uint32_t freq_hz, uint32_t duration_ms)
{
    // Enforce max 1 second duration
    if (duration_ms > 1000) {
        duration_ms = 1000;
    }

    if (freq_hz == 0) {
        pwm_set_chan_level(g_slice, g_channel, 0);
        pwm_set_enabled(g_slice, false);
        g_playing = false;
        g_has_end_time = false;
        return;
    }

    uint32_t sys_clk = clock_get_hz(clk_sys);
    float div = 1.0f;
    uint32_t wrap = (uint32_t)((float)sys_clk / (div * (float)freq_hz)) - 1u;

    while (wrap > 65535u && div < 256.0f) {
        div *= 2.0f;
        wrap = (uint32_t)((float)sys_clk / (div * (float)freq_hz)) - 1u;
    }

    if (wrap < 1u) {
        wrap = 1u;
    }

    pwm_set_clkdiv(g_slice, div);
    pwm_set_wrap(g_slice, wrap);
    pwm_set_chan_level(g_slice, g_channel, wrap / 2u);
    pwm_set_enabled(g_slice, true);

    g_playing = true;
    if (duration_ms > 0) {
        g_end_time = make_timeout_time_ms(duration_ms);
        g_has_end_time = true;
    } else {
        g_has_end_time = false;
    }
}

#endif

void buzzer_init(uint32_t gpio_pin)
{
    g_buzzer_pin = gpio_pin;
    g_initialized = true;
#ifdef PICO_ON_DEVICE
    gpio_set_function(g_buzzer_pin, GPIO_FUNC_PWM);
    g_slice = pwm_gpio_to_slice_num(g_buzzer_pin);
    g_channel = pwm_gpio_to_channel(g_buzzer_pin);
    pwm_set_chan_level(g_slice, g_channel, 0);
    pwm_set_enabled(g_slice, false);
    g_has_end_time = false;
    g_sequence_active = false;
    g_seq_freqs = NULL;
    g_seq_durs = NULL;
    g_seq_count = 0;
    g_seq_index = 0;
#else
    (void)g_buzzer_pin;
#endif
}

void buzzer_play_tone(uint32_t freq_hz, uint32_t duration_ms)
{
   
    if (!g_initialized) {
        return;
    }

#ifdef PICO_ON_DEVICE
    g_sequence_active = false;
    g_seq_freqs = NULL;
    g_seq_durs = NULL;
    g_seq_count = 0;
    g_seq_index = 0;
    buzzer_start_tone(freq_hz, duration_ms);
#else
    g_playing = (freq_hz != 0);
    if (g_playing) {
        printf("[buzzer] play %u Hz for %u ms on pin %u\n",
               (unsigned)freq_hz,
               (unsigned)duration_ms,
               (unsigned)g_buzzer_pin);
    } else {
        printf("[buzzer] stop\n");
    }
    (void)duration_ms;
#endif
}
// static void buzzer_play_rising_sweep(uint32_t start_freq,
//                                      uint32_t end_freq,
//                                      uint32_t sweep_time_ms,
//                                      uint32_t step_ms)
// {
//     uint32_t steps = sweep_time_ms / step_ms;
//     if (steps < 1) steps = 1;
//     if (steps > 256) steps = 256;

//     float delta = (float)(end_freq - start_freq) / (float)steps;

//     static uint32_t freqs[256];
//     static uint32_t durs[256];

//     for (uint32_t i = 0; i < steps; i++) {
//         freqs[i] = (uint32_t)(start_freq + delta * i);
//         durs[i]  = step_ms;
//     }

//     buzzer_play_sequence(freqs, durs, steps);
// }

void buzzer_play_sequence(const uint32_t *freqs,
                          const uint32_t *durations_ms,
                          size_t count)
{
    if (!g_initialized || freqs == NULL || durations_ms == NULL || count == 0) {
        return;
    }

#ifdef PICO_ON_DEVICE
    g_sequence_active = true;
    g_seq_freqs = freqs;
    g_seq_durs = durations_ms;
    g_seq_count = count;
    g_seq_index = 0;
    buzzer_start_tone(g_seq_freqs[0], g_seq_durs[0]);
#else
    printf("[buzzer] play sequence of %zu notes\n", count);
    for (size_t i = 0; i < count; ++i) {
        printf("  note %zu: %u Hz for %u ms\n",
               i,
               (unsigned)freqs[i],
               (unsigned)durations_ms[i]);
    }
    g_playing = true;
#endif
}

void buzzer_stop(void)
{
    if (!g_initialized) {
        return;
    }

#ifdef PICO_ON_DEVICE
    pwm_set_chan_level(g_slice, g_channel, 0);
    pwm_set_enabled(g_slice, false);
    g_has_end_time = false;
    g_sequence_active = false;
    g_seq_freqs = NULL;
    g_seq_durs = NULL;
    g_seq_count = 0;
    g_seq_index = 0;

    // Rising sweep ending sound: 600 Hz → 1800 Hz over 200 ms
    //buzzer_play_rising_sweep(600, 1800, 200, 10);
#endif

    g_playing = false;
}

void buzzer_update(void)
{
    if (!g_initialized || !g_playing) {
        return;
    }

#ifdef PICO_ON_DEVICE
    if (!g_has_end_time) {
        return;
    }

    if (absolute_time_diff_us(get_absolute_time(), g_end_time) <= 0) {
        if (g_sequence_active && g_seq_index + 1 < g_seq_count) {
            g_seq_index++;
            buzzer_start_tone(g_seq_freqs[g_seq_index], g_seq_durs[g_seq_index]);
        } else {
            buzzer_stop();
        }
    }
#else
    (void)g_buzzer_pin;
#endif
}

bool buzzer_is_playing(void)
{
    return g_playing;
}
