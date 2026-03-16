#include "mymq-7.h"
#include "usart.h"

extern uint8_t buzzer_bit2;

uint32_t mq7_adc_value = 0;

float ppm = 0;

/*HAL_ADC_PollForConversion函数的返回值*/
HAL_StatusTypeDef adc_poll_return;

// float voltage = 0;

void mq7_task(void)
{

  HAL_ADC_Start(&hadc1); // 1. 启动 ADC

  if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
  {

    /*3. 读取转换结果*/
    mq7_adc_value = HAL_ADC_GetValue(&hadc1);
	  
  }
  else
  {
    my_printf(&huart1, "adc_read_error\r\n");
  }

  HAL_ADC_Stop(&hadc1);

  /*打印adc值*/
  my_printf(&huart1, "adc_value= %d\r\n", mq7_adc_value);


  float Vol = ((float)mq7_adc_value * 5 / 4096.0f);
  float RS = (5 - Vol) / (Vol * 0.5);
  float R0 = 6.64;

  ppm = pow(11.5428 * R0 / RS, 0.6549f);
  
	if( (mq7_adc_value < 4000) &&  (ppm < 2000))
	{
		buzzer_bit2 = 0;
	}
	else
	{
		buzzer_bit2 = 1;
	}

  /*打印ppm值*/
  my_printf(&huart1, "ppm_value = %.f\r\n", ppm);

}
