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
	 if(buzzer_bit1 == 0 && buzzer_bit1 == 0)
	 {
		 Buzzer_Off();
	 }
	 else
	 {
		 Buzzer_On();
	 }	
	
}
