/**
 * @file matrizRGB.h
 * @brief Interface para controle de matriz 5x5 de LEDs RGB (WS2812B)
 *
 * Esta biblioteca fornece funções para controlar uma matriz 5x5 de LEDs RGB WS2812B
 * conectados a um Raspberry Pi Pico. Implementa funções para manipulação de cores,
 * padrões, animações e um sistema avançado de correção de cor.
 *
 * @author [Aulo Cezar]
 * @date [01/05/2025]
 * @version 2.1
 */

#ifndef MATRIZ_RGB_H_
#define MATRIZ_RGB_H_

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

/**
 * @brief Número total de LEDs na matriz 5x5
 */
#define NP_LED_COUNT 25

/**
 * @brief Dimensões da matriz de LEDs
 */
#define NP_MATRIX_WIDTH 5
#define NP_MATRIX_HEIGHT 5

/**
 * @brief Estrutura representando um LED RGB com componentes na ordem GRB
 * (ordem específica requerida pelo protocolo WS2812B)
 */
typedef struct
{
    uint8_t G; /**< Componente verde (0-255) */
    uint8_t R; /**< Componente vermelho (0-255) */
    uint8_t B; /**< Componente azul (0-255) */
} npLED_t;

/**
 * @brief Estrutura representando uma cor RGB em ordem natural
 */
typedef struct
{
    uint8_t r; /**< Componente vermelho (0-255) */
    uint8_t g; /**< Componente verde (0-255) */
    uint8_t b; /**< Componente azul (0-255) */
} npColor_t;

/**
 * @brief Cores predefinidas para uso conveniente
 */
// CORRIGIDO: Valores ajustados de 1 para 255 para intensidade máxima
#define COLOR_BLACK (npColor_t){0, 0, 0}
#define COLOR_RED (npColor_t){255, 0, 0}
#define COLOR_GREEN (npColor_t){0, 255, 0}
#define COLOR_BLUE (npColor_t){0, 0, 255}
#define COLOR_WHITE (npColor_t){255, 255, 255}
#define COLOR_YELLOW (npColor_t){255, 170, 0}
#define COLOR_CYAN (npColor_t){0, 255, 255}
#define COLOR_MAGENTA (npColor_t){255, 0, 255}
#define COLOR_PURPLE (npColor_t){128, 0, 128}
#define COLOR_ORANGE (npColor_t){255, 20, 0}
#define COLOR_BROWN (npColor_t){60, 40, 0}
#define COLOR_VIOLET (npColor_t){175, 0, 168}
#define COLOR_GREY (npColor_t){128, 128, 128}
#define COLOR_GOLD (npColor_t){255, 215, 0}
#define COLOR_SILVER (npColor_t){192, 192, 192}

/** @brief Tabela de cores predefinidas acessível externamente */
extern const npColor_t npColors[];

/** @brief Array contendo o estado atual de todos os LEDs da matriz */
extern npLED_t leds[NP_LED_COUNT];

// --- Funções de Inicialização e Controle Básico ---

/**
 * @brief Inicializa a matriz de LEDs RGB
 *
 * Configura o hardware PIO para comunicação com os LEDs WS2812B e
 * limpa a matriz (todos os LEDs desligados).
 *
 * @param pin Número do pino GPIO conectado ao sinal de dados dos LEDs
 */
void npInit(uint8_t pin);

/**
 * @brief Envia os dados de cores atuais para a matriz de LEDs
 *
 * Transmite o conteúdo atual do buffer de LEDs para o hardware,
 * atualizando visualmente o estado da matriz.
 */
void npWrite(void);

/**
 * @brief Desliga todos os LEDs da matriz (define todos para preto)
 *
 * Esta função também atualiza o hardware automaticamente.
 */
void npClear(void);

// --- Funções de Correção de Cor (ADICIONADAS) ---

/**
 * @brief Configura os parâmetros de correção de cor.
 * @param gamma Valor da correção gamma (ex: 2.2).
 * @param noise_threshold Limite para filtro de ruído (0-255).
 * @param color_dominance_ratio Razão para purificação de cor (ex: 8.0).
 * @param enable_gamma Habilita/desabilita correção gamma.
 * @param enable_noise Habilita/desabilita filtro de ruído.
 * @param enable_purification Habilita/desabilita purificação de cor.
 */
void npSetColorCorrectionConfig(float gamma, uint8_t noise_threshold,
                                float color_dominance_ratio,
                                bool enable_gamma, bool enable_noise,
                                bool enable_purification);

/**
 * @brief Configura um modo predefinido de correção de cor.
 * @param mode 0: Desabilitado, 1: Suave, 2: Normal, 3: Agressivo, 4: Muito Agressivo.
 */
void npSetColorCorrectionMode(int mode);

// --- Funções de Manipulação de LEDs Individuais ---

/**
 * @brief Verifica se uma posição (x,y) é válida na matriz
 *
 * @param x Coordenada horizontal (0-4)
 * @param y Coordenada vertical (0-4)
 * @return true se a posição é válida, false caso contrário
 */
bool npIsPositionValid(int x, int y);

/**
 * @brief Define a cor de um LED específico na matriz, aplicando correção de cor.
 *
 * @param x Coordenada horizontal (0-4)
 * @param y Coordenada vertical (0-4)
 * @param color Cor a ser aplicada ao LED
 */
void npSetLED(int x, int y, npColor_t color);

/**
 * @brief Define a cor de um LED específico SEM aplicar correção de cor.
 *
 * @param x Coordenada horizontal (0-4)
 * @param y Coordenada vertical (0-4)
 * @param color Cor "crua" a ser aplicada ao LED.
 */
void npSetLEDRaw(int x, int y, npColor_t color);

/**
 * @brief Define a cor de um LED específico com intensidade ajustável.
 * A correção de cor é aplicada após o ajuste de intensidade.
 *
 * @param x Coordenada horizontal (0-4)
 * @param y Coordenada vertical (0-4)
 * @param color Cor base a ser aplicada
 * @param intensity Intensidade da cor (0.0 - 1.0)
 */
void npSetLEDIntensity(int x, int y, npColor_t color, float intensity);

// --- Funções de Manipulação de Grupos de LEDs ---

/**
 * @brief Preenche toda uma linha com uma cor específica.
 * @param row Índice da linha (0-4)
 * @param color Cor a ser aplicada a toda a linha
 */
void npSetRow(int row, npColor_t color);

/**
 * @brief Preenche toda uma linha com uma cor e intensidade específicas.
 * @param row Índice da linha (0-4)
 * @param color Cor base a ser aplicada
 * @param intensity Intensidade da cor (0.0 - 1.0)
 */
void npSetRowIntensity(int row, npColor_t color, float intensity);

/**
 * @brief Preenche toda uma coluna com uma cor específica.
 * @param col Índice da coluna (0-4)
 * @param color Cor a ser aplicada a toda a coluna
 */
void npSetColumn(int col, npColor_t color);

/**
 * @brief Preenche toda uma coluna com uma cor e intensidade específicas.
 * @param col Índice da coluna (0-4)
 * @param color Cor base a ser aplicada
 * @param intensity Intensidade da cor (0.0 - 1.0)
 */
void npSetColumnIntensity(int col, npColor_t color, float intensity);

/**
 * @brief Preenche a borda da matriz com uma cor específica.
 * @param color Cor a ser aplicada à borda da matriz
 */
void npSetBorder(npColor_t color);

/**
 * @brief Preenche uma diagonal da matriz com uma cor específica.
 * @param mainDiagonal true para a diagonal principal, false para a secundária
 * @param color Cor a ser aplicada à diagonal
 */
void npSetDiagonal(bool mainDiagonal, npColor_t color);

// --- Funções de Preenchimento Total da Matriz ---

/**
 * @brief Preenche toda a matriz com uma cor específica, aplicando correção.
 * @param color Cor a ser aplicada a todos os LEDs
 */
void npFill(npColor_t color);

/**
 * @brief Preenche toda a matriz com uma cor (RGB), aplicando correção.
 * @param r Componente vermelho (0-255)
 * @param g Componente verde (0-255)
 * @param b Componente azul (0-255)
 */
void npFillRGB(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Preenche toda a matriz com uma cor (RGB) SEM aplicar correção.
 * @param r Componente vermelho (0-255)
 * @param g Componente verde (0-255)
 * @param b Componente azul (0-255)
 */
void npFillRGBRaw(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Preenche toda a matriz com uma cor e intensidade específicas.
 * @param color Cor base a ser aplicada
 * @param intensity Intensidade da cor (0.0 - 1.0)
 */
void npFillIntensity(npColor_t color, float intensity);

// --- Funções de Animação e Desenho ---

/**
 * @brief Define o estado da matriz a partir de uma matriz de cores 5x5.
 * A intensidade é aplicada e a correção de cor é processada para cada LED.
 *
 * @param matriz Matriz 5x5x3 [linha][coluna][cor]
 * @param intensity Intensidade a ser aplicada a todas as cores (0.0 - 1.0)
 */
void npSetMatrixWithIntensity(int matriz[NP_MATRIX_HEIGHT][NP_MATRIX_WIDTH][3], float intensity);

/**
 * @brief Reproduz uma sequência de frames como uma animação.
 *
 * @param period Tempo em milissegundos entre cada frame
 * @param num_frames Número de frames na animação
 * @param desenho Array de matrizes [frame][linha][coluna][cor]
 * @param intensity Intensidade a ser aplicada a todas as cores (0.0 - 1.0)
 */
void npAnimateFrames(int period, int num_frames,
                     int desenho[num_frames][NP_MATRIX_HEIGHT][NP_MATRIX_WIDTH][3],
                     float intensity);

#endif /* MATRIZ_RGB_H_ */