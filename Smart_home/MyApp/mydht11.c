#include "mydht11.h"

/**
 * @brief   微秒级延时函数(使用定时器1)
 * @param   us 延时的微秒数
 */
void Delay_us(uint16_t us)
{
    // 计算计数器起始值，确保延时精度
    uint16_t differ = 0xffff - us - 5;
    __HAL_TIM_SET_COUNTER(&htim1, differ); // 设置定时器初始值
    HAL_TIM_Base_Start(&htim1);            // 启动定时器

    // 循环等待直到延时完成
    while (differ < 0xffff - 5)
    {
        differ = __HAL_TIM_GET_COUNTER(&htim1); // 获取当前计数值
    }
    HAL_TIM_Base_Stop(&htim1); // 停止定时器
}

/**
 * @brief   复位 DHT11 函数(即主机发送起始信号)
 */
void DHT11_Rst(void)
{
    DHT11_IO_OUT();  // 设置数据引脚为输出模式(用于主机主送发送)
    DHT11_DQ_OUT(0); // 拉低数据线
    HAL_Delay(20);   // 保持低电平至少 18ms
    DHT11_DQ_OUT(1); // 拉高数据线
    Delay_us(30);    // 保持高电平 20~40us
}

/**
 * @brief   检测 DHT11 响应函数(即从机DHT11是否发送响应信号)
 * @return  0 表示检测到 DHT11，1 表示未检测到
 */
uint8_t DHT11_Check(void)
{
    uint8_t retry = 0;
    DHT11_IO_IN(); // 设置数据引脚为输入模式(用于从机发送数据)

    // 等待 DHT11 拉低数据线（40~80us）
    // 从机超时没拉低电平时跳出while循环，同时满足retry >= 100 return 1; // 超时，未检测到 DHT11 跳出函数
    // 从机拉低数据线时跳出while循环，同时retry >= 100不满足，retry归零。等待 DHT11 拉高数据线（40~80us）
    while (DHT11_DQ_IN && retry < 100)
    {
        retry++;
        Delay_us(1);
    }
    if (retry >= 100)
        return 1; // 超时，未检测到 DHT11

    retry = 0;

    // 等待 DHT11 拉高数据线（40~80us）
    // 从机超时没拉高电平时跳出while循环，同时满足retry >= 100 return 1; // 超时，未检测到 DHT11 跳出函数
    // 从机拉高数据线时跳出while循环，同时retry >= 100不满足，retry归零。// return 0; 成功检测到 DHT11。// 跳出函数
    while (!DHT11_DQ_IN && retry < 100)
    {
        retry++;
        Delay_us(1);
    }
    if (retry >= 100)
        return 1; // 超时，未检测到 DHT11

    return 0; // 成功检测到 DHT11
}

/**
 * @brief   从 DHT11 读取一个位
 * @return  读取到的位（0 或 1）
 */
uint8_t DHT11_Read_Bit(void)
{
    uint8_t retry = 0;

    // 等待数据线变为低电平
    while (DHT11_DQ_IN && retry < 100)
    {
        // retry的主要作用是防止程序卡死
        // 当retry达到100时（即等待100us），退出循环避免程序无限等待
        retry++;
        Delay_us(1);
    }

    retry = 0;
    // 等待数据线变为高电平
    while (!DHT11_DQ_IN && retry < 100)
    {
        // retry的主要作用是防止程序卡死
        // 当retry达到100时（即等待100us），退出循环避免程序无限等待
        retry++;
        Delay_us(1);
    }

    // 延时等待，判断是短高电平（0）还是长高电平（1）
    Delay_us(40);

    // 数据位0的格式:先发送一个低信号(54us)，然后再发送一个高信号(典型值为27us)
    // 数据位1的格式:先发送一个低信号(54us)，然后再发送一个高信号(典型值为71us)
    if (DHT11_DQ_IN)
        return 1; // 长高电平表示 1
    else
        return 0; // 短高电平表示 0
}

/**
 * @brief   从 DHT11 读取一个字节
 * @return  读取到的字节数据
 */
uint8_t DHT11_Read_Byte(void)
{
    uint8_t i, dat;
    dat = 0;

    // 通过调用 DHT11_Read_Bit() 8 次，把读到的 8 个数据位按“从高位到低位
    for (i = 0; i < 8; i++) // 读取 8 位数据
    {
        // 将当前的 dat 左移 1 位，为新读入的 bit 腾出最低位位置。
        // 等价于 dat = dat * 2
        // 解释:
        // 假设dat = 0000 0101 (十进制 5)
        // 执行dat <<= 1;后所有位都整体往左推一格，右边空出来补 0  dat=0000 1010(十进制10)
        // 效果就是原数值变成了原来的 2 倍。
        dat <<= 1; // 左移一位

        // 调用 DHT11_Read_Bit() 读出一个 bit（函数返回 0 或 1）。
        // 用按位或把读取到的 bit 放到 dat 的最低位。
        dat |= DHT11_Read_Bit(); // 读取一位数据并存入
    }
    return dat;
}

/**
 * @brief   从 DHT11 读取一次完整数据
 * @param   temp 存储温度值的指针
 * @param   humi 存储湿度值的指针
 * @return  0 表示读取成功，1 表示读取失败
 */
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5]; // 用于存储接收到的 40 位数据（5 个字节）

    uint8_t i; // 数组下标

    DHT11_Rst();            // 发送复位信号(即主机发送起始信号)
    if (DHT11_Check() == 0) // 检测到 DHT11 响应(即从机DHT11是否发送响应信号) 0 表示检测到 DHT11，1 表示未检测到
    {
        for (i = 0; i < 5; i++) // 读取 5 个字节的数据
        {                       // 将每个字节的数据分别存储在buf[0] - buf[4]中
            buf[i] = DHT11_Read_Byte();
        }

        // 验证校验和是否正确（前四个字节之和等于第五个字节说明校验成功）
        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            *humi = buf[0]; // 湿度整数部分
            *temp = buf[2]; // 温度整数部分
        }
        else
        {
            return 1; // 校验和错误，读取失败
        }
    }
    else
    {
        return 1; // 未检测到 DHT11 响应，读取失败
    }

    return 0; // 读取成功
}

/**
 * @brief   初始化 DHT11 函数
 * @return  0 表示初始化成功，1 表示初始化失败
 */
uint8_t DHT11_Init(void)
{
    // 配置 DHT11 数据引脚
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE(); // 启用 GPIOA 时钟

    GPIO_InitStruct.Pin = GPIO_PIN_8;             // PA8 引脚
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;   // 推挽输出模式
    GPIO_InitStruct.Pull = GPIO_NOPULL;           // 不上下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; // 高速模式
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);       // 初始化 GPIO

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET); // 设置 PA8 为高电平 // 初始为高电平方便拉低数据线

    HAL_TIM_Base_Start(&htim1);

    DHT11_Rst();          // 发送复位信号 (即主机发送起始信号)
    return DHT11_Check(); // 检测 DHT11 是否存在(即从机DHT11是否发送响应信号)
}

// 湿度和温度变量，用于存储传感器数据
uint8_t humi;
uint8_t temp;

/**
 * @brief   DHT11 温湿度传感器任务函数
 */
void dht11_task(void)
{
    // 读取 DHT11 传感器数据，将温度存储到 temp，湿度存储到 humi
    if (DHT11_Read_Data(&temp, &humi) == 0)
    {
        // 通过 UART 打印温湿度数据
        my_printf(&huart1, "temp:%d,humi:%d\r\n", temp, humi);
    }
    else
    {
        my_printf(&huart1, "ERROR - Failed to read DHT11\r\n");
    }
}
