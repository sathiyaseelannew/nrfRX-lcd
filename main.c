#include"lcd.h"
#include"stdio.h"
/* ================= GPIO SAFE MACROS ================= */

#define CE_HIGH()   (GPIOA->BSRR = (1<<8))
#define CE_LOW()    (GPIOA->BSRR = (1<<8)<<16)

#define CSN_HIGH()  (GPIOA->BSRR = (1<<4))
#define CSN_LOW()   (GPIOA->BSRR = (1<<4)<<16)

/* ================= NVIC ================= */

#define NVIC_ISER0 (*(volatile uint32_t*)0xE000E100)

/* ================= NRF COMMANDS ================= */

#define W_REGISTER  0x20
#define R_REGISTER  0x00

/* ================= GLOBAL ================= */

volatile uint8_t read = 0;
uint8_t data[11];




/* ================= SPI + GPIO + EXTI INIT ================= */

void SPI1_Init(void)
{
    RCC->AHB1ENR |= (1<<0);
    RCC->APB2ENR |= (1<<12);

    /* PA5,6,7 AF */
    GPIOA->MODER &= ~(0x3F << (5*2));
    GPIOA->MODER |=  (0x2A << (5*2));

    GPIOA->AFR[0] &= ~((0xF<<(5*4)) | (0xF<<(6*4)) | (0xF<<(7*4)));
    GPIOA->AFR[0] |=  ((5<<(5*4)) | (5<<(6*4)) | (5<<(7*4)));

    GPIOA->OSPEEDR |=(3<<(4*2))| (3<<(5*2)) | (3<<(6*2)) | (3<<(7*2));

    /* PA4, PA8 output */
    GPIOA->MODER &= ~((3<<(4*2)) | (3<<(8*2)));
    GPIOA->MODER |=  ((1<<(4*2)) | (1<<(8*2)));

    CE_LOW();
    CSN_HIGH();

    /* SPI config */
    SPI1->CR1 = 0;
    SPI1->CR1 |= (1<<2);
    SPI1->CR1 |= (1<<9);
    SPI1->CR1 |= (1<<8);
    SPI1->CR1 |= (1<<3);
    SPI1->CR1 |= (1<<6);

    /* EXTI PA0 (NRF IRQ) */
    RCC->APB2ENR |= (1<<14);

    GPIOA->MODER &= ~(3<<(0*2));

    SYSCFG->EXTICR[0] &= ~(0xF<<0);

    EXTI->IMR |= (1<<0);

    EXTI->RTSR &= ~(1<<0); // disable rising
    EXTI->FTSR |= (1<<0);  // falling edge

    NVIC_ISER0 |= (1<<6); // EXTI0 enable
}

/* ================= SPI ================= */

uint8_t SPI_Transfer(uint8_t data)
{
    while(!(SPI1->SR & (1<<1)));
    SPI1->DR = data;
    while(!(SPI1->SR & (1<<0)));
    return SPI1->DR;
}

/* ================= NRF ================= */


uint8_t NRF_Read_Reg(uint8_t reg)
{
    uint8_t val;
    CSN_LOW();
    SPI_Transfer(R_REGISTER | reg);
    val = SPI_Transfer(0xFF);
    CSN_HIGH();
    return val;
}

void NRF_Write_Reg(uint8_t reg, uint8_t val)
{
    CSN_LOW();
    SPI_Transfer(W_REGISTER | reg);
    SPI_Transfer(val);
    CSN_HIGH();
}

void NRF_Write_Buf(uint8_t reg, uint8_t *buf, uint8_t len)
{
    CSN_LOW();
    SPI_Transfer(W_REGISTER | reg);
    for(int i=0;i<len;i++)
        SPI_Transfer(buf[i]);
    CSN_HIGH();
}

void NRF_ReadPayload(uint8_t *buf, uint8_t len)
{
    CSN_LOW();
    SPI_Transfer(0x61); // R_RX_PAYLOAD

    for(int i=0;i<len;i++)
        buf[i] = SPI_Transfer(0xFF);

    CSN_HIGH();
}


/* ================= NRF RX INIT ================= */
void NRF_Flush_TX(void){
	CSN_LOW();
	SPI_Transfer(0xE1);//Flush_TX
	CSN_HIGH();
}
void NRF_Flush_RX(void){
	CSN_LOW();
	SPI_Transfer(0xE2);//Flush_RX
	CSN_HIGH();
}
void NRF_Init_RX(void)
{
    delay_ms(200);

    uint8_t addr[5] = {'N','O','D','E','1'};
    NRF_Write_Reg(0x07,0x70);//clear status flags
    NRF_Flush_TX();
    NRF_Flush_RX();

    NRF_Write_Reg(0x00, 0x0F);

    NRF_Write_Reg(0x01, 0x00);
    NRF_Write_Reg(0x02, 0x01);
    NRF_Write_Reg(0x03, 0x03);
    NRF_Write_Reg(0x05, 76);
    NRF_Write_Reg(0x06, 0x06);

    NRF_Write_Buf(0x0A, addr, 5);

    NRF_Write_Reg(0x11, 8);

    CE_HIGH();
}

/* ================= ISR ================= */

void EXTI0_IRQHandler(void)
{
    if(EXTI->PR & (1<<0))
    {
        read = 1;
        EXTI->PR |= (1<<0);
    }
}

/* ================= MAIN ================= */

typedef struct{
    uint32_t value1;
    uint32_t value2;
} rxdata;

int main(void)
{
    SysTick_Init();
    SCB->CPACR |= (0xF << 20);

    SPI1_Init();
    NRF_Init_RX();

    I2C1_Init();
    LCD_Init();

    char line1[20];
    char line2[20];

    rxdata t;

    while(1)
    {
        if(read)
        {
            read = 0;

            uint8_t status = NRF_Read_Reg(0x07);

            if(status & (1<<6)) // RX_DR
            {
                NRF_ReadPayload((uint8_t*)&t, sizeof(t));

                //  Print values
                sprintf(line1,"Temp: %.2f C",
                        (float)t.value1 / 100.0f);

                sprintf(line2,"Pres:%.1f hPa",
                        (float)t.value2 / 100.0f);

                //  Clear LCD (important)
                             LCD_SetCursor(0,0);
                             LCD_Print("                ");
                             LCD_SetCursor(1,0);
                             LCD_Print("                ");
                LCD_SetCursor(0,0);
                LCD_Print(line1);

                LCD_SetCursor(1,0);
                LCD_Print(line2);

                // Clear RX flag
                NRF_Write_Reg(0x07, (1<<6));
                delay_ms(900);
            }
        }
    }
}
