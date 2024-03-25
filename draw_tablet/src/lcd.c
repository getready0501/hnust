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

#define LCD_PATH "/dev/fb0"
#define TS_PATH "/dev/input/event0"
#define LCD_WIDTH 800
#define LCD_HEIGHT 480
#define LCD_SIZE LCD_HEIGHT *LCD_WIDTH * 4

int lcd_init(LcdInfo *info)
{
    // 打开LCD

    // 打开设备文件
    info->fd_lcd = open(LCD_PATH, O_RDWR);
    if (info->fd_lcd == -1)
    {
        perror("open LCD failed");
        return -1;
    }
    // 内存映射
    info->p_lcd = mmap(NULL, LCD_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, info->fd_lcd, 0);
    if (MAP_FAILED == info->p_lcd)
    {
        return -1;
    }

    // 触摸屏
    info->fd_ts = open(TS_PATH, O_RDONLY);
    if (-1 == info->fd_ts)
    {
        return -1;
    }
    return 0;
}



void lcd_draw_point(int i, int j, unsigned int color, LcdInfo info)
{
    if (i >= 0 && i < 480 && j >= 0 && j < 800)
        *(info.p_lcd + i * 800 + j) = color;
}

int display_bmp(char *bmppath, int x0, int y0, LcdInfo info)
{
    int bmp_fd = open(bmppath, O_RDWR);
    if (bmp_fd == -1)
    {
        perror("open bmppath failed:");
        return -1;
    }

    // 拿到图片的宽度
    int width = 0;
    lseek(bmp_fd, 0x12, SEEK_SET);
    int res1 = read(bmp_fd, &width, 4);
    if (res1 == -1)
    {
        perror("read width failed:");
        return -1;
    }
    // printf("width = %d\n", width);

    // 拿到图片的高度
    int height = 0;
    lseek(bmp_fd, 0x16, SEEK_SET);
    int res2 = read(bmp_fd, &height, 4);
    if (res2 == -1)
    {
        perror("read height failed:");
        return -1;
    }
    // printf("height = %d\n", height);

    // 拿到图片的色深
    int depth = 0;
    lseek(bmp_fd, 0x1c, SEEK_SET);
    int res3 = read(bmp_fd, &depth, 2);
    if (res3 == -1)
    {
        perror("read depth failed:");
        return -1;
    }
    // printf("depth = %d\n", depth);

    // 每一行需要的赖子数
    int laizi = 0;
    if (abs(width) * depth / 8 % 4)
    {
        laizi = 4 - abs(width) * depth / 8 % 4;
    }
    // 一行总的字节数
    int line_bytes = laizi + abs(width) * depth / 8;
    // 整个像素数组的大小
    int total_bytes = line_bytes * abs(height);
    // 读取像素数组
    char p[total_bytes];
    char buf[4]; // 用来存储argb四个字节
    lseek(bmp_fd, 0x36, SEEK_SET);
    read(bmp_fd, p, total_bytes);

    unsigned char a, r, g, b;
    laizi = 0;
    int i = 0; // p数组的下标
    unsigned int color;
    // int x0 = 0, y0 = 0; // x0y0为图片显示的起始位置的行列
    if (abs(width) * depth / 8 % 4)
    {
        laizi = 4 - abs(width) * depth / 8 % 4;
    }
    for (int x = 0; x < abs(height); x++) // 行
    {
        for (int y = 0; y < abs(width); y++) // 列
        {
            b = p[i++];
            g = p[i++];
            r = p[i++];
            if (depth == 32)
            {
                a = p[i++];
            }
            else if (depth == 24)
            {
                a = 0;
            }
            // 组转成像素点
            color = a << 24 | r << 16 | g << 8 | b;
            // 画点
            lcd_draw_point(height > 0 ? abs(height) - x - 1 + x0 : x + x0, width > 0 ? y + y0 : abs(width) - y - 1 + y0, color, info);
        }
        i += laizi;
    }
    // 关闭图片
    close(bmp_fd);
    return 1;
}



/*
    画矩形
    x0:左上角的起始点的行
    y0:左上角的起始点的列
    width:宽
    height:高
*/
void lcd_draw_rectangle(int x0, int y0, int width, int height, unsigned int color, LcdInfo info)
{
    for (int i = x0; i < x0 + height; i++)
    {
        for (int j = y0; j < y0 + width; j++)
        {
            lcd_draw_point(i, j, color, info);
        }
    }
}

void lcd_draw_circle(int *center_x, int *center_y, int *rad, unsigned int color, LcdInfo info)
{
    int x1 = -1, y1 = -1;
    int x2 = -1, y2 = -1;
    int x = 0;
    int y = 0;
    struct input_event ts_event;

    while (1)
    {
        int ret = read(info.fd_ts, &ts_event, sizeof(ts_event));
        if (ret == -1)
        {
            perror("read touch failed:");
            return;
        }

        if (ts_event.type == EV_ABS)
        {

            if (ts_event.code == ABS_X)
            {
                x = ts_event.value * 800 / 1024;
            }
            else if (ts_event.code == ABS_Y)
            {
                y = ts_event.value * 480 / 600;
            }
        }
        // 记录动手坐标
        if (ts_event.type == EV_KEY && ts_event.code == BTN_TOUCH && ts_event.value == 1)
        {
            x1 = x;
            y1 = y;
        }
        // 记录松手坐标
        if (ts_event.type == EV_KEY && ts_event.code == BTN_TOUCH && ts_event.value == 0)
        {
            x2 = x;
            y2 = y;
            break;
        }
    }
    int avg_x = (x1 + x2) / 2;
    int avg_y = (y1 + y2) / 2;

    *center_x = avg_x;
    *center_y = avg_y;

    int r;
    r = (int)(sqrt(pow((x1 - avg_x), 2) + pow((y1 - avg_y), 2)));
    *rad = r;
    circle_cal(avg_x, avg_y, r, color, info);
}

void circle_cal(int avg_x, int avg_y, int r, unsigned int color, LcdInfo info)
{
    int chazhi[4] = {30, 100, 200, 400};
    int chazhi_get;

    if (2 * r < 120 && 2 * r > 0)
    {
        chazhi_get = chazhi[0];
    }
    else if (2 * r < 240 && 2 * r > 120)
    {
        chazhi_get = chazhi[1];
    }
    else if (2 * r < 480 && 2 * r > 240)
    {
        chazhi_get = chazhi[2];
    }
    else if (2 * r > 480)
    {
        chazhi_get = chazhi[3];
    }
    for (int j = avg_y - r; j <= avg_y + r; j++)
    {
        for (int i = avg_x - r; i <= avg_x + r; i++)
        {
            if (abs((int)(pow((i - avg_x), 2) + pow((j - avg_y), 2)) - (int)(pow(r, 2))) < chazhi_get)
            {
                lcd_draw_rectangle(j - 3, i - 3, 6, 6, color, info);
            }
            if ((j == avg_y + r) && (i == avg_x + r))
            {
                return;
            }
        }
    }
}

void draw_air_core_rectangle(int *center_x, int *center_y, int *rad, unsigned int color, LcdInfo info)
{
    int x1 = -1, y1 = -1;
    int x2 = -1, y2 = -1;
    int x = 0;
    int y = 0;
    struct input_event ts_event;

    while (1)
    {
        int ret = read(info.fd_ts, &ts_event, sizeof(ts_event));
        if (ret == -1)
        {
            perror("read touch failed:");
            return;
        }

        if (ts_event.type == EV_ABS)
        {

            if (ts_event.code == ABS_X)
            {
                x = ts_event.value * 800 / 1024;
            }
            else if (ts_event.code == ABS_Y)
            {
                y = ts_event.value * 480 / 600;
            }
        }
        // 记录动手坐标
        if (ts_event.type == EV_KEY && ts_event.code == BTN_TOUCH && ts_event.value == 1)
        {
            x1 = x;
            y1 = y;
        }
        // 记录松手坐标
        if (ts_event.type == EV_KEY && ts_event.code == BTN_TOUCH && ts_event.value == 0)
        {
            x2 = x;
            y2 = y;
            break;
        }
    }
    int avg_x = (x1 + x2) / 2;
    int avg_y = (y1 + y2) / 2;

    *center_x = avg_x;
    *center_y = avg_y;

    int r;
    r = (int)(sqrt(pow((x1 - avg_x), 2) + pow((y1 - avg_y), 2)));
    *rad = r;

    rect_cal(avg_x, avg_y, r, color, info);
}

void rect_cal(int avg_x, int avg_y, int r, unsigned int color, LcdInfo info)
{

    for (int i = avg_x - r; i <= avg_x + r; i++)
    {
        lcd_draw_rectangle(avg_y - r - 3, i - 3, 6, 6, color, info);
        lcd_draw_rectangle(avg_y + r - 3, i - 3, 6, 6, color, info);
    }
    for (int j = avg_y - r; j <= avg_y + r; j++)
    {
        lcd_draw_rectangle(j - 3, avg_x - r - 3, 6, 6, color, info);
        lcd_draw_rectangle(j - 3, avg_x + r - 3, 6, 6, color, info);
    }
}

void color_fill(int center_x, int center_y, int rad, unsigned int color[], int sharp_get, LcdInfo info)
{
    int x = 0;
    int y = 0;
    unsigned int color_to;
    struct input_event ts_event;

    while (1)
    {
        int ret = read(info.fd_ts, &ts_event, sizeof(ts_event));
        if (ret == -1)
        {
            perror("read touch failed:");
            return;
        }

        if (ts_event.type == EV_ABS)
        {

            if (ts_event.code == ABS_X)
            {
                x = ts_event.value * 800 / 1024;
            }
            else if (ts_event.code == ABS_Y)
            {
                y = ts_event.value * 480 / 600;
            }
        }
        // 记录松手坐标
        if (ts_event.type == EV_KEY && ts_event.code == BTN_TOUCH && ts_event.value == 0)
        {
            break;
        }
    }
    if (x > 0 && x < 65 && y > 0 && y < 65)
    {
        // 点击了 第一区块 线条变为红色
        color_to = color[0];
        printf("点击了 第一区块 填充颜色将为红色\n");
    }
    else if (x > 65 && x < 130 && y > 0 && y < 65)
    {
        // 点击了 第二区块 线条变为蓝色
        color_to = color[2];
        printf("点击了 第二区块 填充颜色将为蓝色\n");
    }
    else if (x > 130 && x < 195 && y > 0 && y < 65)
    {
        // 点击了 第三区块 线条变为绿色
        color_to = color[1];
        printf("点击了 第三区块 填充颜色将为绿色\n");
    }
    else if (x > 195 && x < 260 && y > 0 && y < 65)
    {
        // 点击了 第四区块 线条变为黑色
        color_to = color[3];
        printf("点击了 第四区块 线条变为黑色\n");
    }

    while (1)
    {
        int ret = read(info.fd_ts, &ts_event, sizeof(ts_event));
        // 记录松手坐标
        if (ts_event.type == EV_KEY && ts_event.code == BTN_TOUCH && ts_event.value == 0)
        {
            if (sharp_get == 0)
            {
                for (int j = center_y - rad; j <= center_y + rad; j++)
                {
                    for (int i = center_x - rad; i <= center_x + rad; i++)
                    {
                        if (abs((int)(pow((i - center_x), 2) + pow((j - center_y), 2)) < (int)(pow(rad, 2))) )
                        {
                            lcd_draw_rectangle(j - 3, i - 3, 6, 6, color_to, info);
                        }
                        if ((j == center_y + rad) && (i == center_x + rad))
                        {
                            return;
                        }
                    }
                }
            }
            else if (sharp_get==1)
            {
                for (int  j = center_y-rad; j < center_y+rad; j++)
                {
                    for (int  i = center_x-rad; i < center_x+rad; i++)
                    {
                        lcd_draw_rectangle(j - 3, i - 3, 6, 6, color_to, info);
                    }
                    
                }
                return;
            }
            else if (sharp_get==-1)
            {
                lcd_draw_rectangle(65, 0, 800, 415, color_to, info);
                return;
            }
            break;
        }
    }
}

void touch_draw(LcdInfo info)
{
    unsigned int color[5] = {0xFF0000, 0x00FF00, 0x0000FF, 0x000000, 0xFFFFFF}; // 三原色和黑白
    int rough[3] = {5, 10, 15};                                                 // 定义粗糙程度
    int rough_get = rough[1];
    unsigned int color_get;
    int sharp[3] = {0, 1, -1}; // 0代表圆框，1代表方框,-1代表没有框图
    int sharp_get = sharp[2];
    int center_x=0; // 记录中心点x轴
    int center_y=0; // 记录中心点y轴
    int radius=0;   // 半径

    lcd_draw_rectangle(0, 0, 800, 480, 0xFFFFFF, info); // 第一次进入页面清空为白色
    display_bmp("./pic/choice.bmp", 0, 0, info);        // 在顶部显示可选项
    struct input_event ts_event;

    // 记录xy轴
    int x = 0;
    int y = 0;
    while (1)
    {
        int rea_val = read(info.fd_ts, &ts_event, sizeof(ts_event));
        if (ts_event.type == EV_ABS)
        {

            if (ts_event.code == ABS_X)
            {
                x = ts_event.value * 800 / 1024;
            }
            else if (ts_event.code == ABS_Y)
            {
                y = ts_event.value * 480 / 600;
            }
            // 滑动时涂黑指定区域
            if (x != 0 && y != 0 && y >= 65)
            {
                lcd_draw_rectangle(y - rough_get, x - rough_get, (2 * rough_get), (2 * rough_get), color_get, info);
            }
        }

        // 在屏幕顶部设置可选项
        if (ts_event.type == EV_KEY && ts_event.code == BTN_TOUCH && ts_event.value == 0)
        {
            if (x > 0 && x < 65 && y > 0 && y < 65)
            {
                // 点击了 第一区块 线条变为红色
                color_get = color[0];
                printf("点击了 第一区块 线条变为红色\n");
            }
            else if (x > 65 && x < 130 && y > 0 && y < 65)
            {
                // 点击了 第二区块 线条变为蓝色
                color_get = color[2];
                printf("点击了 第二区块 线条变为蓝色\n");
            }
            else if (x > 130 && x < 195 && y > 0 && y < 65)
            {
                // 点击了 第三区块 线条变为绿色
                color_get = color[1];
                printf("点击了 第三区块 线条变为绿色\n");
            }
            else if (x > 195 && x < 260 && y > 0 && y < 65)
            {
                // 点击了 第四区块 线条变为黑色
                color_get = color[3];
                printf("点击了 第四区块 线条变为黑色\n");
            }
            else if (x > 260 && x < 325 && y > 0 && y < 65)
            {
                // 点击了 第五区块 线条粗度为大
                rough_get = rough[2];
                printf("点击了 第五区块 线条粗度为大\n");
            }
            else if (x > 325 && x < 390 && y > 0 && y < 65)
            {
                // 点击了 第六区块 线条粗度为中
                rough_get = rough[1];
                printf("点击了 第六区块 线条粗度为中\n");
            }
            else if (x > 390 && x < 455 && y > 0 && y < 65)
            {
                // 点击了 第七区块 线条粗度为小
                rough_get = rough[0];
                printf("点击了 第七区块 线条粗度为小\n");
            }
            else if (x > 455 && x < 520 && y > 0 && y < 65)
            {
                // 点击了 第八区块 线条变为白色，实现擦除功能
                color_get = color[4];
                printf("点击了 第八区块 线条变为白色，实现擦除功能\n");
            }
            else if (x > 520 && x < 585 && y > 0 && y < 65)
            {
                // 点击了 第九区块 屏幕清空
                printf("点击了 第九区块 屏幕清空\n");
                lcd_draw_rectangle(65, 0, 800, 415, 0xFFFFFF, info); // 第一次进入页面清空为白色
                display_bmp("./pic/choice.bmp", 0, 0, info);        // 在顶部显示可选项
                sharp_get = sharp[2];                               // 重新定义填充数值
            }
            else if (x > 585 && x < 650 && y > 0 && y < 65)
            {
                // 点击了 第十区块 进行圆框绘画
                printf("点击了 第十区块 进行圆框绘画\n");
                lcd_draw_circle(&center_x, &center_y, &radius, color_get, info);
                sharp_get = sharp[0];
            }
            else if (x > 650 && x < 715 && y > 0 && y < 65)
            {
                // 点击了 第十一区块 进行方框绘画
                printf("点击了 第十一区块 进行方框绘画\n");
                draw_air_core_rectangle(&center_x, &center_y, &radius, color_get, info);
                sharp_get = sharp[1];
            }
            else if (x > 715 && x < 780 && y > 0 && y < 65)
            {
                // 点击了 第十二区块 进行颜色填充
                printf("点击了 第十二区块 进行颜色填充\n");
                color_fill(center_x,center_y,radius,color,sharp_get,info);
                sharp_get = sharp[2];
            }
            x = 0;
            y = 0;
        }
    }

    return;
}