#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

void Delay_ms(unsigned int time);
void Config_GPIO(void);

int main(void)
{
    Config_GPIO();
    
    while(1)
    {
        GPIO_SetBits(GPIOC, GPIO_Pin_13);
        Delay_ms(500);
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);
        Delay_ms(500);
    }
}

void Config_GPIO(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void Delay_ms(unsigned int time)
{
    unsigned int i, j;
    for(i = 0; i < time; i++)
        for(j = 0; j < 0x2AFF; j++);
}