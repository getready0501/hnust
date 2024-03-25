#ifndef _LCD_H_
#define _LCD_H_

typedef struct lcd_info
{
    int fd_lcd;
    int fd_ts;
    int *p_lcd;
}LcdInfo;

int lcd_init(LcdInfo* info);
int get_xy(int *x,int *y,int fd_ts);
void lcd_draw_point(int i, int j, unsigned int color,LcdInfo info);
int display_bmp(char *bmppath, int x0, int y0,LcdInfo info);
void get_position();
void lcd_draw_rectangle(int x0, int y0, int width, int height, unsigned int color,LcdInfo info);
void touch_draw(LcdInfo info);
void lcd_draw_circle(int *center_x, int *center_y, int *rad,unsigned int color, LcdInfo info);
void circle_cal(int avg_x, int avg_y, int r, unsigned int color, LcdInfo info);
void draw_air_core_rectangle(int *center_x, int *center_y,int *r,unsigned int color, LcdInfo info);
void rect_cal(int avg_x, int avg_y, int r, unsigned int color, LcdInfo info);
void color_fill(int center_x, int center_y, int rad, unsigned int color[], int sharp_get, LcdInfo info);


#endif