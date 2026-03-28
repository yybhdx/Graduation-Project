#ifndef _MYDHT11_H
#define _MYDHT11_H
#include "usart.h"

#include <stdint.h>
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_tim.h"


// GPIOx_CRH = GPIO 端口 配置寄存器 High（Control Register High）。用来配置 高 8 个引脚（PIN8 ~ PIN15） 的模式和功能。

// DHT11 数据引脚配置为输入模式
// GPIOA->CRH &= 0XFFFFFFF0;?
// 作用：清空GPIOA配置寄存器高位（CRH）中针对Pin 8的配置位。0XFFFFFFF0是一个掩码，其二进制表示为1111 1111 1111 1111 1111 1111 1111 0000。这条语句的作用是将CRH寄存器的第0-3位（对应Pin 8）清零，同时不影响其他引脚（Pin 9到Pin 15）的配置。
// GPIOA->CRH |= 8;
// 作用：将Pin 8配置为输入模式。数字8的二进制是1000。
// 其中 bit[1:0] (MODE8) 设置为 00，表示输入模式。
// 其中 bit[3:2] (CNF8) 设置为 10，根据STM32参考手册，10表示配置为上拉/下拉输入模式。结合后续通常会对ODR（输出数据寄存器）的操作（例如设置为1以上拉），此处的 10通常与上拉输入模式对应。
#define DHT11_IO_IN()  {GPIOA->CRH&=0XFFFFFFF0;GPIOA->CRH|=8;}

// DHT11 数据引脚配置为输出模式
// GPIOA->CRH &= 0XFFFFFFF0;
// 作用：与输入模式相同，先清空Pin 8的原有配置，为重新配置做准备。
// GPIOA->CRH |= 3;
// 作用：将Pin 8配置为推挽输出模式，最大速度50MHz。数字3的二进制是0011。
// 其中 bit[1:0] (MODE8) 设置为 11，表示输出模式，最大速度50MHz。
// 其中 bit[3:2] (CNF8) 设置为 00，表示通用推挽输出模式。
#define DHT11_IO_OUT() {GPIOA->CRH&=0XFFFFFFF0;GPIOA->CRH|=3;}

// DHT11 数据引脚输出宏定义
#define DHT11_DQ_OUT(x) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, (x)?GPIO_PIN_SET:GPIO_PIN_RESET)
#define DHT11_DQ_IN     HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8)

extern TIM_HandleTypeDef htim1;
extern UART_HandleTypeDef huart1;

extern uint8_t humi;
extern uint8_t temp;


// 微秒级延时函数，用于精确控制信号时序
void Delay_us(uint16_t us);

// 初始化 DHT11 函数，配置 GPIO 并检测传感器是否存在
uint8_t DHT11_Init(void);

// 读取 DHT11 数据函数，获取温度和湿度值
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi);

// DHT11 任务函数，用于定期读取传感器数据并打印
void dht11_task(void);

#endif
