#include <stdint.h>
#include "tl_common.h"
#include "main.h"
#include "epd.h"
#include "epd_spi.h"
#include "epd_bwr_213.h"
#include "drivers.h"
#include "stack/ble/ble.h"

// SSD1675 mixed with SSD1680 EPD Controller

/**
 * （已弃用，保留备查）官方 Waveshare EPD_2in13_V3 的 WF_PARTIAL_2IN13_V3，取前 153 字节。
 * 原来那份是「错位拷贝」——每组少了第 1 个字节、整体左移、尾部还缺 VCOM 段。
 * 但换成正确内容后，这块三色屏上依然一点都擦不掉（见 EPD_BWR_213_Display_Partial 的说明），
 * 所以现在局刷只用 OTP 波形，这个数组不再被引用。
 */
#define BWR_213_Len 153
uint8_t LUT_bwr_213_part[BWR_213_Len] = {

    0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00,

};

#define EPD_BWR_213_test_pattern 0xA5
_attribute_ram_code_ uint8_t EPD_BWR_213_detect(void)
{
    // SW Reset
    EPD_WriteCmd(0x12);
    WaitMs(10);

    EPD_WriteCmd(0x32);
    int i;
    for (i = 0; i < 153; i++) // This model has a 159 bytes LUT storage so we test for that
    {
        EPD_WriteData(EPD_BWR_213_test_pattern);
    }
    EPD_WriteCmd(0x33);
    for (i = 0; i < 153; i++)
    {
        if (EPD_SPI_read() != EPD_BWR_213_test_pattern)
            return 0;
    }
    return 1;
}

_attribute_ram_code_ uint8_t EPD_BWR_213_read_temp(void)
{
    uint8_t epd_temperature = 0 ;

    // SW Reset
    EPD_WriteCmd(0x12);

    EPD_CheckStatus_inverted(100);

    // Set Analog Block control
    EPD_WriteCmd(0x74);
    EPD_WriteData(0x54);
    // Set Digital Block control
    EPD_WriteCmd(0x7E);
    EPD_WriteData(0x3B);

    // Booster soft start
    EPD_WriteCmd(0x0C);
    EPD_WriteData(0x8B);
    EPD_WriteData(0x9C);
    EPD_WriteData(0x96);
    EPD_WriteData(0x0F);

    // Driver output control
    EPD_WriteCmd(0x01);
    EPD_WriteData(0x28);
    EPD_WriteData(0x01);
    EPD_WriteData(0x01);

    // Data entry mode setting
    EPD_WriteCmd(0x11);
    EPD_WriteData(0x01);

    // Set RAM X- Address Start/End
    EPD_WriteCmd(0x44);
    EPD_WriteData(0x00);
    EPD_WriteData(0x0F);

    // Set RAM Y- Address Start/End
    EPD_WriteCmd(0x45);
    EPD_WriteData(0x27);   //0x0127-->(295+1)=296
	EPD_WriteData(0x01);
	EPD_WriteData(0x00);
	EPD_WriteData(0x00);

    // Border waveform control
    EPD_WriteCmd(0x3C);
    EPD_WriteData(0x05);

    // Display update control
    EPD_WriteCmd(0x21);
    EPD_WriteData(0x00);
    EPD_WriteData(0x80);

    // Temperature sensor control
    EPD_WriteCmd(0x18);
    EPD_WriteData(0x80);

    // Display update control
    EPD_WriteCmd(0x22);
    EPD_WriteData(0xB1);

    // Master Activation
    EPD_WriteCmd(0x20);

    EPD_CheckStatus_inverted(100);

    // Temperature sensor read from register
    EPD_WriteCmd(0x1B);
    epd_temperature = EPD_SPI_read();
    EPD_SPI_read();

    WaitMs(5);

    // deep sleep
    EPD_WriteCmd(0x10);
    EPD_WriteData(0x01);

    return epd_temperature;
}

_attribute_ram_code_ uint8_t EPD_BWR_213_Display(unsigned char *image, int size, uint8_t full_or_partial)
{
    uint8_t epd_temperature = 0 ;

    // SW Reset
    EPD_WriteCmd(0x12);

    EPD_CheckStatus_inverted(100);

    // Set Analog Block control
    EPD_WriteCmd(0x74);
    EPD_WriteData(0x54);
    // Set Digital Block control
    EPD_WriteCmd(0x7E);
    EPD_WriteData(0x3B);

    // Booster soft start
    EPD_WriteCmd(0x0C);
    EPD_WriteData(0x8B);
    EPD_WriteData(0x9C);
    EPD_WriteData(0x96);
    EPD_WriteData(0x0F);

    // Driver output control
    EPD_WriteCmd(0x01);
    EPD_WriteData(0x28);
    EPD_WriteData(0x01);
    EPD_WriteData(0x01);

    // Data entry mode setting
    EPD_WriteCmd(0x11);
    EPD_WriteData(0x01);

    // Set RAM X- Address Start/End
    EPD_WriteCmd(0x44);
    EPD_WriteData(0x00);
    EPD_WriteData(0x0F);

    // Set RAM Y- Address Start/End
    EPD_WriteCmd(0x45);
    EPD_WriteData(0x28);   //0x0127-->(295+1)=296
	EPD_WriteData(0x01);
	EPD_WriteData(0x00);
	EPD_WriteData(0x00);

    // Border waveform control
    EPD_WriteCmd(0x3C);
    EPD_WriteData(full_or_partial ? 0x05 : 0x80);

    // Display update control
    EPD_WriteCmd(0x21);
    EPD_WriteData(0x00);
    EPD_WriteData(0x80);

    // Temperature sensor control
    EPD_WriteCmd(0x18);
    EPD_WriteData(0x80);

    // Display update control
    EPD_WriteCmd(0x22);
    EPD_WriteData(0xB1);

    // Master Activation
    EPD_WriteCmd(0x20);

    EPD_CheckStatus_inverted(100);

    // Temperature sensor read from register
    EPD_WriteCmd(0x1B);
    epd_temperature = EPD_SPI_read();
    EPD_SPI_read();

    WaitMs(5);

    // Set RAM X address
    EPD_WriteCmd(0x4E);
    EPD_WriteData(0x00);

    // Set RAM Y address
    EPD_WriteCmd(0x4F);
    EPD_WriteData(0x28);
    EPD_WriteData(0x01);

    EPD_LoadImage(image, size, 0x24);

    // Set RAM X address
    EPD_WriteCmd(0x4E);
    EPD_WriteData(0x00);

    // Set RAM Y address
    EPD_WriteCmd(0x4F);
    EPD_WriteData(0x28);
    EPD_WriteData(0x01);

    int i;
    if (full_or_partial)
    {
        // 全刷才清红平面；局刷不动它（红平面和"旧图"共用 RAM，写它会把局刷基准冲掉）
        EPD_WriteCmd(0x26);
        for (i = 0; i < size; i++)
        {
            EPD_WriteData(0x00);
        }
    }

    // 局刷已不走这两个函数（见 EPD_BWR_213_Display_Partial）：
    // 这块屏上 host 写的局刷 LUT 一点都擦不掉，所以不再往 0x32 写自定义 LUT，
    // 留在这里只会误伤后续刷新，直接删掉。

    // Display update control
    EPD_WriteCmd(0x22);
    EPD_WriteData(0xC7);

    // Master Activation
    EPD_WriteCmd(0x20);

    return epd_temperature;
}

_attribute_ram_code_ uint8_t EPD_BWR_213_Display_BWR(unsigned char *image, unsigned char *red_image, int size, uint8_t full_or_partial)
{
    if (red_image == NULL) {
        return EPD_BWR_213_Display(image, size, full_or_partial);
    }

    uint8_t epd_temperature = 0 ;

    // SW Reset
    EPD_WriteCmd(0x12);

    EPD_CheckStatus_inverted(100);

    // Set Analog Block control
    EPD_WriteCmd(0x74);
    EPD_WriteData(0x54);
    // Set Digital Block control
    EPD_WriteCmd(0x7E);
    EPD_WriteData(0x3B);

    // Booster soft start
    EPD_WriteCmd(0x0C);
    EPD_WriteData(0x8B);
    EPD_WriteData(0x9C);
    EPD_WriteData(0x96);
    EPD_WriteData(0x0F);

    // Driver output control
    EPD_WriteCmd(0x01);
    EPD_WriteData(0x28);
    EPD_WriteData(0x01);
    EPD_WriteData(0x01);

    // Data entry mode setting
    EPD_WriteCmd(0x11);
    EPD_WriteData(0x01);

    // Set RAM X- Address Start/End
    EPD_WriteCmd(0x44);
    EPD_WriteData(0x00);
    EPD_WriteData(0x0F);

    // Set RAM Y- Address Start/End
    EPD_WriteCmd(0x45);
    EPD_WriteData(0x28);   //0x0127-->(295+1)=296
	EPD_WriteData(0x01);
	EPD_WriteData(0x00);
	EPD_WriteData(0x00);

    // Border waveform control
    EPD_WriteCmd(0x3C);
    EPD_WriteData(full_or_partial ? 0x05 : 0x80);

    // Display update control
    EPD_WriteCmd(0x21);
    EPD_WriteData(0x00);
    EPD_WriteData(0x80);

    // Temperature sensor control
    EPD_WriteCmd(0x18);
    EPD_WriteData(0x80);

    // Display update control
    EPD_WriteCmd(0x22);
    EPD_WriteData(0xB1);

    // Master Activation
    EPD_WriteCmd(0x20);

    EPD_CheckStatus_inverted(100);

    // Temperature sensor read from register
    EPD_WriteCmd(0x1B);
    epd_temperature = EPD_SPI_read();
    EPD_SPI_read();

    WaitMs(5);

    // Set RAM X address
    EPD_WriteCmd(0x4E);
    EPD_WriteData(0x00);

    // Set RAM Y address
    EPD_WriteCmd(0x4F);
    EPD_WriteData(0x28);
    EPD_WriteData(0x01);

    EPD_LoadImage(image, size, 0x24);

    // Set RAM X address
    EPD_WriteCmd(0x4E);
    EPD_WriteData(0x00);

    // Set RAM Y address
    EPD_WriteCmd(0x4F);
    EPD_WriteData(0x28);
    EPD_WriteData(0x01);

    if (full_or_partial) // 局刷不动红平面
        EPD_LoadImage(red_image, size, 0x26);

    // 局刷已不走这两个函数（见 EPD_BWR_213_Display_Partial）：
    // 这块屏上 host 写的局刷 LUT 一点都擦不掉，所以不再往 0x32 写自定义 LUT，
    // 留在这里只会误伤后续刷新，直接删掉。

    // Display update control
    EPD_WriteCmd(0x22);
    EPD_WriteData(0xC7);

    // Master Activation
    EPD_WriteCmd(0x20);

    return epd_temperature;
}

/**
 * 局刷。
 *
 * 这块三色屏（BWR213 / SSD1675+1680）上，"host 写的局刷波形"完全不被驱动
 * （见下面 LUT 数组注释），只有控制器 **OTP 里的波形**能擦干净。
 *
 * 两个独立开关，改完烧一次对比：
 *
 * 1) EPD_BWR213_PARTIAL_TRIGGER —— 决定用 OTP 里哪一档波形
 *      1 = 0x18 → 0x22=0xB1(载 OTP 波形) → 0x24 → 0x22=0xC7   【已验证：擦得干净，但 ~10s、全屏闪】
 *      3 = 0x3C=0x80 → 0x24 → 0x22=0xFF                        【SSD1680/V4 官方式】
 *      4 = 0xB1 载波形 + 0x3C=0x80 + 0xFF
 *
 * 2) EPD_BWR213_PARTIAL_SEQ —— 直接指定 0x22 的字节（换 OTP 波形档，见下）
 *
 * 3) EPD_BWR213_PARTIAL_WINDOW —— 刷哪些 gate（实测：减不了闪，只有整屏波形在动）
 *      0 = 整屏（和之前一样，128 条 gate 全扫，所以全屏闪）
 *      1 = 只刷时间数字所在的那条带：gate X = WIN_X0..WIN_X1（≈ 面板竖直 40..87 行）
 *          → 刷新时间按 gate 行数等比缩短（48/128），闪烁也只出现在这条带上
 *          → 代价：这条带以外的内容这一次不刷新（静态内容本来就没错，每小时的全刷会兜）
 *
 * 注：坐标是按 FixBuffer 那套变换算的 —— epd_buffer[X + 16*Y]，
 *     控制器 X = 面板的"竖直 8 像素带"、Y = 面板的"水平位置"。
 *     时间串画在 (x=25, y=85)，DSEG14 字高 ~40px → py 45..85 → X = 5..10。
 *     如果烧出来发现刷新的位置不在数字上，就调下面这两个数。
 */
#define EPD_BWR213_PARTIAL_TRIGGER 1
#define EPD_BWR213_PARTIAL_WINDOW 0
#define EPD_BWR213_WIN_X0 5
#define EPD_BWR213_WIN_X1 10

/* 局刷时是否省掉"重新载入 OTP 波形"这一步（0x18 + 0x22=0xB1 + 0x20）。
   OTP 波形在整点全刷时已经载进去，而且我们不再写自定义 LUT，所以它会一直有效。
   设 1 可以少一次 0x20 动作（如果你看到"闪两下"多半就是它）。
   预算里默认 0（保守，和已验证能擦干净的那条完全一致）。 */
#define EPD_BWR213_PARTIAL_SKIP_OTP_RELOAD 0

/* 直接指定最后那次 0x22（"update sequence"）用的字节 —— 不同值会选 OTP 里不同的波形档。
   0    = 用 TRIGGER 的默认值（TRIGGER 1 → 0xC7；3/4 → 0xFF）
   想找"不整屏推白"的波形就把下面改成 0xFF / 0xF7 / 0xFC / 0xD7 / 0xE0 / 0x83 一个个试。 */
#define EPD_BWR213_PARTIAL_SEQ 0xD7

_attribute_ram_code_ void EPD_BWR_213_Display_Partial(unsigned char *image, int size)
{
#if (EPD_BWR213_PARTIAL_WINDOW)
    int xx, yy;
#endif

#if ((EPD_BWR213_PARTIAL_TRIGGER == 1) || (EPD_BWR213_PARTIAL_TRIGGER == 4)) && (!EPD_BWR213_PARTIAL_SKIP_OTP_RELOAD)
    // 温度传感器 + 载入 OTP 波形
    EPD_WriteCmd(0x18);
    EPD_WriteData(0x80);

    EPD_WriteCmd(0x22);
    EPD_WriteData(0xB1);
    EPD_WriteCmd(0x20);
    EPD_CheckStatus_inverted(100);
#endif

#if (EPD_BWR213_PARTIAL_TRIGGER == 3) || (EPD_BWR213_PARTIAL_TRIGGER == 4)
    // partial 的 BorderWaveform
    EPD_WriteCmd(0x3C);
    EPD_WriteData(0x80);
#endif

#if (EPD_BWR213_PARTIAL_WINDOW)
    // ---- 只刷时间数字那条 gate 带 ----
    EPD_WriteCmd(0x44); // RAM X start/end（= 面板竖直方向的片选）
    EPD_WriteData(EPD_BWR213_WIN_X0);
    EPD_WriteData(EPD_BWR213_WIN_X1);

    // Y 窗口/起始点必须和"整屏那套"完全一致（0x0128 起、Y 递减），
    // 否则 Y 的相位会绕一圈，画面会整体平移/跑到别处去。
    EPD_WriteCmd(0x45); // RAM Y start/end（= 面板水平方向）
    EPD_WriteData(0x28);
    EPD_WriteData(0x01);
    EPD_WriteData(0x00);
    EPD_WriteData(0x00);

    EPD_WriteCmd(0x4E); // Set RAM X address
    EPD_WriteData(EPD_BWR213_WIN_X0);

    EPD_WriteCmd(0x4F); // Set RAM Y address
    EPD_WriteData(0x28);
    EPD_WriteData(0x01);

    EPD_WriteCmd(0x24);
    for (yy = 0; yy < 256; yy++)
    {
        for (xx = EPD_BWR213_WIN_X0; xx <= EPD_BWR213_WIN_X1; xx++)
        {
            EPD_WriteData(image[xx + (yy << 4)]); // epd_buffer[X + 16*Y]
        }
    }
#else
    // ---- 整屏 ----
    // Set RAM X address / Set RAM Y address（沿用本驱动的区域参数）
    EPD_WriteCmd(0x4E);
    EPD_WriteData(0x00);

    EPD_WriteCmd(0x4F);
    EPD_WriteData(0x28);
    EPD_WriteData(0x01);

    // 只写黑白平面
    EPD_LoadImage(image, size, 0x24);
#endif

    // 触发
    EPD_WriteCmd(0x22);
#if (EPD_BWR213_PARTIAL_SEQ)
    EPD_WriteData(EPD_BWR213_PARTIAL_SEQ); // 手动指定的 sequence
#elif (EPD_BWR213_PARTIAL_TRIGGER == 1)
    EPD_WriteData(0xC7);
#else
    EPD_WriteData(0xFF);
#endif
    EPD_WriteCmd(0x20);
}

_attribute_ram_code_ void EPD_BWR_213_set_sleep(void)
{
    // deep sleep
    EPD_WriteCmd(0x10);
    EPD_WriteData(0x01);

}