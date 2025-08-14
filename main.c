#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/gpio.h"

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

// I2C para o sensor BH1750
#define I2C_PORT_BH1750 i2c0
#define I2C_SDA_BH1750 0
#define I2C_SCL_BH1750 1

// --- Estados da Aplicação ---
typedef enum
{
    STATE_CALIBRATE_WHITE,
    STATE_CALIBRATE_BLACK,
    STATE_RUNNING
} AppState;

AppState current_state = STATE_CALIBRATE_WHITE;
int display_mode = 0; // 0: Cor, 1: Luz

// --- Protótipos das Funções de Desenho ---
void draw_cal_screen(ssd1306_t *ssd, const char *line1, const char *line2);
void draw_color_screen(ssd1306_t *ssd, uint8_t r, uint8_t g, uint8_t b);
void draw_light_screen(ssd1306_t *ssd, uint16_t lux);

int main()
{
    stdio_init_all();
    sleep_ms(2000);

    // Inicializa periféricos
    buttons_init();
    led_init();
    gy33_init();

    // I2C para BH1750
    i2c_init(I2C_PORT_BH1750, 100 * 1000);
    gpio_set_function(I2C_SDA_BH1750, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_BH1750, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_BH1750);
    gpio_pull_up(I2C_SCL_BH1750);
    bh1750_power_on(I2C_PORT_BH1750);

    // I2C e Display SSD1306
    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISP);
    gpio_pull_up(I2C_SCL_DISP);
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, DISPLAY_ADDRESS, I2C_PORT_DISP);
    ssd1306_config(&ssd);

    while (1)
    {
        switch (current_state)
        {
        case STATE_CALIBRATE_WHITE:
        {
            draw_cal_screen(&ssd, "Calibrar BRANCO", "Aperte A");
            if (gpio_get(BUTTON_A_PIN) == 0)
            { // Botão pressionado (pull-up)
                gy33_calibrate_white();
                current_state = STATE_CALIBRATE_BLACK;
                sleep_ms(500); // Debounce
            }
            break;
        }
        case STATE_CALIBRATE_BLACK:
        {
            draw_cal_screen(&ssd, "Calibrar PRETO", "Aperte A");
            if (gpio_get(BUTTON_A_PIN) == 0)
            {
                gy33_calibrate_black();
                current_state = STATE_RUNNING;
                sleep_ms(500); // Debounce
            }
            break;
        }
        case STATE_RUNNING:
        {
            if (gpio_get(BUTTON_C_PIN) == 0)
            {
                display_mode = !display_mode;
                sleep_ms(250); // Debounce
            }

            if (display_mode == 0)
            { // Modo Cor
                uint8_t r_cal, g_cal, b_cal;
                gy33_get_calibrated_rgb(&r_cal, &g_cal, &b_cal);
                draw_color_screen(&ssd, r_cal, g_cal, b_cal);
                acender_led_rgb(r_cal, g_cal, b_cal);
            }
            else
            { // Modo Luz
                uint16_t lux = bh1750_read_measurement(I2C_PORT_BH1750);
                draw_light_screen(&ssd, lux);
                turn_off_leds();
            }
            break;
        }
        }
        // BOOTSEL pode ser checado a qualquer momento
        if (gpio_get(BUTTON_B_PIN) == 0)
        {
            reset_usb_boot(0, 0);
        }
        sleep_ms(100);
    }
}

void draw_cal_screen(ssd1306_t *ssd, const char *line1, const char *line2)
{
    ssd1306_fill(ssd, false);
    ssd1306_draw_string(ssd, "-- CALIBRACAO --", 2, 6);
    ssd1306_draw_string(ssd, line1, 8, 25);
    ssd1306_draw_string(ssd, line2, 25, 45);
    ssd1306_send_data(ssd);
}

void draw_color_screen(ssd1306_t *ssd, uint8_t r, uint8_t g, uint8_t b)
{
    char str_r[10], str_g[10], str_b[10];
    sprintf(str_r, "R: %d", r);
    sprintf(str_g, "G: %d", g);
    sprintf(str_b, "B: %d", b);

    ssd1306_fill(ssd, false);
    ssd1306_draw_string(ssd, "Cor Calibrada", 8, 6);
    ssd1306_draw_string(ssd, str_r, 10, 25);
    ssd1306_draw_string(ssd, str_g, 10, 38);
    ssd1306_draw_string(ssd, str_b, 10, 51);
    ssd1306_send_data(ssd);
}

void draw_light_screen(ssd1306_t *ssd, uint16_t lux)
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
