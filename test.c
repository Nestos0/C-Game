#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdbool.h>

// 假设的全局变量
typedef struct {
    char *start;
    // ... 其他字段
} InputBuffer;
InputBuffer *cur_input = NULL;

typedef struct {
    bool is_running;
} GameState;
GameState game_state = {true};

// --- 初始化终端为游戏模式（关键！）---
void init_terminal_raw_mode() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO); // 关闭行缓冲和回显
    // 设置为非阻塞读取（替代 kbhit）
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

// 恢复终端
void restore_terminal() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void screen_clear() {
    printf("\033[2J\033[H"); // ANSI 转义序列清屏
    fflush(stdout);
}

// --- 你原来的函数，彻底重写版 ---
void process_input(void) {
    char buffer[1]; // 游戏输入通常一次只读一个按键
    // read 的返回值必须是 ssize_t
    ssize_t bytes_read; 

    // 在 Linux 下，替代 kbhit 的方法就是直接用非阻塞的 read
    bytes_read = read(STDIN_FILENO, buffer, 1);

    if (bytes_read > 0) {
        // 成功读到了一个字符
        char ch = buffer[0];

        // 注意：由于我们关闭了 ICANON，Ctrl-D 现在会被当成普通字节 4 传进来！
        // 如果你依然想用 Ctrl-D 退出，判断 ASCII 4 (EOT)
        if (ch == 4) { 
            screen_clear();
            game_state.is_running = false;
        } else {
            if (cur_input && cur_input->start) {
                // 存入字符
                cur_input->start[0] = ch;
                // 指针后移一位
                cur_input->start += 1; 
            }
        }
    } else if (bytes_read == 0) {
        // 在非阻塞模式下，如果返回 0，通常意味着标准输入被关闭了（比如管道断了）
        game_state.is_running = false;
    }
    // 如果 bytes_read == -1，说明没有按键按下（非阻塞的正常表现），直接忽略即可
}

int main(void) {
    // 分配测试用的输入缓冲区
    char my_buffer[100] = {0};
    cur_input = malloc(sizeof(InputBuffer));
    cur_input->start = my_buffer;

    init_terminal_raw_mode(); // 进入游戏模式
    screen_clear();

    // 游戏主循环
    while (game_state.is_running) {
        process_input(); // 不断轮询键盘
        
        // 这里可以放你的游戏渲染逻辑
        // usleep(10000); // 睡眠 10 毫秒，降低 CPU 占用
    }

    printf("Game Over. You typed: %s\n", my_buffer);
    
    restore_terminal(); // 退出前一定要恢复终端！
    free(cur_input);
    return 0;
}
