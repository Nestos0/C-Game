#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <format> // C++20

class BBCodeEngine {
public:
    // 使用 std::string_view 避免不必要的字符串拷贝
    static constexpr std::string_view RESET = "\x1b[0m";
    
    static const inline std::unordered_map<std::string_view, std::string_view> COLOR_MAP = {
        {"red", "\x1b[31m"}, {"green", "\x1b[32m"}, {"yellow", "\x1b[33m"},
        {"blue", "\x1b[34m"}, {"magenta", "\x1b[35m"}, {"cyan", "\x1b[36m"},
        {"white", "\x1b[37m"}
    };

    struct Token {
        bool is_tag;
        std::string content; // 如果是 tag，则是颜色代码；如果是文本，则是原始文字
    };

    /**
     * @brief 解析并验证 BBCode 
     * 逻辑：利用 stack 记录未闭合标签，非法标签直接降级为普通文本
     */
    static std::vector<Token> parse(std::string_view input) {
        std::vector<Token> tokens;
        std::stack<std::string> tag_stack;
        size_t i = 0;

        while (i < input.length()) {
            if (input[i] == '[') {
                size_t j = input.find(']', i);
                if (j != std::string_view::npos) {
                    std::string_view tag_content = input.substr(i + 1, j - i - 1);
                    
                    if (!tag_content.empty() && tag_content[0] == '/') {
                        // 处理闭合标签 [/tag]
                        std::string_view tag_name = tag_content.substr(1);
                        if (!tag_stack.empty() && tag_stack.top() == tag_name) {
                            tag_stack.pop();
                            tokens.push_back({true, std::string(RESET)});
                            // 如果栈内还有标签，恢复上一个颜色
                            if (!tag_stack.empty()) {
                                tokens.push_back({true, std::string(COLOR_MAP.at(tag_stack.top()))});
                            }
                        } else {
                            // 不匹配的尾标签，当作普通文本处理
                            tokens.push_back({false, std::string(input.substr(i, j - i + 1))});
                        }
                    } else {
                        // 处理开始标签 [tag]
                        if (COLOR_MAP.contains(tag_content)) {
                            tag_stack.push(std::string(tag_content));
                            tokens.push_back({true, std::string(COLOR_MAP.at(tag_content))});
                        } else {
                            // 未定义标签，当作文本
                            tokens.push_back({false, std::string(input.substr(i, j - i + 1))});
                        }
                    }
                    i = j + 1;
                    continue;
                }
            }
            
            // 处理普通文本
            size_t next_bracket = input.find('[', i);
            std::string_view text = input.substr(i, next_bracket - i);
            tokens.push_back({false, std::string(text)});
            i = (next_bracket == std::string_view::npos) ? input.length() : next_bracket;
        }

        // 扫尾：如果有未闭合标签，在末尾强制重置（确保终端颜色不溢出）
        if (!tag_stack.empty()) {
            tokens.push_back({true, std::string(RESET)});
        }

        return tokens;
    }

    /**
     * @brief 模板格式化输出，支持 C++ 风格的可变参数
     */
    template<typename... Args>
    static void print_color(std::string_view format_str, Args&&... args) {
        // 先处理 BBCode 转换
        auto tokens = parse(format_str);
        std::string final_fmt;
        for (const auto& t : tokens) {
            final_fmt += t.content;
        }

        // 使用 std::vformat 或简单的 printf 包装
        // 这里为了演示逻辑，直接输出
        try {
            std::string result = std::vformat(final_fmt, std::make_format_args(args...));
            std::cout << result;
        } catch (const std::exception& e) {
            std::cerr << "\n[Format Error]: " << e.what() << std::endl;
        }
    }
};

int main() {
    // 测试 1: 标准嵌套与安全性检查
    // [white] 内嵌套了不匹配的标签或正确闭合
    BBCodeEngine::print_color(
        "[red]Red [white]Text[/white][/red] Normal [blue]{} Text[/blue]\n", 
        "Blue"
    );

    // 测试 2: 非法标签自动跳过（非法 tag [ninja] 会被当做原始文本）
    BBCodeEngine::print_color(
        "[green]Success:[/green] [ninja]Invisible[/ninja] User [yellow]{}[/yellow] logged in.\n", 
        "Admin"
    );

    // 测试 3: 标签未闭合自动保护
    BBCodeEngine::print_color("[magenta]This tag is never closed, but RAII handles it.\n");

    return 0;
}
