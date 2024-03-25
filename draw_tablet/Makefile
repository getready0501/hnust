#变量
TARGET:=draw_tablet_last			#最后的可执行文件名
SRC:=					#指定所有的源文件.c
OBJS:= 					#指定所有的目标文件.o
CC:=arm-linux-gnueabi-gcc #编译器名  如果要在开发板允许 改成arm-linux-gnueabi-gcc
INCS:=-I ./inc			#指定头文件的搜索路径
LIBS:=-L ./lib             #指定库文件的搜索路径
LIBNAME:= -lm				#指定库的名字

SRC+=$(wildcard ./src/*.c) #展开所有的.c文件 赋值给SRC
OBJS+=$(patsubst %.c,%.o,$(SRC)) #把src中所有的.c文件的名字转成.o赋值给OBJS
$(TARGET):$(OBJS)
	$(CC) $^ -o $@ $(INCS) $(LIBS) $(LIBNAME)
	sudo cp $(TARGET) /srv/tftp/
%.o:%.c
	$(CC) -c $< -o $@ $(INCS) $(LIBS) $(LIBNAME)

clean:
	rm -rf $(OBJS) $(TARGET) #删除所有的.o文件以及可执行文件