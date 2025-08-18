/**
 * @file matrizRGB.c
 * @brief Implementação das funções para controle de matriz 5x5 de LEDs RGB
 *
 * Esta implementação utiliza o hardware PIO do Raspberry Pi Pico para
 * controlar uma matriz de LEDs WS2812B (NeoPixels).
 *
 * @author [Aulo Cezar]
 * @date [01/05/2025]
 * @modified Versão com correção de cor aprimorada
 */

#include "matrizRGB.h"
#include "hardware/pio.h"
#include <math.h>
#include "hardware/clocks.h"
#include "ws2818b.pio.h" 

/* Estado global da matriz de LEDs */
npLED_t leds[NP_LED_COUNT];

/* Cores predefinidas acessíveis externamente */
const npColor_t npColors[] = {
    COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_WHITE, COLOR_BLACK,
    COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA, COLOR_PURPLE, COLOR_ORANGE,
    COLOR_BROWN, COLOR_VIOLET, COLOR_GREY, COLOR_GOLD, COLOR_SILVER};

/* Estado interno do hardware PIO */
static PIO np_pio = NULL; // Instância PIO utilizada
static uint sm = 0;       // State Machine utilizada

/* Configurações de correção de cor */
typedef struct
{
    float gamma;                    // Correção gamma (default: 2.2)
    uint8_t noise_threshold;        // Limite para considerar como ruído (default: 15)
    float color_dominance_ratio;    // Razão mínima para considerar cor dominante (default: 8.0)
    bool enable_gamma_correction;   // Habilita correção gamma
    bool enable_noise_filter;       // Habilita filtro de ruído
    bool enable_color_purification; // Habilita purificação de cor
} ColorCorrectionConfig;

/* Configuração padrão de correção de cor */
static ColorCorrectionConfig color_config = {
    .gamma = 2.2f,
    .noise_threshold = 15,
    .color_dominance_ratio = 8.0f,
    .enable_gamma_correction = true,
    .enable_noise_filter = true,
    .enable_color_purification = true};

/**
 * @brief Tabela de correção gamma pré-calculada para melhor performance
 */
static uint8_t gamma_table[256];
static bool gamma_table_initialized = false;

/**
 * @brief Inicializa a tabela de correção gamma
 */
static void initGammaTable(void)
{
    if (!gamma_table_initialized)
    {
        for (int i = 0; i < 256; i++)
        {
            float normalized = i / 255.0f;
            float corrected = powf(normalized, color_config.gamma);
            gamma_table[i] = (uint8_t)(corrected * 255.0f + 0.5f);
        }
        gamma_table_initialized = true;
    }
}

/**
 * @brief Aplica correção gamma a um valor de cor
 */
static inline uint8_t applyGamma(uint8_t value)
{
    if (color_config.enable_gamma_correction && gamma_table_initialized)
    {
        return gamma_table[value];
    }
    return value;
}

/**
 * @brief Converte coordenadas (x,y) para índice no array linear de LEDs
 *
 * A função leva em conta o padrão de "zig-zag" típico de matrizes WS2812B,
 * onde linhas alternadas têm sentidos opostos.
 *
 * @param x Coordenada horizontal (0-4)
 * @param y Coordenada vertical (0-4)
 * @return Índice correspondente no array linear
 */
static int getIndex(int x, int y)
{
    // Calculamos o índice considerando que a matriz é conectada em zigzag
    // As linhas pares vão da esquerda para a direita, as ímpares da direita para a esquerda
    if (y % 2 == 0)
        return (NP_LED_COUNT - 1) - (y * NP_MATRIX_WIDTH + x);
    return (NP_LED_COUNT - 1) - (y * NP_MATRIX_WIDTH + (NP_MATRIX_WIDTH - 1 - x));
}

/**
 * @brief Limita um valor float entre 0.0 e 1.0
 *
 * @param value Valor a ser limitado
 * @return Valor limitado entre 0.0 e 1.0
 */
static inline float clampIntensity(float value)
{
    return (value < 0.0f) ? 0.0f : (value > 1.0f) ? 1.0f
                                                  : value;
}

/**
 * @brief Aplica filtro de ruído e purificação de cor
 *
 * Esta função implementa três estratégias:
 * 1. Filtro de ruído: Remove valores muito baixos que causam contaminação de cor
 * 2. Purificação de cor dominante: Se uma cor é muito dominante, zera as outras
 * 3. Correção gamma: Aplica correção não-linear para melhor percepção visual
 */
static npColor_t processColor(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t r_final = r;
    uint8_t g_final = g;
    uint8_t b_final = b;

    // Etapa 1: Filtro de ruído básico
    if (color_config.enable_noise_filter)
    {
        // Remove valores muito baixos que causam contaminação
        if (r_final < color_config.noise_threshold && r_final > 0)
        {
            r_final = 0;
        }
        if (g_final < color_config.noise_threshold && g_final > 0)
        {
            g_final = 0;
        }
        if (b_final < color_config.noise_threshold && b_final > 0)
        {
            b_final = 0;
        }
    }

    // Etapa 2: Purificação de cor dominante
    if (color_config.enable_color_purification)
    {
        // Encontra o canal dominante
        uint8_t max_val = r_final;
        if (g_final > max_val)
            max_val = g_final;
        if (b_final > max_val)
            b_final;

        uint8_t min_val = r_final;
        if (g_final < min_val)
            min_val = g_final;
        if (b_final < min_val)
            min_val = b_final;

        // Se existe uma cor claramente dominante
        if (max_val > 100 && min_val > 0)
        {
            float ratio = (float)max_val / (float)min_val;

            // Se a razão for muito alta, provavelmente queremos uma cor pura
            if (ratio > color_config.color_dominance_ratio)
            {
                // Identifica qual é a cor dominante e zera as outras
                if (r_final == max_val)
                {
                    // Vermelho dominante
                    if (g_final < max_val / 4)
                        g_final = 0;
                    if (b_final < max_val / 4)
                        b_final = 0;
                }
                else if (g_final == max_val)
                {
                    // Verde dominante
                    if (r_final < max_val / 4)
                        r_final = 0;
                    if (b_final < max_val / 4)
                        b_final = 0;
                }
                else if (b_final == max_val)
                {
                    // Azul dominante
                    if (r_final < max_val / 4)
                        r_final = 0;
                    if (g_final < max_val / 4)
                        g_final = 0;
                }
            }
        }

        // Caso especial: cores muito próximas do branco puro
        if (r_final > 240 && g_final > 240 && b_final > 240)
        {
            // Mantém branco puro
            r_final = 255;
            g_final = 255;
            b_final = 255;
        }
    }

    // Etapa 3: Aplicar correção gamma (melhora a linearidade perceptual)
    r_final = applyGamma(r_final);
    g_final = applyGamma(g_final);
    b_final = applyGamma(b_final);

    npColor_t result = {r_final, g_final, b_final};
    return result;
}

/**
 * @brief Configura os parâmetros de correção de cor
 */
void npSetColorCorrectionConfig(float gamma, uint8_t noise_threshold,
                                float color_dominance_ratio,
                                bool enable_gamma, bool enable_noise,
                                bool enable_purification)
{
    color_config.gamma = gamma;
    color_config.noise_threshold = noise_threshold;
    color_config.color_dominance_ratio = color_dominance_ratio;
    color_config.enable_gamma_correction = enable_gamma;
    color_config.enable_noise_filter = enable_noise;
    color_config.enable_color_purification = enable_purification;

    // Recalcula a tabela gamma se necessário
    if (enable_gamma)
    {
        gamma_table_initialized = false;
        initGammaTable();
    }
}

/**
 * @brief Função auxiliar para configuração rápida de correção
 */
void npSetColorCorrectionMode(int mode)
{
    switch (mode)
    {
    case 0: // Desabilitado
        npSetColorCorrectionConfig(1.0f, 0, 1.0f, false, false, false);
        break;
    case 1: // Suave
        npSetColorCorrectionConfig(2.2f, 10, 6.0f, true, true, false);
        break;
    case 2: // Normal (recomendado)
        npSetColorCorrectionConfig(2.2f, 15, 8.0f, true, true, true);
        break;
    case 3: // Agressivo
        npSetColorCorrectionConfig(2.5f, 25, 10.0f, true, true, true);
        break;
    case 4: // Muito agressivo (cores puras apenas)
        npSetColorCorrectionConfig(2.8f, 40, 15.0f, true, true, true);
        break;
    }
}

void npInit(uint8_t pin)
{
    // Inicializa a tabela gamma
    initGammaTable();

    // Adiciona o programa PIO à memória do PIO
    uint offset = pio_add_program(pio0, &ws2818b_program);

    // Tenta usar o PIO0 primeiro, se não estiver disponível usa o PIO1
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, false);

    if (sm == -1) // Se não conseguiu uma state machine no PIO0
    {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true); // Exige que haja uma SM disponível
    }

    // Inicializa o programa PIO com a frequência de 800kHz (padrão WS2812B)
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.0f);

    // Limpa a matriz, iniciando com todos os LEDs apagados
    npClear();
}

void npWrite(void)
{
    // Envia os dados de cada LED para o hardware PIO na ordem correta (GRB)
    for (uint i = 0; i < NP_LED_COUNT; ++i)
    {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
}

void npClear(void)
{
    // Define todos os LEDs como preto (apagados)
    for (uint i = 0; i < NP_LED_COUNT; ++i)
    {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
    npWrite(); // Atualiza o hardware
}

bool npIsPositionValid(int x, int y)
{
    return (x >= 0 && x < NP_MATRIX_WIDTH && y >= 0 && y < NP_MATRIX_HEIGHT);
}

void npSetLED(int x, int y, npColor_t color)
{
    if (npIsPositionValid(x, y))
    {
        uint index = getIndex(x, y);
        // Aplica processamento de cor
        npColor_t processed = processColor(color.r, color.g, color.b);
        leds[index].R = processed.r;
        leds[index].G = processed.g;
        leds[index].B = processed.b;
    }
}

void npSetLEDRaw(int x, int y, npColor_t color)
{
    if (npIsPositionValid(x, y))
    {
        uint index = getIndex(x, y);
        // Não aplica processamento - usa valores brutos
        leds[index].R = color.r;
        leds[index].G = color.g;
        leds[index].B = color.b;
    }
}

void npSetLEDIntensity(int x, int y, npColor_t color, float intensity)
{
    if (npIsPositionValid(x, y))
    {
        intensity = clampIntensity(intensity);
        uint index = getIndex(x, y);

        // Aplica intensidade primeiro, depois processa a cor
        uint8_t r = (uint8_t)(color.r * intensity);
        uint8_t g = (uint8_t)(color.g * intensity);
        uint8_t b = (uint8_t)(color.b * intensity);

        npColor_t processed = processColor(r, g, b);
        leds[index].R = processed.r;
        leds[index].G = processed.g;
        leds[index].B = processed.b;
    }
}

void npSetRow(int row, npColor_t color)
{
    if (row >= 0 && row < NP_MATRIX_HEIGHT)
    {
        // Processa a cor uma vez
        npColor_t processed = processColor(color.r, color.g, color.b);

        for (int x = 0; x < NP_MATRIX_WIDTH; x++)
        {
            uint index = getIndex(x, row);
            leds[index].R = processed.r;
            leds[index].G = processed.g;
            leds[index].B = processed.b;
        }
        npWrite(); // Atualiza o hardware
    }
}

void npSetRowIntensity(int row, npColor_t color, float intensity)
{
    if (row >= 0 && row < NP_MATRIX_HEIGHT)
    {
        intensity = clampIntensity(intensity);

        // Aplica intensidade e processa a cor
        uint8_t r = (uint8_t)(color.r * intensity);
        uint8_t g = (uint8_t)(color.g * intensity);
        uint8_t b = (uint8_t)(color.b * intensity);

        npColor_t processed = processColor(r, g, b);

        for (int x = 0; x < NP_MATRIX_WIDTH; x++)
        {
            uint index = getIndex(x, row);
            leds[index].R = processed.r;
            leds[index].G = processed.g;
            leds[index].B = processed.b;
        }
        npWrite(); // Atualiza o hardware
    }
}

void npSetColumn(int col, npColor_t color)
{
    if (col >= 0 && col < NP_MATRIX_WIDTH)
    {
        // Processa a cor uma vez
        npColor_t processed = processColor(color.r, color.g, color.b);

        for (int y = 0; y < NP_MATRIX_HEIGHT; y++)
        {
            uint index = getIndex(col, y);
            leds[index].R = processed.r;
            leds[index].G = processed.g;
            leds[index].B = processed.b;
        }
        npWrite(); // Atualiza o hardware
    }
}

void npSetColumnIntensity(int col, npColor_t color, float intensity)
{
    if (col >= 0 && col < NP_MATRIX_WIDTH)
    {
        intensity = clampIntensity(intensity);

        // Aplica intensidade e processa a cor
        uint8_t r = (uint8_t)(color.r * intensity);
        uint8_t g = (uint8_t)(color.g * intensity);
        uint8_t b = (uint8_t)(color.b * intensity);

        npColor_t processed = processColor(r, g, b);

        for (int y = 0; y < NP_MATRIX_HEIGHT; y++)
        {
            uint index = getIndex(col, y);
            leds[index].R = processed.r;
            leds[index].G = processed.g;
            leds[index].B = processed.b;
        }
        npWrite(); // Atualiza o hardware
    }
}

void npSetBorder(npColor_t color)
{
    // Processa a cor uma vez
    npColor_t processed = processColor(color.r, color.g, color.b);

    // Define as linhas superiores e inferiores
    for (int x = 0; x < NP_MATRIX_WIDTH; x++)
    {
        uint index_top = getIndex(x, 0);
        uint index_bottom = getIndex(x, NP_MATRIX_HEIGHT - 1);

        leds[index_top].R = processed.r;
        leds[index_top].G = processed.g;
        leds[index_top].B = processed.b;

        leds[index_bottom].R = processed.r;
        leds[index_bottom].G = processed.g;
        leds[index_bottom].B = processed.b;
    }

    // Define as colunas laterais (excluindo os cantos já definidos)
    for (int y = 1; y < NP_MATRIX_HEIGHT - 1; y++)
    {
        uint index_left = getIndex(0, y);
        uint index_right = getIndex(NP_MATRIX_WIDTH - 1, y);

        leds[index_left].R = processed.r;
        leds[index_left].G = processed.g;
        leds[index_left].B = processed.b;

        leds[index_right].R = processed.r;
        leds[index_right].G = processed.g;
        leds[index_right].B = processed.b;
    }

    npWrite(); // Atualiza o hardware
}

void npSetDiagonal(bool mainDiagonal, npColor_t color)
{
    // Processa a cor uma vez
    npColor_t processed = processColor(color.r, color.g, color.b);

    for (int i = 0; i < NP_MATRIX_WIDTH; i++)
    {
        uint index;
        if (mainDiagonal)
        {
            index = getIndex(i, i); // Diagonal principal
        }
        else
        {
            index = getIndex(NP_MATRIX_WIDTH - 1 - i, i); // Diagonal secundária
        }

        leds[index].R = processed.r;
        leds[index].G = processed.g;
        leds[index].B = processed.b;
    }
    npWrite(); // Atualiza o hardware
}

void npFill(npColor_t color)
{
    // Processa a cor uma vez
    npColor_t processed = processColor(color.r, color.g, color.b);

    for (int i = 0; i < NP_LED_COUNT; i++)
    {
        leds[i].R = processed.r;
        leds[i].G = processed.g;
        leds[i].B = processed.b;
    }
    npWrite(); // Atualiza o hardware
}

void npFillRGB(uint8_t r, uint8_t g, uint8_t b)
{
    // Usa o novo sistema de processamento de cor
    npColor_t processed = processColor(r, g, b);

    for (int i = 0; i < NP_LED_COUNT; i++)
    {
        leds[i].R = processed.r;
        leds[i].G = processed.g;
        leds[i].B = processed.b;
    }
    npWrite(); // Atualiza o hardware
}

void npFillRGBRaw(uint8_t r, uint8_t g, uint8_t b)
{
    // Versão sem processamento para quando você quer controle total
    for (int i = 0; i < NP_LED_COUNT; i++)
    {
        leds[i].R = r;
        leds[i].G = g;
        leds[i].B = b;
    }
    npWrite(); // Atualiza o hardware
}

void npFillIntensity(npColor_t color, float intensity)
{
    intensity = clampIntensity(intensity);

    // Aplica intensidade e processa a cor
    uint8_t r = (uint8_t)(color.r * intensity);
    uint8_t g = (uint8_t)(color.g * intensity);
    uint8_t b = (uint8_t)(color.b * intensity);

    npColor_t processed = processColor(r, g, b);

    for (int i = 0; i < NP_LED_COUNT; i++)
    {
        leds[i].R = processed.r;
        leds[i].G = processed.g;
        leds[i].B = processed.b;
    }
    npWrite(); // Atualiza o hardware
}

void npSetMatrixWithIntensity(int matriz[NP_MATRIX_HEIGHT][NP_MATRIX_WIDTH][3], float intensity)
{
    intensity = clampIntensity(intensity);

    // Loop para configurar os LEDs
    for (uint8_t linha = 0; linha < 5; linha++)
    {
        for (uint8_t coluna = 0; coluna < 5; coluna++)
        {
            // Calcula os valores RGB ajustados pela intensidade
            uint8_t r = (uint8_t)(float)(matriz[linha][coluna][0] * intensity);
            uint8_t g = (uint8_t)(float)(matriz[linha][coluna][1] * intensity);
            uint8_t b = (uint8_t)(float)(matriz[linha][coluna][2] * intensity);

            // Processa a cor
            npColor_t processed = processColor(r, g, b);

            uint index = getIndex(coluna, linha);

            // Configura o LED diretamente
            leds[index].R = processed.r;
            leds[index].G = processed.g;
            leds[index].B = processed.b;
        }
    }
    npWrite(); // Atualiza o hardware
}

void npAnimateFrames(int period, int num_frames,
                     int desenho[num_frames][NP_MATRIX_HEIGHT][NP_MATRIX_WIDTH][3],
                     float intensity)
{
    intensity = clampIntensity(intensity);

    for (int i = 0; i < num_frames; i++)
    {
        npSetMatrixWithIntensity(desenho[i], intensity);
        sleep_ms(period); // Aguarda o período definido entre frames
    }
}