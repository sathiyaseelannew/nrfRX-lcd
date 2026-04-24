/*
 * LCD.c
 *
 *  Created on: 21-Apr-2026
 *      Author: sathi
 */
#include"lcd.h"
void SysTick_Init(void)
{
    SysTick->LOAD = 16000 - 1; // 1ms @16MHz
    SysTick->VAL  = 0;
    SysTick->CTRL = (1<<0) | (1<<2);
}

void delay_ms(uint32_t ms)
{
    for(uint32_t i=0;i<ms;i++)
    {
        SysTick->VAL = 0;
        while(!(SysTick->CTRL & (1<<16)));
    }
}

void delay_us(uint32_t us)
{
    uint32_t temp = SysTick->LOAD;

    SysTick->LOAD = 16 - 1;

    for(uint32_t i=0;i<us;i++)
    {
        SysTick->VAL = 0;
        while(!(SysTick->CTRL & (1<<16)));
    }

    SysTick->LOAD = temp;
}


/* ========= I2C INIT (PB8, PB9) ========= */
void I2C1_Init(void)
{
    RCC->AHB1ENR |= (1<<1);
    RCC->APB1ENR |= (1<<21);

    // PB8, PB9 → AF
    GPIOB->MODER &= ~((3<<(8*2)) | (3<<(9*2)));
    GPIOB->MODER |=  ((2<<(8*2)) | (2<<(9*2)));

    GPIOB->OTYPER |= (1<<8) | (1<<9);
    GPIOB->OSPEEDR |= (3<<(8*2)) | (3<<(9*2));

    GPIOB->PUPDR &= ~((3<<(8*2)) | (3<<(9*2)));
    GPIOB->PUPDR |=  ((1<<(8*2)) | (1<<(9*2)));

    GPIOB->AFR[1] &= ~((0xF<<0) | (0xF<<4));
    GPIOB->AFR[1] |=  ((4<<0) | (4<<4));

    I2C1->CR1 = 0;
    I2C1->CR2 = 16;
    I2C1->CCR = 80;
    I2C1->TRISE = 17;

    I2C1->CR1 |= (1<<0);
}

/* ========= I2C WRITE ========= */
static void I2C_Write(uint8_t addr, uint8_t data)
{
    I2C1->CR1 |= (1<<8);
    while(!(I2C1->SR1 & (1<<0)));

    I2C1->DR = addr;
    while(!(I2C1->SR1 & (1<<1)));

    (void)I2C1->SR2;

    I2C1->DR = data;
    while(!(I2C1->SR1 & (1<<7)));

    I2C1->CR1 |= (1<<9);
}

/* ========= LCD CORE ========= */

#define LCD_ADDR 0x4E   // try 0x7E if not working

static void LCD_Write4(uint8_t data)
{
    I2C_Write(LCD_ADDR, data | 0x04);
    delay_ms(1);
    I2C_Write(LCD_ADDR, data & ~0x04);
}

static void LCD_Cmd(uint8_t cmd)
{
    uint8_t high = cmd & 0xF0;
    uint8_t low  = (cmd<<4) & 0xF0;

    LCD_Write4(high | 0x08);
    LCD_Write4(low  | 0x08);
}

static void LCD_Data(uint8_t data)
{
    uint8_t high = data & 0xF0;
    uint8_t low  = (data<<4) & 0xF0;

    LCD_Write4(high | 0x09);
    LCD_Write4(low  | 0x09);
}

/* ========= LCD INIT ========= */
void LCD_Init(void)
{
    delay_ms(50);

    LCD_Write4(0x30);
    delay_ms(5);
    LCD_Write4(0x30);
    delay_ms(1);
    LCD_Write4(0x30);
    LCD_Write4(0x20);

    LCD_Cmd(0x28);
    LCD_Cmd(0x0C);
    LCD_Cmd(0x06);
    LCD_Cmd(0x01);

    delay_ms(5);
}

/* ========= CURSOR ========= */
void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (row==0) ? 0x80 : 0xC0;
    LCD_Cmd(addr + col);
}

/* ========= PRINT ========= */
void LCD_Print(char *str)
{
    while(*str)
        LCD_Data(*str++);
}
