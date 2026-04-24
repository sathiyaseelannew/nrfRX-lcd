#ifndef LCD_H
#define LCD_H

#include "stm32f4xx.h"
#include <stdint.h>

void I2C1_Init(void);
void LCD_Init(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Print(char *str);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);
void SysTick_Init(void);
#endif
