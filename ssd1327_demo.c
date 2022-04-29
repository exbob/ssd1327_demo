#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include "ssd1327.h"

int main()
{
    int i2c_fd = 0;
    int ret = 0;
    int i = 0;
    unsigned char color = 0;

    i2c_fd = open("/dev/i2c-2", O_RDWR);
    ret = ioctl(i2c_fd, I2C_SLAVE, CHIP_ADDR);
    if(ret != 0)
    {
        printf("ioctl error: %d\n", ret);
        return -1;
    }

    ssd1327_init(i2c_fd);
    usleep(100000);

    // 清空屏幕显示的内容
    ssd1327_fill(i2c_fd, 0x00, 0, 0, 127, 127);    
    usleep(1000000);

    // 全部显示为最亮
    ssd1327_fill(i2c_fd, 0xff, 0, 0, 127, 127);
    usleep(1000000);

    // 半边显示为渐变色，每八行显示一个颜色
    for(i=0x0; i<=0x0f; i++)
    {
        color=(i<<4)+i;
        ssd1327_fill(i2c_fd, color, 0, i*8, 64, i*8+7);
    }

    // 显示一段文字
    ssd1327_print_char(i2c_fd, 'H', 0, 0, 0x0f);
    ssd1327_print_char(i2c_fd, 'e', 8, 0, 0x0f);
    ssd1327_print_char(i2c_fd, 'l', 8*2, 0, 0x0f);
    ssd1327_print_char(i2c_fd, 'l', 8*3, 0, 0x0f);
    ssd1327_print_char(i2c_fd, 'o', 8*4, 0, 0x0f);
    ssd1327_print_char(i2c_fd, '!', 8*5, 0, 0x0f);

    close(i2c_fd);
}
