#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "driver/i2s_std.h"

// 引脚定义
#define MIC_I2S_SCK (12)
#define MIC_I2S_WS  (11)
#define MIC_I2S_SD  (10)
#define SPK_I2S_BCLK (16)
#define SPK_I2S_LRC  (15)
#define SPK_I2S_DIN  (7)

void audio_manager_init(void);
void audio_manager_start_loopback(void);

#endif
