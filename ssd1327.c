#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include "ssd1327.h"

int ssd1327_cmd(int dev, unsigned char *cmd, unsigned char len)
{
    unsigned char buf[4];
    int ret = 0;
    char i = 0;
    if(len<1 || len>3)
    return -1;

    buf[0] = 0x00; // command
    for(i=0; i<len; i++)
    {
    buf[i+1] = cmd[i];
    }

    ret = write(dev, buf, len+1);
    if(ret != len+1)
        return -1;
    else
        return 0;
}

int ssd1327_data(int dev, unsigned char *data, unsigned char len)
{
    unsigned char buf[129];
    int ret = 0;
    char i = 0;

    buf[0] = 0x40; // data
    for(i=0; i<len; i++)
    {
        buf[i+1] = data[i];
    }

    ret = write(dev, buf, len+1);
    if(ret != len+1)
        return -1;
    else
        return 0;
}

int ssd1327_init(int dev)
{
    unsigned char cmd[3];
    
    cmd[0] = 0xae;
    ssd1327_cmd(dev, cmd, 1);

    cmd[0] = 0xa0;
    cmd[1] = 0x01<<6;
    ssd1327_cmd(dev, cmd, 2);

    cmd[0] = 0xa1;
    cmd[1] = 0x00;
    ssd1327_cmd(dev, cmd, 2);

    cmd[0] = 0xa2;
    cmd[1] = 0x00;
    ssd1327_cmd(dev, cmd, 2);

    cmd[0] = 0xa4;
    ssd1327_cmd(dev, cmd, 1);

    cmd[0] = 0xa8;
    cmd[1] = 0x7f;
    ssd1327_cmd(dev, cmd, 2);

    cmd[0] = 0xaf;
    ssd1327_cmd(dev, cmd, 1);
}
/* 设置显示窗口，映射到 GDDRAM ，坐标的取值范围是 0~127 */
void ssd1327_setwindow(int dev, unsigned char xstart, unsigned char ystart, unsigned char xend, unsigned char yend)
{
    unsigned char cmd[3];

    cmd[0] = 0x15;
    cmd[1] = xstart/2;
    cmd[2] = xend/2;
    ssd1327_cmd(dev, cmd, 3);

    cmd[0] = 0x75;
    cmd[1] = ystart;
    cmd[2] = yend;
    ssd1327_cmd(dev, cmd, 3);
}

/* 向坐标围成的窗口填充同样的数据 */
int ssd1327_fill(int dev, unsigned char data, unsigned char xstart, unsigned char ystart, unsigned char xend, unsigned char yend)
{
    int i = 0;
    int ret = 0;
    unsigned char buf[RAM_SIZE+1];

    memset(buf, data, RAM_SIZE+1);

    ssd1327_setwindow(dev, xstart, ystart, xend, yend);

    buf[0] = 0x40; // data
    ret = write(dev, buf, (yend-ystart+1)*(xend/2-xstart/2+1)+1);

    return ret;
}

/* 两个像素点的状态，转换为 16bit 颜色深度
   p : 0x00~0x03
   color : 0x00~0x0f
*/
static inline unsigned char p2r(unsigned char p, unsigned char color)
{
    switch(p) {
        case 0 :
            return 0;
        case 1 :
            return color&0x0f;
        case 2 :
            return (color&0x0f)<<4;
        case 3 :
            return (color&0x0f)<<4|(color&0x0f);
        default :
            return 0;
    }

}

/* 在指定坐标写一个 8x16 bit 的 ASCII 字符 */
int ssd1327_print_char(int dev, unsigned char ch, unsigned char xstart, unsigned char ystart, unsigned char color)
{
    int i, j, k = 0;
    int ret = 0;
    unsigned char row_byte;
    unsigned char w_buf[16*4+1];

    w_buf[k++] = 0x40; // data

    for(i=0; i<16; i++)
    {
        // 从字库中提前字符 ch 的点阵数据, 每行 1 个字节，共 16 个字节
        row_byte = ascii_8x16[(ch-' ')*16+i];

        // 处理一行，八个像素，占四个字节
        for(j=0; j<4; j++)
        {
            w_buf[k++] = p2r((row_byte>>(j*2))&0x03, color);
        }
    }

    ssd1327_setwindow(dev, xstart, ystart, xstart+7, ystart+15);

    ret = write(dev, w_buf, k);

    return ret;
}

/* 8x16 bit ASCII */
const unsigned char ascii_8x16[] = 
{
/* [字库]：[ASC8x16E字库] [数据排列]:从上到下从左到右 [取模方式]:横向8点右高位 [正负反色]:否 [去掉重复后]共95个字符
[总字符库]：" !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~"*/

/*-- ID:0,字符:" ",ASCII编码:20,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/*-- ID:1,字符:"!",ASCII编码:21,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x18,0x3c,0x3c,0x3c,0x18,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,

/*-- ID:2,字符:""",ASCII编码:22,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x66,0x66,0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/*-- ID:3,字符:"#",ASCII编码:23,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x36,0x36,0x7f,0x36,0x36,0x36,0x7f,0x36,0x36,0x00,0x00,0x00,

/*-- ID:4,字符:"$",ASCII编码:24,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x18,0x18,0x18,0x3e,0x63,0x43,0x03,0x3e,0x60,0x61,0x63,0x3e,0x18,0x18,0x00,0x00,

/*-- ID:5,字符:"%",ASCII编码:25,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x43,0x63,0x30,0x18,0x0c,0x06,0x63,0x61,0x00,0x00,0x00,

/*-- ID:6,字符:"&",ASCII编码:26,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x1c,0x36,0x36,0x1c,0x6e,0x3b,0x33,0x33,0x33,0x6e,0x00,0x00,0x00,

/*-- ID:7,字符:"'",ASCII编码:27,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x0c,0x0c,0x0c,0x0c,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/*-- ID:8,字符:"(",ASCII编码:28,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x30,0x18,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x18,0x30,0x00,0x00,0x00,

/*-- ID:9,字符:")",ASCII编码:29,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x0c,0x18,0x30,0x30,0x30,0x30,0x30,0x30,0x18,0x0c,0x00,0x00,0x00,

/*-- ID:10,字符:"*",ASCII编码:2A,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x66,0x3c,0xff,0x3c,0x66,0x00,0x00,0x00,0x00,0x00,

/*-- ID:11,字符:"+",ASCII编码:2B,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x7e,0x18,0x18,0x00,0x00,0x00,0x00,0x00,

/*-- ID:12,字符:",",ASCII编码:2C,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x18,0x0c,0x00,0x00,

/*-- ID:13,字符:"-",ASCII编码:2D,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/*-- ID:14,字符:".",ASCII编码:2E,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,

/*-- ID:15,字符:"/",ASCII编码:2F,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x40,0x60,0x30,0x18,0x0c,0x06,0x03,0x01,0x00,0x00,0x00,

/*-- ID:16,字符:"0",ASCII编码:30,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x3e,0x63,0x63,0x73,0x6b,0x6b,0x67,0x63,0x63,0x3e,0x00,0x00,0x00,

/*-- ID:17,字符:"1",ASCII编码:31,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x18,0x1c,0x1e,0x18,0x18,0x18,0x18,0x18,0x18,0x7e,0x00,0x00,0x00,

/*-- ID:18,字符:"2",ASCII编码:32,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x3e,0x63,0x60,0x30,0x18,0x0c,0x06,0x03,0x63,0x7f,0x00,0x00,0x00,

/*-- ID:19,字符:"3",ASCII编码:33,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x3e,0x63,0x60,0x60,0x3c,0x60,0x60,0x60,0x63,0x3e,0x00,0x00,0x00,

/*-- ID:20,字符:"4",ASCII编码:34,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x30,0x38,0x3c,0x36,0x33,0x7f,0x30,0x30,0x30,0x78,0x00,0x00,0x00,

/*-- ID:21,字符:"5",ASCII编码:35,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x7f,0x03,0x03,0x03,0x3f,0x70,0x60,0x60,0x63,0x3e,0x00,0x00,0x00,

/*-- ID:22,字符:"6",ASCII编码:36,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x1c,0x06,0x03,0x03,0x3f,0x63,0x63,0x63,0x63,0x3e,0x00,0x00,0x00,

/*-- ID:23,字符:"7",ASCII编码:37,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x7f,0x63,0x60,0x60,0x30,0x18,0x0c,0x0c,0x0c,0x0c,0x00,0x00,0x00,

/*-- ID:24,字符:"8",ASCII编码:38,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x3e,0x63,0x63,0x63,0x3e,0x63,0x63,0x63,0x63,0x3e,0x00,0x00,0x00,

/*-- ID:25,字符:"9",ASCII编码:39,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x3e,0x63,0x63,0x63,0x7e,0x60,0x60,0x60,0x30,0x1e,0x00,0x00,0x00,

/*-- ID:26,字符:":",ASCII编码:3A,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,

/*-- ID:27,字符:";",ASCII编码:3B,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x0c,0x00,0x00,0x00,

/*-- ID:28,字符:"<",ASCII编码:3C,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x60,0x30,0x18,0x0c,0x06,0x0c,0x18,0x30,0x60,0x00,0x00,0x00,

/*-- ID:29,字符:"=",ASCII编码:3D,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,0x00,

/*-- ID:30,字符:">",ASCII编码:3E,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x06,0x0c,0x18,0x30,0x60,0x30,0x18,0x0c,0x06,0x00,0x00,0x00,

/*-- ID:31,字符:"?",ASCII编码:3F,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x3e,0x63,0x63,0x30,0x18,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,

/*-- ID:32,字符:"@",ASCII编码:40,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x3e,0x63,0x63,0x7b,0x7b,0x7b,0x3b,0x03,0x3e,0x00,0x00,0x00,

/*-- ID:33,字符:"A",ASCII编码:41,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x08,0x1c,0x36,0x63,0x63,0x7f,0x63,0x63,0x63,0x63,0x00,0x00,0x00,

/*-- ID:34,字符:"B",ASCII编码:42,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x3f,0x66,0x66,0x66,0x3e,0x66,0x66,0x66,0x66,0x3f,0x00,0x00,0x00,

/*-- ID:35,字符:"C",ASCII编码:43,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x3c,0x66,0x43,0x03,0x03,0x03,0x03,0x43,0x66,0x3c,0x00,0x00,0x00,

/*-- ID:36,字符:"D",ASCII编码:44,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x1f,0x36,0x66,0x66,0x66,0x66,0x66,0x66,0x36,0x1f,0x00,0x00,0x00,

/*-- ID:37,字符:"E",ASCII编码:45,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x7f,0x66,0x46,0x16,0x1e,0x16,0x06,0x46,0x66,0x7f,0x00,0x00,0x00,

/*-- ID:38,字符:"F",ASCII编码:46,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x7f,0x66,0x46,0x16,0x1e,0x16,0x06,0x06,0x06,0x0f,0x00,0x00,0x00,

/*-- ID:39,字符:"G",ASCII编码:47,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x3c,0x66,0x43,0x03,0x03,0x7b,0x63,0x63,0x66,0x5c,0x00,0x00,0x00,

/*-- ID:40,字符:"H",ASCII编码:48,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x63,0x63,0x63,0x63,0x7f,0x63,0x63,0x63,0x63,0x63,0x00,0x00,0x00,

/*-- ID:41,字符:"I",ASCII编码:49,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x3c,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3c,0x00,0x00,0x00,

/*-- ID:42,字符:"J",ASCII编码:4A,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x78,0x30,0x30,0x30,0x30,0x30,0x33,0x33,0x33,0x1e,0x00,0x00,0x00,

/*-- ID:43,字符:"K",ASCII编码:4B,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x67,0x66,0x36,0x36,0x1e,0x1e,0x36,0x66,0x66,0x67,0x00,0x00,0x00,

/*-- ID:44,字符:"L",ASCII编码:4C,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x0f,0x06,0x06,0x06,0x06,0x06,0x06,0x46,0x66,0x7f,0x00,0x00,0x00,

/*-- ID:45,字符:"M",ASCII编码:4D,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x63,0x77,0x7f,0x7f,0x6b,0x63,0x63,0x63,0x63,0x63,0x00,0x00,0x00,

/*-- ID:46,字符:"N",ASCII编码:4E,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x63,0x67,0x6f,0x7f,0x7b,0x73,0x63,0x63,0x63,0x63,0x00,0x00,0x00,

/*-- ID:47,字符:"O",ASCII编码:4F,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x1c,0x36,0x63,0x63,0x63,0x63,0x63,0x63,0x36,0x1c,0x00,0x00,0x00,

/*-- ID:48,字符:"P",ASCII编码:50,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x3f,0x66,0x66,0x66,0x3e,0x06,0x06,0x06,0x06,0x0f,0x00,0x00,0x00,

/*-- ID:49,字符:"Q",ASCII编码:51,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x3e,0x63,0x63,0x63,0x63,0x63,0x63,0x6b,0x7b,0x3e,0x30,0x70,0x00,

/*-- ID:50,字符:"R",ASCII编码:52,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x3f,0x66,0x66,0x66,0x3e,0x36,0x66,0x66,0x66,0x67,0x00,0x00,0x00,

/*-- ID:51,字符:"S",ASCII编码:53,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x3e,0x63,0x63,0x06,0x1c,0x30,0x60,0x63,0x63,0x3e,0x00,0x00,0x00,

/*-- ID:52,字符:"T",ASCII编码:54,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x7e,0x7e,0x5a,0x18,0x18,0x18,0x18,0x18,0x18,0x3c,0x00,0x00,0x00,

/*-- ID:53,字符:"U",ASCII编码:55,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x3e,0x00,0x00,0x00,

/*-- ID:54,字符:"V",ASCII编码:56,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x36,0x1c,0x08,0x00,0x00,0x00,

/*-- ID:55,字符:"W",ASCII编码:57,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x63,0x63,0x63,0x63,0x63,0x6b,0x6b,0x7f,0x36,0x36,0x00,0x00,0x00,

/*-- ID:56,字符:"X",ASCII编码:58,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x63,0x63,0x36,0x36,0x1c,0x1c,0x36,0x36,0x63,0x63,0x00,0x00,0x00,

/*-- ID:57,字符:"Y",ASCII编码:59,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x3c,0x18,0x18,0x18,0x18,0x3c,0x00,0x00,0x00,

/*-- ID:58,字符:"Z",ASCII编码:5A,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x7f,0x63,0x61,0x30,0x18,0x0c,0x06,0x43,0x63,0x7f,0x00,0x00,0x00,

/*-- ID:59,字符:"[",ASCII编码:5B,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x3c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x3c,0x00,0x00,0x00,

/*-- ID:60,字符:"\",ASCII编码:5C,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x01,0x03,0x07,0x0e,0x1c,0x38,0x70,0x60,0x40,0x00,0x00,0x00,

/*-- ID:61,字符:"]",ASCII编码:5D,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x3c,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x3c,0x00,0x00,0x00,

/*-- ID:62,字符:"^",ASCII编码:5E,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x08,0x1c,0x1c,0x36,0x63,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/*-- ID:63,字符:"_",ASCII编码:5F,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x00,

/*-- ID:64,字符:"`",ASCII编码:60,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x0c,0x0c,0x0c,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/*-- ID:65,字符:"a",ASCII编码:61,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x1e,0x30,0x3e,0x33,0x33,0x33,0x6e,0x00,0x00,0x00,

/*-- ID:66,字符:"b",ASCII编码:62,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x07,0x06,0x06,0x1e,0x36,0x66,0x66,0x66,0x66,0x3b,0x00,0x00,0x00,

/*-- ID:67,字符:"c",ASCII编码:63,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x63,0x03,0x03,0x03,0x63,0x3e,0x00,0x00,0x00,

/*-- ID:68,字符:"d",ASCII编码:64,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x38,0x30,0x30,0x3c,0x36,0x33,0x33,0x33,0x33,0x6e,0x00,0x00,0x00,

/*-- ID:69,字符:"e",ASCII编码:65,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x63,0x7f,0x03,0x03,0x63,0x3e,0x00,0x00,0x00,

/*-- ID:70,字符:"f",ASCII编码:66,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x1c,0x36,0x26,0x06,0x0f,0x06,0x06,0x06,0x06,0x0f,0x00,0x00,0x00,

/*-- ID:71,字符:"g",ASCII编码:67,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x6e,0x33,0x33,0x33,0x33,0x33,0x3e,0x30,0x33,0x1e,

/*-- ID:72,字符:"h",ASCII编码:68,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x07,0x06,0x06,0x36,0x6e,0x66,0x66,0x66,0x66,0x67,0x00,0x00,0x00,

/*-- ID:73,字符:"i",ASCII编码:69,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x18,0x18,0x00,0x1c,0x18,0x18,0x18,0x18,0x18,0x3c,0x00,0x00,0x00,

/*-- ID:74,字符:"j",ASCII编码:6A,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x60,0x60,0x00,0x70,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x3c,

/*-- ID:75,字符:"k",ASCII编码:6B,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x07,0x06,0x06,0x66,0x36,0x1e,0x1e,0x36,0x66,0x67,0x00,0x00,0x00,

/*-- ID:76,字符:"l",ASCII编码:6C,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x1c,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3c,0x00,0x00,0x00,

/*-- ID:77,字符:"m",ASCII编码:6D,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x37,0x7f,0x6b,0x6b,0x6b,0x6b,0x6b,0x00,0x00,0x00,

/*-- ID:78,字符:"n",ASCII编码:6E,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x3b,0x66,0x66,0x66,0x66,0x66,0x66,0x00,0x00,0x00,

/*-- ID:79,字符:"o",ASCII编码:6F,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x63,0x63,0x63,0x63,0x63,0x3e,0x00,0x00,0x00,

/*-- ID:80,字符:"p",ASCII编码:70,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x3b,0x66,0x66,0x66,0x66,0x66,0x3e,0x06,0x06,0x0f,

/*-- ID:81,字符:"q",ASCII编码:71,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x6e,0x33,0x33,0x33,0x33,0x33,0x3e,0x30,0x30,0x78,

/*-- ID:82,字符:"r",ASCII编码:72,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x3b,0x6e,0x46,0x06,0x06,0x06,0x0f,0x00,0x00,0x00,

/*-- ID:83,字符:"s",ASCII编码:73,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x63,0x06,0x1c,0x30,0x63,0x3e,0x00,0x00,0x00,

/*-- ID:84,字符:"t",ASCII编码:74,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x08,0x0c,0x0c,0x3f,0x0c,0x0c,0x0c,0x0c,0x6c,0x38,0x00,0x00,0x00,

/*-- ID:85,字符:"u",ASCII编码:75,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x33,0x33,0x33,0x33,0x33,0x33,0x6e,0x00,0x00,0x00,

/*-- ID:86,字符:"v",ASCII编码:76,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x3c,0x18,0x00,0x00,0x00,

/*-- ID:87,字符:"w",ASCII编码:77,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x63,0x63,0x63,0x6b,0x6b,0x7f,0x36,0x00,0x00,0x00,

/*-- ID:88,字符:"x",ASCII编码:78,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x63,0x36,0x1c,0x1c,0x1c,0x36,0x63,0x00,0x00,0x00,

/*-- ID:89,字符:"y",ASCII编码:79,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x63,0x63,0x63,0x63,0x63,0x63,0x7e,0x60,0x30,0x1f,

/*-- ID:90,字符:"z",ASCII编码:7A,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0x33,0x18,0x0c,0x06,0x63,0x7f,0x00,0x00,0x00,

/*-- ID:91,字符:"{",ASCII编码:7B,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x70,0x18,0x18,0x18,0x0e,0x18,0x18,0x18,0x18,0x70,0x00,0x00,0x00,

/*-- ID:92,字符:"|",ASCII编码:7C,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x18,0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,

/*-- ID:93,字符:"}",ASCII编码:7D,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x0e,0x18,0x18,0x18,0x70,0x18,0x18,0x18,0x18,0x0e,0x00,0x00,0x00,

/*-- ID:94,字符:"~",ASCII编码:7E,对应字:宽x高=8x16,画布:宽W=8 高H=16,共16字节*/
0x00,0x00,0x00,0x6e,0x3b,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00

};