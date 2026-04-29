#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

void get_terminal_size(int *width, int *height)
{
	struct winsize w;
	// 使用 STDOUT_FILENO (1) 获取标准输出终端的信息
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
		*width = w.ws_col;
		*height = w.ws_row;
	} else {
		// 如果获取失败（例如输出被重定向到了文件），设置默认值
		*width = 80;
		*height = 24;
	}
}

int main()
{
	int w, h;
	get_terminal_size(&w, &h);
	printf("当前终端宽度: %d, 高度: %d\n", w, h);
	return 0;
}
