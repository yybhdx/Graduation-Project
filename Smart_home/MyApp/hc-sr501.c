#include "hc-sr501.h"
#include "gpio.h"
#include "usart.h"
#include "myoled.h"

extern uint8_t buzzer_bit2;

uint8_t hc_sr501_value = 0;

void hc_sr501_task(void)
{
	
  // 有人时为高电平，没人时为低电平，所以使用下拉电阻
  hc_sr501_value = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
	
  if(hc_sr501_value == 1)
  {
	buzzer_bit2 = 1;
   }
  else
  {
	buzzer_bit2 = 0;
  }
  
  my_printf(&huart1, "hc_sr501_value = %d\r\n",hc_sr501_value);
  
}
