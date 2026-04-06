#include "mymq-7.h"
#include "usart.h"
#include "adc.h"

extern uint8_t buzzer_bit1;

uint32_t mq7_adc_value = 0;

float ppm = 0;

// float voltage = 0;

void mq7_task(void)
{

  HAL_ADC_Start(&hadc1); // 1. 启动 ADC

  if (HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY) == HAL_OK)
  {

    /*3. 读取转换结果*/
    mq7_adc_value = HAL_ADC_GetValue(&hadc1);
	  
  }
  else
  {
    my_printf(&huart1, "adc_read_error\r\n");
  }

  /*打印adc值*/
  my_printf(&huart1, "adc_value= %d\r\n", mq7_adc_value);


  float Vol = ((float)mq7_adc_value * 5 / 4096.0f);
  float RS = (5 - Vol) / (Vol * 0.5);
  float R0 = 6.64;

  ppm = pow(11.5428 * R0 / RS, 0.6549f);
  
//  if( (mq7_adc_value < 4000) &&  (ppm < 2000))
//  {
//	buzzer_bit1 = 0;
//  }
//  else
// {
//	buzzer_bit1 = 1;
//  }
 
   if( (mq7_adc_value >= 2000))
  {
	buzzer_bit1 = 1;
  }
  else
 {
	buzzer_bit1 = 0;
  }

  /*打印ppm值*/
  my_printf(&huart1, "ppm_value = %.f\r\n", ppm);

}
