#include "gy33.h"
#include "hardware/i2c.h"
#include <stdio.h>

// Definições do sensor GY-33
#define GY33_I2C_ADDR 0x29
#define I2C_PORT i2c0
#define SDA_PIN 0
#define SCL_PIN 1

// Registros do sensor
#define ENABLE_REG 0x80
#define ATIME_REG 0x81
#define CONTROL_REG 0x8F
#define ID_REG 0x92
#define STATUS_REG 0x93
#define CDATA_REG 0x94
#define RDATA_REG 0x96
#define GDATA_REG 0x98
#define BDATA_REG 0x9A

// --- Calibração e Correção de Cor ---

// Variáveis estáticas para armazenar as referências de calibração P/B
static uint16_t white_ref[3]; // R, G, B
static uint16_t black_ref[3]; // R, G, B

// Matriz de Correção de Cor (CCM) calculada com os dados fornecidos
static const float ccm[3][3] = {
    {1.81f, -0.10f, -0.48f},
    {-0.46f, 6.32f, -2.42f},
    {-0.28f, -2.70f, 4.11f}};

// Funções internas (estáticas)
static void gy33_write_register(uint8_t reg, uint8_t value);
static uint16_t gy33_read_register(uint8_t reg);
static void gy33_read_raw_rgb(uint16_t *r, uint16_t *g, uint16_t *b);
static void gy33_get_bw_calibrated_rgb(uint8_t *r, uint8_t *g, uint8_t *b);

// Implementação das funções públicas
void gy33_init()
{
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    printf("Iniciando GY-33...\n");
    gy33_write_register(ENABLE_REG, 0x03);
    gy33_write_register(ATIME_REG, 0xF5);
    gy33_write_register(CONTROL_REG, 0x00);
}

void gy33_calibrate_white()
{
    gy33_read_raw_rgb(&white_ref[0], &white_ref[1], &white_ref[2]);
    printf("Referência BRANCO salva: R=%d, G=%d, B=%d\n", white_ref[0], white_ref[1], white_ref[2]);
}

void gy33_calibrate_black()
{
    gy33_read_raw_rgb(&black_ref[0], &black_ref[1], &black_ref[2]);
    printf("Referência PRETO salva: R=%d, G=%d, B=%d\n", black_ref[0], black_ref[1], black_ref[2]);
}

void gy33_get_final_rgb(uint8_t *r_final, uint8_t *g_final, uint8_t *b_final)
{
    uint8_t r_bw, g_bw, b_bw;
    // 1. Obter cor calibrada por P/B
    gy33_get_bw_calibrated_rgb(&r_bw, &g_bw, &b_bw);

    // 2. Aplicar a Matriz de Correção de Cor
    float r_corrected = ccm[0][0] * r_bw + ccm[0][1] * g_bw + ccm[0][2] * b_bw;
    float g_corrected = ccm[1][0] * r_bw + ccm[1][1] * g_bw + ccm[1][2] * b_bw;
    float b_corrected = ccm[2][0] * r_bw + ccm[2][1] * g_bw + ccm[2][2] * b_bw;

    // 3. Limitar os valores ao intervalo 0-255
    if (r_corrected > 255.0f)
        r_corrected = 255.0f;
    else if (r_corrected < 0)
        r_corrected = 0;
    if (g_corrected > 255.0f)
        g_corrected = 255.0f;
    else if (g_corrected < 0)
        g_corrected = 0;
    if (b_corrected > 255.0f)
        b_corrected = 255.0f;
    else if (b_corrected < 0)
        b_corrected = 0;

    *r_final = (uint8_t)r_corrected;
    *g_final = (uint8_t)g_corrected;
    *b_final = (uint8_t)b_corrected;
}

// Implementação das funções internas
static void gy33_get_bw_calibrated_rgb(uint8_t *r_cal, uint8_t *g_cal, uint8_t *b_cal)
{
    uint16_t r_raw, g_raw, b_raw;
    gy33_read_raw_rgb(&r_raw, &g_raw, &b_raw);

    uint16_t raw_values[] = {r_raw, g_raw, b_raw};
    uint8_t *cal_values[] = {r_cal, g_cal, b_cal};

    for (int i = 0; i < 3; i++)
    {
        float calibrated_float;
        float range = (float)white_ref[i] - (float)black_ref[i];
        if (range <= 0)
        {
            calibrated_float = 0;
        }
        else
        {
            calibrated_float = ((float)raw_values[i] - (float)black_ref[i]) / range * 255.0f;
        }
        if (calibrated_float > 255.0f)
            calibrated_float = 255.0f;
        else if (calibrated_float < 0)
            calibrated_float = 0;
        *cal_values[i] = (uint8_t)calibrated_float;
    }
}

static void gy33_write_register(uint8_t reg, uint8_t value)
{
    uint8_t buffer[2] = {reg, value};
    i2c_write_blocking(I2C_PORT, GY33_I2C_ADDR, buffer, 2, false);
}

static uint16_t gy33_read_register(uint8_t reg)
{
    uint8_t buffer[2];
    i2c_write_blocking(I2C_PORT, GY33_I2C_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, GY33_I2C_ADDR, buffer, 2, false);
    return (buffer[1] << 8) | buffer[0];
}

static void gy33_read_raw_rgb(uint16_t *r, uint16_t *g, uint16_t *b)
{
    *r = gy33_read_register(RDATA_REG);
    *g = gy33_read_register(GDATA_REG);
    *b = gy33_read_register(BDATA_REG);
}
