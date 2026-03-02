#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_tim.h"
#include <stdio.h>

#define DHT_PORT GPIOA
#define DHT_PIN  GPIO_Pin_1
#define ADXL345_CS_PORT GPIOA
#define ADXL345_CS_PIN  GPIO_Pin_4
#define BH1750_ADDR 0x46

char buf[128];

void Timer2_Config(void) {
    TIM_TimeBaseInitTypeDef tim;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    tim.TIM_Prescaler = 72 - 1; 
    tim.TIM_Period = 0xFFFF;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &tim);
    TIM_Cmd(TIM2, ENABLE);
}

void delay_us(uint16_t us) {
    TIM_SetCounter(TIM2, 0);
    while (TIM_GetCounter(TIM2) < us);
}

void delay_ms(uint32_t ms) {
    while (ms--) delay_us(1000);
}

void Peripherals_Init(void) {
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    SPI_InitTypeDef spi;
    I2C_InitTypeDef i2c;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_USART1 | RCC_APB2Periph_SPI1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_9; gpio.GPIO_Mode = GPIO_Mode_AF_PP; gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_10; gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);
    usart.USART_BaudRate = 115200; usart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    usart.USART_WordLength = USART_WordLength_8b; usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No; usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &usart); USART_Cmd(USART1, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7; gpio.GPIO_Mode = GPIO_Mode_AF_PP; GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_6; gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING; GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = ADXL345_CS_PIN; gpio.GPIO_Mode = GPIO_Mode_Out_PP; GPIO_Init(GPIOA, &gpio);
    GPIO_SetBits(GPIOA, ADXL345_CS_PIN);
    spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex; spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b; spi.SPI_CPOL = SPI_CPOL_High;
    spi.SPI_CPHA = SPI_CPHA_2Edge; spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; spi.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_Init(SPI1, &spi); SPI_Cmd(SPI1, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; gpio.GPIO_Mode = GPIO_Mode_AF_OD; GPIO_Init(GPIOB, &gpio);
    i2c.I2C_ClockSpeed = 100000; i2c.I2C_Mode = I2C_Mode_I2C;
    i2c.I2C_DutyCycle = I2C_DutyCycle_2; i2c.I2C_OwnAddress1 = 0x00;
    i2c.I2C_Ack = I2C_Ack_Enable; i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &i2c); I2C_Cmd(I2C1, ENABLE);
}

void UART_String(char *s) {
    while (*s) {
        USART_SendData(USART1, *s++);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }
}

uint8_t SPI_RW(uint8_t data) {
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, data);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(SPI1);
}

int main(void) {
    int16_t ax, ay, az; 
    uint16_t lux; 
    float d_temp = 0, d_humi = 0; 
    uint8_t m, l, h1, h2, t1, t2, ck, res, i, j;
    uint8_t data[5];
    GPIO_InitTypeDef g;
    SystemInit(); Timer2_Config(); Peripherals_Init();
    GPIO_ResetBits(GPIOA, ADXL345_CS_PIN); SPI_RW(0x31); SPI_RW(0x0B); GPIO_SetBits(GPIOA, ADXL345_CS_PIN);
    GPIO_ResetBits(GPIOA, ADXL345_CS_PIN); SPI_RW(0x2D); SPI_RW(0x08); GPIO_SetBits(GPIOA, ADXL345_CS_PIN);
    while (1) {
        GPIO_ResetBits(GPIOA, ADXL345_CS_PIN); SPI_RW(0xC0 | 0x32);
        ax = SPI_RW(0x00) | (SPI_RW(0x00) << 8); ay = SPI_RW(0x00) | (SPI_RW(0x00) << 8); az = SPI_RW(0x00) | (SPI_RW(0x00) << 8);
        GPIO_SetBits(GPIOA, ADXL345_CS_PIN);
        I2C_GenerateSTART(I2C1, ENABLE); while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
        I2C_Send7bitAddress(I2C1, BH1750_ADDR, I2C_Direction_Receiver);
        while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
        while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)); m = I2C_ReceiveData(I2C1);
        I2C_AcknowledgeConfig(I2C1, DISABLE); I2C_GenerateSTOP(I2C1, ENABLE);
        while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)); l = I2C_ReceiveData(I2C1);
        I2C_AcknowledgeConfig(I2C1, ENABLE); lux = ((m << 8) | l) / 1.2;
        g.GPIO_Pin = DHT_PIN; g.GPIO_Mode = GPIO_Mode_Out_OD; g.GPIO_Speed = GPIO_Speed_50MHz; GPIO_Init(DHT_PORT, &g);
        GPIO_ResetBits(DHT_PORT, DHT_PIN); delay_ms(18); GPIO_SetBits(DHT_PORT, DHT_PIN); delay_us(30);
        g.GPIO_Mode = GPIO_Mode_IPU; GPIO_Init(DHT_PORT, &g); delay_us(40);
        if (!GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN)) {
            delay_us(80); delay_us(80);
            for(j=0; j<5; j++) {
                res = 0; for(i=0; i<8; i++) {
                    while(!GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN)); delay_us(40);
                    if(GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN)) { res |= (1<<(7-i)); while(GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN)); }
                } data[j] = res;
            }
            h1 = data[0]; h2 = data[1]; t1 = data[2]; t2 = data[3]; ck = data[4];
            if((uint8_t)(h1+h2+t1+t2) == ck) {
                d_humi = (float)((h1<<8)|h2)/10.0; d_temp = (float)(((t1&0x7F)<<8)|t2)/10.0; if(t1&0x80) d_temp*=-1;
            }
        }
        sprintf(buf, "ACC:%d,%d,%d | LUX:%u | T:%.1fC H:%.1f%%\r\n", ax, ay, az, lux, d_temp, d_humi);
        UART_String(buf); delay_ms(2000);
    }
}