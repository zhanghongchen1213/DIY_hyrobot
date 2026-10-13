# 1.新增：指定 C++ 标准为 C++14（必选）
cmake_minimum_required(VERSION 3.14)
project(rk_camera)

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
find_package(cv_bridge REQUIRED)
find_package(image_transport REQUIRED)

# 5. 寻找 GStreamer
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0)

# 寻找 Rockchip 专用库 (RGA)
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
if (NOT RGA_INCLUDE_DIR OR NOT RGA_LIBRARY)
  message(WARNING "RGA library not found. Please ensure librga-dev is installed.")
endif()

# 6.新增：包含 src 目录以支持同目录下的头文件（必选）
include_directories(include src ${RGA_INCLUDE_DIR})

# 编译库（组件）
add_library(rk_camera_component SHARED
  src/rk_camera.cpp
)
target_include_directories(rk_camera_component PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${GST_INCLUDE_DIRS}
  ${RGA_INCLUDE_DIR}
)
target_link_libraries(rk_camera_component
  ${GST_LIBRARIES}
  ${RGA_LIBRARY}
)

ament_target_dependencies(rk_camera_component
  rclcpp
  rclcpp_components
  sensor_msgs
  cv_bridge
  image_transport
)

# 7.注册组件
rclcpp_components_register_node(rk_camera_component
  PLUGIN "RkCamera"
  EXECUTABLE rk_camera_node
)

# 8.安装库
install(TARGETS rk_camera_component
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# 9.安装头文件（如果有）
# install(DIRECTORY include/
#   DESTINATION include
# )

# 10.新增：安装 launch 时启动文件目录（必选）
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
