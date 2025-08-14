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
 * @brief Reads the raw sensor data and calculates the calibrated RGB values based on the stored white and black references.
 *
 * @param r Pointer to store the calibrated red value (0-255).
 * @param g Pointer to store the calibrated green value (0-255).
 * @param b Pointer to store the calibrated blue value (0-255).
 */
void gy33_get_calibrated_rgb(uint8_t *r, uint8_t *g, uint8_t *b);

#endif // GY33_H