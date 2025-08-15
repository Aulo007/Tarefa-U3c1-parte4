#include "buttons.h"
#include "hardware/gpio.h"

static button_callback_t general_callback = NULL;
static const uint32_t debounce_ms = 200;
static uint32_t last_press_time = 0;

// Função de callback que o SDK do Pico chamará
void gpio_callback_handler(uint gpio, uint32_t events) {
    // Verifica o debounce
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if ((now - last_press_time) < debounce_ms) {
        return; // Ignora o pressionamento se for muito rápido
    }
    last_press_time = now;

    // Chama o callback geral registrado no main
    if (general_callback) {
        general_callback(gpio, events);
    }
}

// Função para inicializar um único botão com interrupção
static void button_init_single(uint gpio_pin) {
    gpio_init(gpio_pin);
    gpio_set_dir(gpio_pin, GPIO_IN);
    gpio_pull_up(gpio_pin); // Ativa o pull-up interno
    // Configura a interrupção para borda de descida (pressionar o botão)
    gpio_set_irq_enabled(gpio_pin, GPIO_IRQ_EDGE_FALL, true);
}

// Função para inicializar todos os botões e registrar o callback
void buttons_init(button_callback_t callback) {
    general_callback = callback;

    button_init_single(BUTTON_A_PIN);
    button_init_single(BUTTON_B_PIN);
    button_init_single(BUTTON_C_PIN);

    // Registra a função de tratamento de interrupção para todos os pinos GPIO
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback_handler);
    // Para os outros botões, apenas habilitamos a IRQ, pois o handler já está registrado
    gpio_set_irq_enabled(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BUTTON_C_PIN, GPIO_IRQ_EDGE_FALL, true);
}