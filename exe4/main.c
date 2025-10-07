/**
* Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
*
* SPDX-License-Identifier: BSD-3-Clause
*/


#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED = 4;

const float conversion_factor = 3.3f / (1 << 12);

/**
* 0..1.0V: Desligado
* 1..2.0V: 150 ms
* 2..3.3V: 400 ms
*/

struct contexto_led {
    int periodo_ms; 
    int tick_ms;    
    int nivel;     
};

static bool callback_piscar(struct repeating_timer *t) {
    struct contexto_led *contexto = (struct contexto_led *)t->user_data;

    if (contexto->periodo_ms <= 0) {
        contexto->nivel = 0;
        gpio_put(PIN_LED, 0);
        return true;
    }

    contexto->tick_ms++;

    if (contexto->tick_ms >= (contexto->periodo_ms * 2)) {
        contexto->tick_ms = 0;
        contexto->nivel = !contexto->nivel;
        gpio_put(PIN_LED, contexto->nivel);
    }
    return true;
}

void inicializar_gpio() {
    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);
    gpio_put(PIN_LED, 0);
}

void inicializar_adc() {
    adc_init();
    adc_gpio_init(28);     
    adc_select_input(2);
}

void configurar_zona(struct contexto_led *ctx, int zona) {
    if (zona == 0) {
        ctx->periodo_ms = 0;
        ctx->tick_ms = 0;
        ctx->nivel = 0;
        gpio_put(PIN_LED, 0);
    }
    else if (zona == 1) {
        ctx->periodo_ms = 300;
        ctx->tick_ms = 0;
    }
    else {
        ctx->periodo_ms = 500;
        ctx->tick_ms = 0;
    }
}

int main() {
    stdio_init_all();

    inicializar_gpio();
    inicializar_adc();

    const int limite1 = (int)(4095.0f * 1.0f / 3.3f + 0.5f);
    const int limite2 = (int)(4095.0f * 2.0f / 3.3f + 0.5f);

    struct contexto_led ctx_led;
    ctx_led.periodo_ms = 300;   
    ctx_led.tick_ms = 0;
    ctx_led.nivel = 0;

    struct repeating_timer timer;
    add_repeating_timer_ms(1, callback_piscar, &ctx_led, &timer);

    int zona_atual = -1;

    while (1) {
        int valor_adc = adc_read();

        int nova_zona;
        if (valor_adc < limite1) {
            nova_zona = 0;
        }   
        else if (valor_adc < limite2) {
            nova_zona = 1;
        }
        else {
            nova_zona = 2;
        }

        if (nova_zona != zona_atual) {
            zona_atual = nova_zona;
            configurar_zona(&ctx_led, zona_atual);
        }
    }
}