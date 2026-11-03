#pragma once

/* 注意：这里原先误把本文件的函数声明成了 EPD_BWR_296_*（复制粘贴残留），
   导致 epd.c 调用 EPD_BWR_213_* 全是隐式声明。这里改回 213。 */
uint8_t EPD_BWR_213_detect(void);
uint8_t EPD_BWR_213_read_temp(void);
uint8_t EPD_BWR_213_Display(unsigned char *image, int size, uint8_t full_or_partial);
uint8_t EPD_BWR_213_Display_BWR(unsigned char *image, unsigned char *red_image, int size, uint8_t full_or_partial);
void EPD_BWR_213_set_sleep(void);

/* 官方 EPD_2in13_V3 式的局刷（只换 LUT + 开 RAM Ping-Pong，不做 SW reset / 全套上电） */
void EPD_BWR_213_Display_Partial(unsigned char *image, int size);