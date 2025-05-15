cmake_minimum_required(VERSION 3.0.2)
project(expo_safety_manage)

find_package(catkin REQUIRED COMPONENTS
  nav_msgs
  roscpp
  rospy
  sensor_msgs
  std_msgs
  geometry_msgs
  safety_data_message
)

add_message_files(
  FILES
  SensorsState.msg
)

generate_messages(
  DEPENDENCIES
  std_msgs
)
catkin_package(
#  INCLUDE_DIRS include
#  LIBRARIES fusion_tools
#  CATKIN_DEPENDS nav_msgs roscpp rospy sensor_msgs std_msgs tf tf2 tf2_ros tf2_geometry_msgs
#  DEPENDS system_lib
)

include_directories(
  "${PROJECT_SOURCE_DIR}/include"
  ${catkin_INCLUDE_DIRS}
)


add_executable(expo_cmdvel_manager
  src/expo_cmdvel_manager.cpp
)
add_executable(expo_sensor_checker
  src/expo_sensor_checker.cpp
)

add_dependencies(expo_cmdvel_manager
  ${${PROJECT_NAME}_EXPORTED_TARGETS}
  ${catkin_EXPORTED_TARGETS}
)
add_dependencies(expo_sensor_checker
  ${${PROJECT_NAME}_EXPORTED_TARGETS}
  ${catkin_EXPORTED_TARGETS}
)

target_link_libraries(expo_cmdvel_manager
  ${catkin_LIBRARIES}
)
target_link_libraries(expo_sensor_checker
  ${catkin_LIBRARIES}
)
