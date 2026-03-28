#include "buzzer.h"
#include "gpio.h"

extern uint8_t buzzer_bit1;
extern uint8_t buzzer_bit2;

/*低电平响*/
void Buzzer_On()
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
}


/*高电平不响*/
void Buzzer_Off()
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
}

void Buzzer_Task()
{
	// 蜂鸣器不响
	 if(buzzer_bit1 == 0 && buzzer_bit2 == 0)
	 {
		 Buzzer_Off();
	 }
	 // 出现一种状况：检测到人或一氧化碳浓度值超标，蜂鸣器就响
	 else
	 {
		 Buzzer_On();
	 }	
	
}
