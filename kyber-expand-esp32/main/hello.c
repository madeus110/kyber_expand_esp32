#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_random.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "fips202.h"  // SHAKE256

#define TRIGGER_GPIO 2

static inline void trigger_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << TRIGGER_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(TRIGGER_GPIO, 0);
}

static inline void trigger_high(void) { gpio_set_level(TRIGGER_GPIO, 1); }
static inline void trigger_low(void)  { gpio_set_level(TRIGGER_GPIO, 0); }

/// generate seed 32b
void gen_seed(uint8_t *seed, size_t len) {
    // trng 256 bytes
    uint8_t entropy[256];
    for (int i = 0; i < sizeof(entropy); i += 4) {
        uint32_t r = esp_random();
        memcpy(entropy + i, &r, 4);
    }

    // shake256
    keccak_state st;
    shake256_init(&st);
    shake256_absorb(&st, entropy, sizeof(entropy));
    shake256_finalize(&st);

    // squeeze 32 bytes
    shake256_squeeze(seed, len, &st);
}

void app_main(void) {
    trigger_init();
    uint8_t seed[32];

    int64_t t0 = esp_timer_get_time();
    trigger_high();
    gen_seed(seed, sizeof(seed));
    trigger_low();
    int64_t t1 = esp_timer_get_time();

    printf("Seed TRNG+SHAKE256 (%lld us):\n", (long long int)(t1 - t0));
    for (int i = 0; i < 32; i++) {
        printf("%02X", seed[i]);
    }
    printf("\n");
}

