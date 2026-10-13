# 1.指定 C++ 标准
cmake_minimum_required(VERSION 3.14)
project(rk_streamer)

# 2.默认使用 C++14 标准
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

# 3.开启编译器警告
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 4.寻找依赖包
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(image_transport REQUIRED)

# 5. 寻找 GStreamer
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0)

# 6.包含 src 目录
include_directories(include src)

# 编译库（组件）
add_library(rk_streamer_component SHARED
  src/rk_streamer.cpp
)

target_include_directories(rk_streamer_component PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${GST_INCLUDE_DIRS}
)

target_link_libraries(rk_streamer_component
  ${GST_LIBRARIES}
)

ament_target_dependencies(rk_streamer_component
  rclcpp
  rclcpp_components
  sensor_msgs
  image_transport
)

# 7.注册组件
# 自动生成 rk_streamer_node 可执行文件
rclcpp_components_register_node(rk_streamer_component
  PLUGIN "RkStreamer"
  EXECUTABLE rk_streamer_node
)

# 8.安装库和可执行文件
install(TARGETS 
  rk_streamer_component 
  rk_streamer_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# 10.安装 launch 文件
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
