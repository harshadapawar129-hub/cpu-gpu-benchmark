cmake_minimum_required(VERSION 3.8)
project(cpu_gpu_benchmark)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(benchmark_node src/benchmark_node.cpp)

ament_target_dependencies(benchmark_node
  rclcpp
  sensor_msgs
  cv_bridge
)

target_include_directories(benchmark_node PRIVATE
  ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(benchmark_node
  ${OpenCV_LIBS}
)

install(TARGETS
  benchmark_node
  DESTINATION lib/${PROJECT_NAME}
)

install(PROGRAMS
  src/image_publisher.py
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
