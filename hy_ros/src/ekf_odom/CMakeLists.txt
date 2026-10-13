# 1.新增：指定 C++ 标准为 C++14（必选）
cmake_minimum_required(VERSION 3.14)
project(ekf_odom)

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

# 5.新增：安装 launch 启动文件目录（必选）
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

# 6.新增：安装 config 配置文件目录（必选）
install(DIRECTORY config
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
