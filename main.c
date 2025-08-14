#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"

// Bibliotecas do projeto
#include "ssd1306.h"
#include "font.h"
#include "gy33.h"
#include "bh1750_light_sensor.h"
#include "buttons.h"
#include "leds.h"

// --- I2C e Display ---
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define DISPLAY_ADDRESS 0x3C

// I2C para o sensor BH1750 (GY-33 usa a sua própria configuração em sua biblioteca)
#define I2C_PORT_BH1750 i2c0
#define I2C_SDA_BH1750 0
#define I2C_SCL_BH1750 1

// --- Variáveis de Estado Globais ---
volatile int led_color_state = 0;      // 0:Vermelho, 1:Amarelo, 2:Verde, 3:Azul
volatile int display_mode = 0;         // 0: Tela do GY-33 (Cores), 1: Tela do BH1750 (Luz)
volatile uint32_t last_press_time = 0; // Para debounce dos botões

// --- Protótipos das Funções de Desenho ---
void draw_gy33_screen(ssd1306_t *ssd, uint16_t r, uint16_t g, uint16_t b, uint16_t c);
void draw_bh1750_screen(ssd1306_t *ssd, uint16_t lux);

// --- Callback Handler para todos os botões ---
void all_buttons_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_press_time < 250)
    { // Debounce de 250ms
        return;
    }
    last_press_time = current_time;

    if (gpio == BUTTON_A_PIN)
    { // Troca a cor do LED
        led_color_state++;
        if (led_color_state > 3)
        {
            led_color_state = 0;
        }
    }
    else if (gpio == BUTTON_B_PIN)
    { // Entra em modo BOOTSEL
        reset_usb_boot(0, 0);
    }
    else if (gpio == BUTTON_C_PIN)
    { // Troca o modo do display
        display_mode = !display_mode;
    }
}

int main()
{
    stdio_init_all();
    sleep_ms(2000); // Espera para o terminal serial conectar

    // Inicializa todos os periféricos usando as bibliotecas
    buttons_init(); // Configura os botões A, B, C
    led_init();
    gy33_init();

    // Configura o I2C para o sensor BH1750
    i2c_init(I2C_PORT_BH1750, 100 * 1000);
    gpio_set_function(I2C_SDA_BH1750, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_BH1750, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_BH1750);
    gpio_pull_up(I2C_SCL_BH1750);
    bh1750_power_on(I2C_PORT_BH1750);

    // Configura o I2C e o Display SSD1306
    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISP);
    gpio_pull_up(I2C_SCL_DISP);
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, DISPLAY_ADDRESS, I2C_PORT_DISP);
    ssd1306_config(&ssd);

    // Registra a função de callback para as interrupções dos botões
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &all_buttons_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &all_buttons_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_C_PIN, GPIO_IRQ_EDGE_FALL, true, &all_buttons_irq_handler);

    while (1)
    {
        if (display_mode == 0)
        { // Modo Sensor de Cor
            uint16_t r, g, b, c;
            gy33_read_color(&r, &g, &b, &c);
            printf("Modo Cor - R: %d, G: %d, B: %d, C: %d\n", r, g, b, c);
            draw_gy33_screen(&ssd, r, g, b, c);

            if (led_color_state == 0)
                acender_led_rgb(255, 0, 0); // Vermelho
            else if (led_color_state == 1)
                acender_led_rgb(255, 255, 0); // Amarelo
            else if (led_color_state == 2)
                acender_led_rgb(0, 255, 0); // Verde
            else if (led_color_state == 3)
                acender_led_rgb(0, 0, 255); // Azul
        }
        else
        { // Modo Sensor de Luz
            uint16_t lux = bh1750_read_measurement(I2C_PORT_BH1750);
            printf("Modo Luz - Lux: %d\n", lux);
            draw_bh1750_screen(&ssd, lux);
            turn_off_leds(); // Desliga os LEDs neste modo
        }

        sleep_ms(200);
    }
}

void draw_gy33_screen(ssd1306_t *ssd, uint16_t r, uint16_t g, uint16_t b, uint16_t c)
{
    char str_r[8], str_g[8], str_b[8], str_c[8];
    sprintf(str_r, "%d R", r);
    sprintf(str_g, "%d G", g);
    sprintf(str_b, "%d B", b);
    sprintf(str_c, "%d C", c);

    ssd1306_fill(ssd, false);
    ssd1306_draw_string(ssd, "Sensor de Cor", 8, 6);
    ssd1306_draw_string(ssd, "GY-33", 35, 16);
    ssd1306_line(ssd, 3, 28, 123, 28, true);
    ssd1306_draw_string(ssd, str_r, 10, 35);
    ssd1306_draw_string(ssd, str_g, 10, 48);
    ssd1306_draw_string(ssd, str_b, 70, 35);
    ssd1306_draw_string(ssd, str_c, 70, 48);
    ssd1306_send_data(ssd);
}

void draw_bh1750_screen(ssd1306_t *ssd, uint16_t lux)
{
    char str_lux[15];
    sprintf(str_lux, "%d Lux", lux);

    ssd1306_fill(ssd, false);
    ssd1306_draw_string(ssd, "Sensor de Luz", 8, 6);
    ssd1306_draw_string(ssd, "BH1750", 35, 16);
    ssd1306_line(ssd, 3, 28, 123, 28, true);
    ssd1306_draw_string(ssd, str_lux, 25, 40);
    ssd1306_send_data(ssd);
}