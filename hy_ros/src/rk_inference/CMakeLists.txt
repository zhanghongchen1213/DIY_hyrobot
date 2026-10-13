# 1.新增：指定 C++ 标准为 C++14（必选）
cmake_minimum_required(VERSION 3.14)
project(rk_inference)

# 2.新增：默认使用 C++14 标准（必选）
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

# 3.新增：开启编译器警告（可选）
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 4.新增：寻找依赖包（必选）
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)

# 5. 寻找 Rockchip 专用库 (RGA & RKNN)
# RGA: 位于 /usr/local/include 和 /usr/local/lib
find_path(RGA_INCLUDE_DIR im2d.h 
  HINTS 
    /usr/include/rga 
    /usr/local/include/rga 
    /usr/include 
    /usr/local/include
)
find_library(RGA_LIBRARY rga 
  HINTS 
    /usr/lib 
    /usr/lib/aarch64-linux-gnu 
    /usr/local/lib
)

# RKNN: 位于 SDK 或系统路径
find_path(RKNN_INCLUDE_DIR rknn_api.h 
  HINTS 
    /usr/include 
    /usr/local/include 
    /usr/include/rknn 
    /usr/local/include/rknn
)
find_library(RKNN_LIBRARY rknnrt 
  HINTS 
    /usr/lib 
    /usr/lib/aarch64-linux-gnu 
    /usr/local/lib
)

if (NOT RGA_INCLUDE_DIR OR NOT RGA_LIBRARY)
  message(WARNING "RGA library not found. Please ensure librga-dev is installed or set RGA_INCLUDE_DIR/RGA_LIBRARY manually.")
endif()

if (NOT RKNN_INCLUDE_DIR OR NOT RKNN_LIBRARY)
  message(WARNING "RKNN library not found. Please ensure rknn-toolkit2/librknnrt is installed.")
endif()

# 6.新增：包含 src 目录以支持同目录下的头文件（必选）
include_directories(
  include
  src
  ${RGA_INCLUDE_DIR}
  ${RKNN_INCLUDE_DIR}
)

# 编译库（组件）
add_library(rk_inference_component SHARED
  src/rk_inference.cpp
)

target_include_directories(rk_inference_component PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${RGA_INCLUDE_DIR}
  ${RKNN_INCLUDE_DIR}
)

target_link_libraries(rk_inference_component
  ${RGA_LIBRARY}
  ${RKNN_LIBRARY}
)

ament_target_dependencies(rk_inference_component
  rclcpp
  rclcpp_components
  sensor_msgs
)

# 7.注册组件
# 这一步会自动生成可执行文件 rk_inference_node
rclcpp_components_register_node(rk_inference_component
  PLUGIN "RkInference"
  EXECUTABLE rk_inference_node
)

# 8.安装库
install(TARGETS rk_inference_component
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# 10.新增：安装 launch 时启动文件目录（必选）
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
