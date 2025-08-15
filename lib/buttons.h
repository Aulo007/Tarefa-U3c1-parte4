#ifndef BUTTONS_H
#define BUTTONS_H

#include "pico/stdlib.h"

// Tipo de função para callbacks de botão
typedef void (*button_callback_t)(uint gpio, uint32_t events);

// Definições dos pinos dos botões
#define BUTTON_A_PIN 5
#define BUTTON_B_PIN 6
#define BUTTON_C_PIN 22 // Pino do joystick

// Função para inicializar todos os botões e configurar interrupções
void buttons_init(button_callback_t callback);

#endif // BUTTONS_H