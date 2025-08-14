#ifndef GY33_H
#define GY33_H

#include "pico/stdlib.h"

/**
 * @brief Initializes the I2C communication and the GY-33 sensor.
 */
void gy33_init(void);

/**
 * @brief Reads the current sensor values and stores them as the white reference.
 */
void gy33_calibrate_white(void);

/**
 * @brief Reads the current sensor values and stores them as the black reference.
 */
void gy33_calibrate_black(void);

/**
 * @brief Applies black/white calibration and a color correction matrix to get the final, corrected RGB values.
 *
 * @param r Pointer to store the final red value (0-255).
 * @param g Pointer to store the final green value (0-255).
 * @param b Pointer to store the final blue value (0-255).
 */
void gy33_get_final_rgb(uint8_t *r, uint8_t *g, uint8_t *b);

#endif // GY33_H
