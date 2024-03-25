#include <stdio.h>
#include <sys/types.h>                                       
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/input.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include "lcd.h"

int main(int argc, char const *argv[])
{
    // 初始化（lcd+触摸屏）
    
    LcdInfo info;
    lcd_init(&info);
    
    touch_draw(info);
    // 退出（释放资源）
    return 0;
}
