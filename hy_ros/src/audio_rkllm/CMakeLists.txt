# 1.新增：指定 C++ 标准为 C++14（必选）
cmake_minimum_required(VERSION 3.14)
project(audio_rkllm)

# 2.新增：默认使用 C++14 标准（必选）
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

# 3.新增：开启编译器警告（可选）
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
  # 3.1 性能优化：Release 模式下启用 O3 优化
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -march=native -DNDEBUG)
  endif()
endif()

# 4.新增：寻找依赖包（必选）
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED) 

# 5.新增：包含 src 目录以支持同目录下的头文件（必选）
include_directories(include src ${OpenCV_INCLUDE_DIRS})

# 6.新增：创建可执行文件：将 .h 文件也加入源文件列表（必选）
add_executable(audio_rkllm_node
  src/audio_rkllm.cpp
  src/audio_rkllm.h
)

# 7.新增：链接 RKLLM 运行时库（可选）
set(RKLLM_LIB_PATH /lib/librkllmrt.so)
target_link_libraries(audio_rkllm_node
  ${RKLLM_LIB_PATH}
  ${OpenCV_LIBS}
  Threads::Threads 
)

# 8.新增：链接依赖库（必选）
ament_target_dependencies(audio_rkllm_node
  rclcpp
  std_msgs
)

# 8.1 性能优化：链接时优化 (LTO) 和移除未使用符号
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set_target_properties(audio_rkllm_node PROPERTIES
    LINK_FLAGS "-Wl,--as-needed"  # 移除未使用的库链接
  )
endif()

# 9.新增：安装可执行文件（必选）
install(TARGETS audio_rkllm_node
  DESTINATION lib/${PROJECT_NAME}
)

# 10. 新增:安装target_link_libraries运行库的.so文件（可选），因为RKLLM_LIB_PATH是系统路径，不需要安装到包目录
#install(FILES ${RKLLM_LIB_PATH}
#  DESTINATION lib/${PROJECT_NAME}
#)

# 11.新增：安装 launch 启动文件目录（必选）
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

# 测试相关配置
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # 跳过版权检查
  set(ament_cmake_copyright_FOUND TRUE)
  # 跳过 cpplint 检查
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
