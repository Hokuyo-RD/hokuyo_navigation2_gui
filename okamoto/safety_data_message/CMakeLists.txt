cmake_minimum_required(VERSION 3.0.2)
project(safety_data_message)

find_package(catkin REQUIRED COMPONENTS
  geometry_msgs
  message_generation
  message_runtime
  std_msgs
  sensor_msgs
)

add_message_files(
  FILES
  SafetyData.msg
)

generate_messages(
  DEPENDENCIES
  geometry_msgs
  std_msgs
  sensor_msgs
)

catkin_package(
)

include_directories(
  ${catkin_INCLUDE_DIRS}
)

