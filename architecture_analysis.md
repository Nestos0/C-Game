# Project PolarRegions 架构分析文档

## 项目概述

Project PolarRegions 是一个基于终端的应用/游戏框架，使用C语言开发。项目采用模块化架构，包含以下主要组件：

- **核心引擎 (core/engine)**: 游戏主循环和状态管理
- **渲染系统 (render/renderer)**: 渲染相关功能
- **用户界面 (ui)**: 屏幕管理、终端控制、窗口组件
- **文本处理 (text)**: UTF-8和富文本处理
- **模块系统 (module)**: 初始化和清理的模块化支持

## 全局变量作用域分析

### 1. `game_state` - 游戏状态
**定义位置**: `main.c:14`
```c
struct GameState game_state;
```

**作用**: 
- `is_running`: 控制游戏主循环的运行状态
- 在 `engine.c:43` 中被检查以决定是否继续游戏循环

**作用域**: ✅ 正确
- 定义在 `main.c` 中，可在整个项目中通过 `extern` 访问
- 只有一个全局实例，避免多重定义

### 2. `screen` - 全局屏幕指针
**声明位置**: `display.h:24`
```c
extern struct Screen *screen;
```

**定义位置**: `screen.c:32`
```c
screen = screen_create(width, height);
```

**问题**: ⚠️ 作用域冲突
- 在 `engine.c:7` 中也声明了同名的局部变量：
```c
Screen *screen;  // 与全局 screen 冲突！
```

**建议**: 
- 移除 `engine.c` 中的局部 `screen` 声明，使用全局 `screen`
- 或重命名局部变量以避免冲突

### 3. 模块系统全局变量
**声明位置**: `module.h:3-6`
```c
extern initcall_t __start_initcalls[];
extern initcall_t __stop_initcalls[];
extern initcall_t __start_exitcalls[];
extern initcall_t __stop_exitcalls[];
```

**使用位置**: 
- `module.c:7-8` 和 `19-20` 在初始化和清理时遍历这些数组
- 通过 `APP_INIT()` 和 `APP_EXIT()` 宏注册模块

**作用域**: ✅ 正确
- 这些变量通过链接器脚本或编译器属性定义，作用域正确

### 4. 其他全局变量
- `raw_mode_active`, `oldt`, `newt` (在 `terminal.c:7-8`): 终端原始终端模式的本地状态，作用域正确

## 项目架构图

```
main.c (入口)
├── 初始化所有模块
├── 设置 game_state
├── 进入 game_loop
│   ├── screen_clear()
│   ├── 渲染循环
│   └── 检查 game_state.is_running
└── 清理所有模块

模块系统:
├── __start_initcalls[] ──→ 各模块的 __module_init 函数
└── __start_exitcalls[] ──→ 各模块的 __module_exit 函数
```

## 修正建议

1. **修复 `screen` 作用域冲突**:
   - 删除 `engine.c:7` 中的 `Screen *screen;` 声明
   - 在 `engine.c` 中直接使用全局的 `screen` 指针

2. **代码规范**:
   - 确保全局变量命名一致，避免同名冲突
   - 添加注释说明每个全局变量的用途和作用域

3. **模块化改进**:
   - 考虑使用更明确的命名空间或结构体封装全局状态
   - 为每个模块提供单独的状态管理

这个项目采用模块化设计是很好的实践，但需要注意全局变量的作用域管理，避免同名冲突。修复 `screen` 的作用域问题将提高代码的可维护性和正确性。