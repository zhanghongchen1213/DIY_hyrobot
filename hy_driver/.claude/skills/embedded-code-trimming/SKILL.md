---
name: "embedded-code-trimming"
description: "分析嵌入式代码冗余模块（未使用驱动/函数），优化编译选项，生成建议文档并自动应用裁剪修复。当用户要求裁剪代码、减小体积或优化编译配置时调用。"
---

# 嵌入式代码裁剪与优化 (Embedded Code Trimming & Optimization)

本技能专注于分析嵌入式项目（C/C++）中的冗余代码和编译配置，通过移除未使用的模块、驱动、函数以及优化编译器标志，来减小固件体积（Flash/RAM Usage）并提升构建效率。

## 目标 (Goals)

1.  **冗余分析 (Redundancy Analysis)**：扫描项目中未被引用的函数、变量、宏以及整个驱动模块。
2.  **编译优化 (Build Optimization)**：检查 `Makefile`, `CMakeLists.txt`, `Kconfig` 等构建文件，推荐最佳的体积优化选项（如 `-Os`, LTO, Newlib-nano）。
3.  **建议报告 (Recommendation Report)**：生成一份详细的《代码裁剪与优化建议书》。
4.  **自动执行 (Auto-Execution)**：根据分析结果，自动移除（或注释）冗余代码，并修改构建配置。

## 执行流程 (Execution Flow)

当用户请求“裁剪代码”、“优化固件体积”或“移除未使用模块”时：

### 1. 项目扫描与依赖分析 (Scan & Analyze)

- **构建系统检查**：
  - 检查是否启用了死代码消除选项：GCC 的 `-ffunction-sections`, `-fdata-sections` 和链接器的 `-Wl,--gc-sections`。
  - 检查是否使用了轻量级 C 库（如 `--specs=nano.specs`）。
  - 对于 C++ 项目，检查是否禁用了异常 (`-fno-exceptions`) 和 RTTI (`-fno-rtti`)。
- **代码特征分析**：
  - **符号引用**：从 `main` 函数和中断向量表 (ISR) 开始，建立函数调用图，识别未被访问的源文件（如 `drivers/` 目录下未被引用的 `.c` 文件）和静态函数。
  - **高开销函数**：检查是否使用了 `printf` 浮点格式化（`%f`），这通常会引入巨大的库依赖。
  - **数据存储**：检查大的常量表是否加上了 `const` 修饰符（确保放入 Flash 而非 RAM）。

### 2. 生成优化建议 (Generate Recommendations)

- 输出一份 Markdown 格式的报告，包含：
  - **建议移除的模块**：列出完全未使用的驱动文件。
  - **建议裁剪的函数**：列出大体积但未使用的函数。
  - **编译选项建议**：
    - 推荐 `-Os` (Size optimization)。
    - 推荐 `-flto` (Link Time Optimization)。
    - 推荐 `--specs=nano.specs` (Newlib-nano) 和 `--specs=nosys.specs`。
    - C++ 项目推荐禁用 Exception 和 RTTI。
  - **代码修改建议**：如将 `printf` 替换为轻量级实现，或去除浮点打印。
  - **预计收益**：估算可节省的 Flash/RAM 空间。

### 3. 执行裁剪与修复 (Execute & Fix)

- **代码清理**：
  - 删除或注释掉未使用的函数定义。
  - 从构建脚本（如 `CMakeLists.txt`）中移除未使用的源文件路径。
  - 使用 `#if 0 ... #endif` 屏蔽未使用的代码块（比直接删除更安全）。
- **配置更新**：
  - 修改构建文件，注入优化标志（如 `-Os`, `-Wl,--gc-sections`, `--specs=nano.specs`）。
  - 如果用户确认，移除 `-u _printf_float` 链接选项。

## 交互示例 (Example)

**用户输入**：

> "分析一下这个 STM32 项目，把没用的驱动删了，优化一下体积。"

**模型响应**：

1.  **分析**：发现 `drivers/i2c_sensor.c` 未被引用；`CMakeLists.txt` 使用了 `-O0` 且未启用 `nano.specs`；发现使用了 `std::vector` 但未禁用异常。
2.  **报告**：
    > ### 优化建议
    >
    > 1. **移除模块**：`i2c_sensor.c` (未使用)，节省约 2KB。
    > 2. **编译选项**：
    >    - 将 `-O0` 改为 `-Os`。
    >    - 启用 `--specs=nano.specs` (预计节省 10KB+)。
    >    - 添加 `-fno-exceptions -fno-rtti` (C++ 优化)。
3.  **执行**：
    - 在 `CMakeLists.txt` 中注释掉 `i2c_sensor.c`。
    - 修改编译选项，添加上述 Flags。

## 注意事项 (Notes)

- **Newlib-nano 限制**：启用 `nano.specs` 后，默认不支持 `printf` 打印浮点数。如需打印浮点且空间允许，需显式添加 `-u _printf_float`（但这会增加约 10KB 体积）。
- **链接器行为**：某些看似未使用的函数可能是通过函数指针或特定 section 调用的（如初始化列表），删除前需谨慎确认。
- **Map 文件分析**：建议添加 `-Wl,-Map=output.map,--print-memory-usage` 选项，以便生成详细的内存映射文件供后续深度分析。
- **备份**：在进行破坏性裁剪（删除文件）前，建议使用 Git 提交当前状态。
