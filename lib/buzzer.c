#include "buzzer.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// Função auxiliar interna para alterar a frequência do buzzer e retornar o valor de wrap.
static uint16_t buzzer_set_freq(uint pino, uint freq)
{
    uint slice_num = pwm_gpio_to_slice_num(pino);
    // Evita divisão por zero.
    if (freq < 1)
        freq = 1;

    // Calcula o valor de 'wrap' para a frequência desejada.
    // Clock (125MHz) / (divisor * freq) = wrap
    float div = 125.0f;
    uint32_t wrap = (uint32_t)(125000000 / (div * freq));
    if (wrap > 65535)
        wrap = 65535; // Garante que o valor de wrap não exceda o máximo de 16 bits

    pwm_set_clkdiv(slice_num, div);
    pwm_set_wrap(slice_num, wrap);
    return (uint16_t)wrap;
}

void inicializar_buzzer(uint pino)
{
    gpio_set_function(pino, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pino);
    pwm_config config = pwm_get_default_config();
    // A frequência inicial é definida aqui, mas pode ser alterada depois
    float div = (float)clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096);
    pwm_config_set_clkdiv(&config, div);
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pino, 0); // Começa desligado
}

// Esta função usará uma aproximação para o duty cycle.
// Funciona bem para o propósito básico de ligar o som.
void ativar_buzzer(uint pino)
{
    // Define o duty cycle para 50% de um valor de wrap comum (4095).
    // Para os toques com frequência dinâmica, o duty cycle é ajustado precisamente.
    pwm_set_gpio_level(pino, 2048);
}

// Esta função também usará uma aproximação.
void ativar_buzzer_com_intensidade(uint pino, float intensidade)
{
    if (intensidade < 0.0f)
        intensidade = 0.0f;
    if (intensidade > 1.0f)
        intensidade = 1.0f;
    // A intensidade controla o duty cycle (volume percebido)
    pwm_set_gpio_level(pino, (uint16_t)(intensidade * 4095));
}

void desativar_buzzer(uint pino)
{
    pwm_set_gpio_level(pino, 0); // Desliga o buzzer
}

// Alerta de baixa luz: um bipe longo e grave a cada segundo
void toque_1(uint pino)
{
    uint16_t wrap = buzzer_set_freq(pino, 440); // Tom grave (Lá)
    pwm_set_gpio_level(pino, wrap / 2);         // Ativa com 50% de duty cycle
    sleep_ms(500);
    desativar_buzzer(pino);
    sleep_ms(500);
}

// Alerta de cor vermelha: três bipes curtos e agudos
void toque_2(uint pino)
{
    uint16_t wrap = buzzer_set_freq(pino, 880); // Tom agudo (Lá uma oitava acima)
    for (int i = 0; i < 3; i++)
    {
        pwm_set_gpio_level(pino, wrap / 2); // Ativa com 50% de duty cycle
        sleep_ms(100);
        desativar_buzzer(pino);
        sleep_ms(100);
    }
}
