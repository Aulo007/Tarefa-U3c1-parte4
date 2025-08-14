#ifndef GY33_H
#define GY33_H

#include "pico/stdlib.h"

/**
 * @brief Initializes the I2C communication and the GY-33 sensor.
 * 
 */
void gy33_init(void);

/**
 * @brief Reads the color values from the GY-33 sensor.
 * 
 * @param r Pointer to store the red value.
 * @param g Pointer to store the green value.
 * @param b Pointer to store the blue value.
 * @param c Pointer to store the clear value.
 */
void gy33_read_color(uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c);

#endif // GY33_H
