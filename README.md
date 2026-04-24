# STM32F446RE + NRF24L01 + 16x2 LCD (Receiver)

This project implements a wireless receiver using STM32F446RE.
It receives temperature and pressure data from NRF24L01 and displays it on a 16x2 LCD using I2C.

## Hardware Used

* STM32F446RE
* NRF24L01
* 16x2 LCD (I2C interface)

## Features

* Receives wireless data using NRF24L01
* Uses interrupt-based data reception
* Extracts struct data (temperature & pressure)
* Displays values on 16x2 LCD
* Bare-metal implementation

## Data Format

```
typedef struct {
    uint32_t temp;
    uint32_t press;
} Data;
```

* Must match transmitter exactly
* Payload size = 8 bytes

## Working

1. NRF24L01 receives wireless data
2. Interrupt (IRQ) triggers on data arrival
3. STM32 reads payload via SPI
4. Data is decoded into struct
5. Temperature and pressure are displayed on LCD

## Pin Connections

### NRF24L01 (SPI1)

| NRF  | STM32 |
| ---- | ----- |
| VCC  | 3.3V  |
| GND  | GND   |
| CE   | PA8   |
| CSN  | PA4   |
| SCK  | PA5   |
| MOSI | PA7   |
| MISO | PA6   |
| IRQ  | PA0   |

### LCD (I2C)

| LCD | STM32 |
| --- | ----- |
| SDA | PB7   |
| SCL | PB6   |

## Notes

* RX payload size must match TX (8 bytes)
* Interrupt flag must be cleared after reading data
* Do not flush RX unnecessarily (can lose data)

## Future Improvements

* Add acknowledgment (ACK payload)
* Display additional sensor data
* Logging and data storage
