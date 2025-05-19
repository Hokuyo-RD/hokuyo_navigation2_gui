cmake_minimum_required(VERSION 3.0.2)
project(safety_urg)

find_package(catkin REQUIRED COMPONENTS
  nav_msgs
  roscpp
  rospy
  sensor_msgs
  std_msgs
  tf
  tf2
  tf2_ros
  tf2_geometry_msgs
  safety_data_message
  geometry_msgs
)

catkin_package(
#  INCLUDE_DIRS include
#  LIBRARIES fusion_tools
#  CATKIN_DEPENDS nav_msgs roscpp rospy sensor_msgs std_msgs tf tf2 tf2_ros tf2_geometry_msgs
#  DEPENDS system_lib
)

### safety_library_safetyCMD
set(URG_LIBRARY_DIR safety_library_safetyCMD)
set(URG_LIBRARY_C_INCLUDE_DIR ${URG_LIBRARY_DIR}/include/c)
set(URG_LIBRARY_SRC_DIR ${URG_LIBRARY_DIR}/src)

include_directories(include ${URG_LIBRARY_C_INCLUDE_DIR})

add_library(urg_c SHARED
  ${URG_LIBRARY_SRC_DIR}/safety_crc.c
  ${URG_LIBRARY_SRC_DIR}/safety_sensor_safetyCMD.c
  ${URG_LIBRARY_SRC_DIR}/urg_connection.c
  ${URG_LIBRARY_SRC_DIR}/urg_debug.c
  ${URG_LIBRARY_SRC_DIR}/urg_ring_buffer.c
  ${URG_LIBRARY_SRC_DIR}/urg_sensor.c
  ${URG_LIBRARY_SRC_DIR}/urg_serial.c
  ${URG_LIBRARY_SRC_DIR}/urg_serial_utils.c
  ${URG_LIBRARY_SRC_DIR}/urg_tcpclient.c
  ${URG_LIBRARY_SRC_DIR}/urg_utils.c
)

###

include_directories(
  "${PROJECT_SOURCE_DIR}/include"
  ${catkin_INCLUDE_DIRS}
  ${PROJ_INCLUDE_DIRS}
)

add_executable(safety_urg
  src/safety_urg_node.cpp
  src/safety_urg_node_lib.cpp
)

add_dependencies(safety_urg
  ${${PROJECT_NAME}_EXPORTED_TARGETS}
  ${catkin_EXPORTED_TARGETS}
)

target_link_libraries(safety_urg urg_c
  ${catkin_LIBRARIES}
  ${PROJ_LIBRARIES}
)

