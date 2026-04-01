#include "oled_software_i2c.h"
#include "oledfont.h"

/* ~1us per half-bit at 80MHz (80 cycles) — SSD1306 needs ≥400ns */
#define I2C_DELAY()  delay_cycles(80)

/* ---- I2C bit-bang ---- */
static void I2C_Start(void)
{
    OLED_SDA_Set(); OLED_SCL_Set();
    I2C_DELAY();
    OLED_SDA_Clr();
    I2C_DELAY();
    OLED_SCL_Clr();
}

static void I2C_Stop(void)
{
    OLED_SDA_Clr(); OLED_SCL_Set();
    I2C_DELAY();
    OLED_SDA_Set();
    I2C_DELAY();
}

static void I2C_WaitAck(void)
{
    OLED_SDA_Set();
    I2C_DELAY();
    OLED_SCL_Set();
    I2C_DELAY();
    OLED_SCL_Clr();
}

static void Send_Byte(uint8_t dat)
{
    for (uint8_t i = 0; i < 8; i++) {
        OLED_SCL_Clr();
        I2C_DELAY();
        if (dat & 0x80) OLED_SDA_Set();
        else            OLED_SDA_Clr();
        I2C_DELAY();
        OLED_SCL_Set();
        I2C_DELAY();
        OLED_SCL_Clr();
        dat <<= 1;
    }
}

void OLED_WR_Byte(uint8_t dat, uint8_t mode)
{
    I2C_Start();
    Send_Byte(0x78);
    I2C_WaitAck();
    Send_Byte(mode ? 0x40 : 0x00);
    I2C_WaitAck();
    Send_Byte(dat);
    I2C_WaitAck();
    I2C_Stop();
}

/* ---- Display control ---- */
void OLED_Set_Pos(uint8_t x, uint8_t y)
{
    OLED_WR_Byte(0xb0 + y,               OLED_CMD);
    OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD);
    OLED_WR_Byte((x & 0x0f),             OLED_CMD);
}

void OLED_Display_On(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x14, OLED_CMD);
    OLED_WR_Byte(0xAF, OLED_CMD);
}

void OLED_Display_Off(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x10, OLED_CMD);
    OLED_WR_Byte(0xAE, OLED_CMD);
}

void OLED_Clear(void)
{
    for (uint8_t i = 0; i < 8; i++) {
        OLED_WR_Byte(0xb0 + i, OLED_CMD);
        OLED_WR_Byte(0x00,     OLED_CMD);
        OLED_WR_Byte(0x10,     OLED_CMD);
        for (uint8_t n = 0; n < 128; n++) OLED_WR_Byte(0, OLED_DATA);
    }
}

/* ---- Font rendering ---- */
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t sizey)
{
    uint8_t c = chr - ' ';
    uint8_t sizex = sizey / 2;
    uint16_t size1;
    if (sizey == 8)  size1 = 6;
    else             size1 = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * (sizey / 2);

    OLED_Set_Pos(x, y);
    for (uint16_t i = 0; i < size1; i++) {
        if (i % sizex == 0 && sizey != 8) OLED_Set_Pos(x, y++);
        if      (sizey == 8)  OLED_WR_Byte(asc2_0806[c][i], OLED_DATA);
        else if (sizey == 16) OLED_WR_Byte(asc2_1608[c][i], OLED_DATA);
        else return;
    }
}

uint32_t oled_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while (n--) result *= m;
    return result;
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t sizey)
{
    uint8_t m = (sizey == 8) ? 2 : 0;
    uint8_t enshow = 0;
    for (uint8_t t = 0; t < len; t++) {
        uint8_t temp = (num / oled_pow(10, len - t - 1)) % 10;
        if (!enshow && t < (len - 1)) {
            if (temp == 0) { OLED_ShowChar(x + (sizey / 2 + m) * t, y, ' ', sizey); continue; }
            else enshow = 1;
        }
        OLED_ShowChar(x + (sizey / 2 + m) * t, y, temp + '0', sizey);
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr, uint8_t sizey)
{
    uint8_t j = 0;
    while (chr[j] != '\0') {
        OLED_ShowChar(x, y, chr[j++], sizey);
        x += (sizey == 8) ? 6 : (sizey / 2);
    }
}

void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t len1, uint8_t len2, uint8_t sizey)
{
    uint8_t negative = 0;
    if (num < 0) { negative = 1; num = -num; }
    uint32_t int_part = (uint32_t)num;
    uint32_t dec_part = (uint32_t)((num - int_part) * oled_pow(10, len2) + 0.5f);
    if (dec_part >= oled_pow(10, len2)) { int_part++; dec_part = 0; }

    uint8_t cw = (sizey == 8) ? 6 : (sizey / 2);
    uint8_t cx = x;
    if (negative) { OLED_ShowChar(cx, y, '-', sizey); cx += cw; }
    OLED_ShowNum(cx, y, int_part, len1, sizey); cx += len1 * cw;
    OLED_ShowChar(cx, y, '.', sizey); cx += cw;
    for (uint8_t i = 0; i < len2; i++) {
        OLED_ShowChar(cx, y, dec_part / oled_pow(10, len2 - 1 - i) % 10 + '0', sizey);
        cx += cw;
    }
}

/* ---- Init ---- */
void OLED_Init(void)
{
    /* 200ms power-up delay — bare-metal busy-wait (80MHz × 0.2s = 16 000 000 cycles) */
    delay_cycles(16000000UL);

    OLED_WR_Byte(0xAE, OLED_CMD);
    OLED_WR_Byte(0x00, OLED_CMD);
    OLED_WR_Byte(0x10, OLED_CMD);
    OLED_WR_Byte(0x40, OLED_CMD);
    OLED_WR_Byte(0x81, OLED_CMD);
    OLED_WR_Byte(0xCF, OLED_CMD);
    OLED_WR_Byte(0xA1, OLED_CMD);
    OLED_WR_Byte(0xC8, OLED_CMD);
    OLED_WR_Byte(0xA6, OLED_CMD);
    OLED_WR_Byte(0xA8, OLED_CMD);
    OLED_WR_Byte(0x3f, OLED_CMD);
    OLED_WR_Byte(0xD3, OLED_CMD);
    OLED_WR_Byte(0x00, OLED_CMD);
    OLED_WR_Byte(0xd5, OLED_CMD);
    OLED_WR_Byte(0x80, OLED_CMD);
    OLED_WR_Byte(0xD9, OLED_CMD);
    OLED_WR_Byte(0xF1, OLED_CMD);
    OLED_WR_Byte(0xDA, OLED_CMD);
    OLED_WR_Byte(0x12, OLED_CMD);
    OLED_WR_Byte(0xDB, OLED_CMD);
    OLED_WR_Byte(0x40, OLED_CMD);
    OLED_WR_Byte(0x20, OLED_CMD);
    OLED_WR_Byte(0x02, OLED_CMD);
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x14, OLED_CMD);
    OLED_WR_Byte(0xA4, OLED_CMD);
    OLED_WR_Byte(0xA6, OLED_CMD);
    OLED_Clear();
    OLED_WR_Byte(0xAF, OLED_CMD);
}
