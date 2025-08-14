#include "gy33.h"
#include "hardware/i2c.h"
#include <stdio.h>

// Definições do sensor GY-33
#define GY33_I2C_ADDR 0x29 // Endereço do GY-33 no barramento I2C
#define I2C_PORT i2c0
#define SDA_PIN 0
#define SCL_PIN 1

// Registros do sensor
#define ENABLE_REG 0x80
#define ATIME_REG 0x81
#define CONTROL_REG 0x8F
#define ID_REG 0x92
#define STATUS_REG 0x93
#define CDATA_REG 0x94 //  "Clear"
#define RDATA_REG 0x96 //  "Red"
#define GDATA_REG 0x98 //  "Green"
#define BDATA_REG 0x9A //  "Blue"

// Função para escrever um valor em um registro do GY-33
static void gy33_write_register(uint8_t reg, uint8_t value)
{
    uint8_t buffer[2] = {reg, value};
    i2c_write_blocking(I2C_PORT, GY33_I2C_ADDR, buffer, 2, false);
}

// Função para ler um valor de um registro do GY-33
static uint16_t gy33_read_register(uint8_t reg)
{
    uint8_t buffer[2];
    i2c_write_blocking(I2C_PORT, GY33_I2C_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, GY33_I2C_ADDR, buffer, 2, false);
    return (buffer[1] << 8) | buffer[0]; // Combina os bytes em um valor de 16 bits
}

// Inicializa o GY-33
void gy33_init()
{
    // Inicializa o I2C para o sensor de cor
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    
    printf("Iniciando GY-33...\n");

    gy33_write_register(ENABLE_REG, 0x03);  // Ativa o sensor (Power ON e Ativação do ADC)
    gy33_write_register(ATIME_REG, 0xF5);   // Tempo de integração (ajusta a sensibilidade) D5 => 103ms
    gy33_write_register(CONTROL_REG, 0x00); // Configuração de ganho padrão (1x) (pode ir até 60x)
}

// Lê os valores RGB e Clear do GY-33
void gy33_read_color(uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c)
{
    *c = gy33_read_register(CDATA_REG);
    *r = gy33_read_register(RDATA_REG);
    *g = gy33_read_register(GDATA_REG);
    *b = gy33_read_register(BDATA_REG);
}
