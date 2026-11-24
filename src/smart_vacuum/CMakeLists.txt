cmake_minimum_required(VERSION 3.8)
project(smart_vacuum)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(std_msgs REQUIRED)
find_package(smart_vacuum_interfaces REQUIRED)

add_executable(hardware_driver src/hardware_driver.cpp)
ament_target_dependencies(hardware_driver
  rclcpp smart_vacuum_interfaces)

add_executable(map_loader src/map_loader.cpp)
ament_target_dependencies(map_loader
  rclcpp std_msgs smart_vacuum_interfaces)

add_executable(task_manager src/task_manager.cpp)
ament_target_dependencies(task_manager
  rclcpp rclcpp_action std_msgs smart_vacuum_interfaces)

install(TARGETS
  hardware_driver
  map_loader
  task_manager
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
