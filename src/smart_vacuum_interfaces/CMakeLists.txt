cmake_minimum_required(VERSION 3.8)
project(smart_vacuum_interfaces)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/VacuumStatus.msg"
  "srv/SetFloorLevel.srv"
  "action/CleanSession.action"
)

ament_package()
