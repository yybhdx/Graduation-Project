cmake_minimum_required(VERSION 3.5)

# 告诉编译器我们要使用的唤醒词模型 (注意拼写)
# 通常是 "wn9_nihaoxiaozhi"
set(esp_sr_models "wn9_nihaoxiaozhi")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wake_word_detection)