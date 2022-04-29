#ifndef _SSD1327_H_
#define _SSD1327_H_

#define CHIP_ADDR  0x3D
#define X_MAX 128
#define Y_MAX 128
#define RAM_SIZE X_MAX*Y_MAX/2

extern const unsigned char ascii_8x16[];

int ssd1327_cmd(int dev, unsigned char *cmd, unsigned char len);
int ssd1327_data(int dev, unsigned char *data, unsigned char len);
int ssd1327_init(int dev);
void ssd1327_setwindow(int dev, unsigned char xstart, unsigned char ystart, unsigned char xend, unsigned char yend);
int ssd1327_fill(int dev, unsigned char data, unsigned char xstart, unsigned char ystart, unsigned char xend, unsigned char yend);
int ssd1327_print_char(int dev, unsigned char ch, unsigned char xstart, unsigned char ystart, unsigned char color);
#endif