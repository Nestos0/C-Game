#include <stdio.h>

int main() {
    // 1. 定义模板
    const char *template = "Status Code: %s";
    char buffer[50]; // 准备一个足够大的容纳结果的“容器”
    
    // 2. 将 %s 替换为具体的数字（以字符串形式）
    sprintf(buffer, template, "404");
    
    printf("%s\n", buffer); // 输出: Status Code: 404
    return 0;
}
