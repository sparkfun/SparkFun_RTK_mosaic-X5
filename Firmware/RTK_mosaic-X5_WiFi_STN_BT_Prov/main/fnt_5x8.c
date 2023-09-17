#include "ssd1306.h"
#include "fnt_5x8.h"

void ssd1306_draw_58char(ssd1306_handle_t dev, uint8_t chXpos, uint8_t chYpos, uint8_t chChar)
{
    uint8_t i, j;
    uint8_t chTemp = 0, chYpos0 = chYpos, chMode = 0;

    for (i = 0; i < 5; i++) {
        chTemp = font5x7_data[(uint16_t)chChar * 5 + i];
        for (j = 0; j < 8; j++) {
            chMode = chTemp & 0x80 ? 1 : 0;
            ssd1306_fill_point(dev, chXpos, chYpos, chMode);
            chTemp <<= 1;
            chYpos++;
            }
        chYpos = chYpos0;
        chXpos++;
    }
}


