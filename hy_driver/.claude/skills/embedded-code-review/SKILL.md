---
name: "embedded-code-review"
description: "结合 Context7 知识库，对嵌入式代码进行深度审查。自动识别 MCU/框架，检索最新官方规范，确保代码符合硬件特性和最佳实践。"
---

# 嵌入式代码审查专家 (Embedded Code Reviewer)

本技能专注于嵌入式系统的代码质量控制。它利用 `context7` MCP 工具实时获取 MCU 厂商（如 ST, Espressif, NXP）和软件框架（如 FreeRTOS, Zephyr, LVGL）的最新文档与规范，对项目代码进行深度审计。

## 核心能力

1.  **环境感知**: 自动分析工程文件（`.ioc`, `CMakeLists.txt`, `sdkconfig`），识别目标硬件和软件栈。
2.  **知识增强**: 使用 `context7` 检索特定 MCU/API 的官方最佳实践、Errata（勘误表）和推荐用法。
3.  **深度审计**: 检查中断安全、内存管理、外设配置冲突及 RTOS 竞态条件。
4.  **规范对齐**: 确保代码风格和 API 使用符合厂商推荐标准（例如：STM32 HAL 库的正确回调方式）。

## 执行流程 (Execution Flow)

当用户请求“审查代码”、“检查规范”或“优化驱动”时，按以下步骤执行：

### 1. 环境扫描 (Environment Scan)

遍历项目根目录，通过特征文件确定上下文：

- **STM32**: 检查 `.ioc` 或 `stm32*.h` -> 确定系列 (F1/F4/H7) 和库类型 (HAL/LL)。
- **ESP32**: 检查 `sdkconfig` 或 `idf_component.yml` -> 确定 IDF 版本。
- **RTOS**: 检查 `FreeRTOSConfig.h` 或 `rtthread.h` -> 确定 OS 类型。

### 2. 获取规范 (Retrieve Specs via Context7)

调用 `context7` 工具链获取权威信息：

1.  **解析库 ID**: 使用 `mcp_context7_resolve-library-id`。
    - 示例: `resolve-library-id(query="STM32 HAL library best practices", libraryName="stm32")`
    - 示例: `resolve-library-id(query="FreeRTOS ISR safety", libraryName="freertos")`
2.  **查询文档**: 使用 `mcp_context7_query-docs`。
    - 查询具体模块的规范，如 DMA 使用、中断优先级配置、Flash 读写保护等。

### 3. 代码审计 (Audit Execution)

读取关键源码文件，结合获取的规范进行检查：

- **硬件操作**: 寄存器读写是否使用了正确的位掩码？HAL 库函数是否检查了返回值？
- **中断处理**: ISR 是否过于复杂？是否在 ISR 中调用了不安全的 API（如 `printf` 或非 `FromISR` 的 OS 函数）？
- **资源管理**: 互斥锁 (`Mutex`) 和信号量 (`Semaphore`) 是否成对使用？DMA 缓冲区是否对齐？
- **时序安全**: 是否存在死循环风险？看门狗 (`Watchdog`) 喂狗策略是否合理？

### 4. 生成报告 (Generate Report)

输出一份结构化的审查报告（Markdown 格式），列出所有发现的问题和建议。

### 5. 自动修复 (Auto-Fix)

在生成报告后，根据审查结果自动应用修复：

1.  **确认修复范围**：针对“关键问题 (Critical Issues)”和明确违反规范的“改进建议”，直接进行代码修改。
2.  **执行修改**：使用文件编辑工具（如 `SearchReplace` 或 `Write`）直接更新源文件。
3.  **注释说明**：在修改处添加简短的注释，说明修改原因（例如：`// Fix: Use FromISR API in ISR context`）。
4.  **最终验证**：确保修改后的代码语法正确，且不破坏原有逻辑。

## 输出格式 (Output Format)

````markdown
# 嵌入式代码审查与修复报告

> 目标平台：[MCU 型号]
> 软件框架：[Framework 名称]

## 🚨 关键问题 (Critical Issues)

_（可能导致崩溃、死锁或硬件损坏的问题）_

### 1. [问题标题，如：ISR 中使用了不安全的 OS API]

- **文件位置**: `src/stm32f4xx_it.c:L45`
- **问题描述**: 在 `TIM3_IRQHandler` 中调用了 `xQueueSend` 而非 `xQueueSendFromISR`。
- **官方规范**: 根据 FreeRTOS 文档，ISR 环境必须使用 `FromISR` 后缀的 API。
- **修复建议**:
  ```c
  // 修改前
  xQueueSend(xQueue, &data, 0);
  // 修改后
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(xQueue, &data, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  ```
````

## ⚠️ 改进建议 (Improvements)

_（性能优化、代码风格或潜在风险）_

### 1. [建议标题，如：DMA 缓冲区未对齐]

- **文件位置**: `src/bsp_sdio.c`
- **详情**: SD 卡 DMA 传输缓冲区未按 4 字节对齐，可能导致传输错误或效率低下。
- **参考**: [Context7 提供的相关文档链接或引用]

## ✅ 符合规范 (Good Practices)

- [列出代码中做得好的地方，如：正确使用了 HAL_Lock 机制]

```

## 使用示例

**用户输入**: "帮我检查一下 `motor_driver.c` 里的定时器配置有没有问题。"

**Agent 动作**:
1. 读取 `motor_driver.c`。
2. 识别到使用了 STM32 HAL 库。
3. 调用 `context7` 查询 "STM32 HAL TIM PWM configuration"。
4. 发现代码中手动修改了 `ARR` 寄存器但未调用 `__HAL_TIM_SET_AUTORELOAD` 宏，可能导致阴影寄存器未更新。
5. 生成包含该警告的报告。
```
