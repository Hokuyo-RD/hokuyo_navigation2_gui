cmake_minimum_required(VERSION 3.0.2)
project(expo_fix_msgs)

find_package(catkin REQUIRED COMPONENTS
roscpp
  geometry_msgs
  message_generation
  message_runtime
  std_msgs
  nav_msgs
  sensor_msgs
)


## Generate messages in the 'msg' folder
add_message_files(
  FILES
  PersonFix.msg
  AreaFix.msg
  FixWithOrientation.msg
)

## Generate added messages and services with any dependencies listed here
generate_messages(
  DEPENDENCIES
  geometry_msgs
  std_msgs
  sensor_msgs
)

catkin_package(
#  INCLUDE_DIRS include
#  LIBRARIES expo_fix_msgs
# CATKIN_DEPENDS message_runtime # geometry_msgs message_generation  std_msgs
#  DEPENDS system_lib
)

include_directories(
# include
  ${catkin_INCLUDE_DIRS}
)

add_executable(fix_odom_to_fix_orient
  src/fix_odom_to_fix_orient.cpp
)

add_dependencies(fix_odom_to_fix_orient
  ${${PROJECT_NAME}_EXPORTED_TARGETS}
  ${catkin_EXPORTED_TARGETS}
)

target_link_libraries(fix_odom_to_fix_orient
  ${catkin_LIBRARIES}
)