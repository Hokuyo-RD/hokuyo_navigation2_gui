cmake_minimum_required(VERSION 3.0.2)
project(expo_latlon2xyz)

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
  expo_msgs
  expo_fix_msgs
)

find_package (Eigen3 REQUIRED)
find_package(PROJ REQUIRED)

catkin_package(
#  INCLUDE_DIRS include
#  LIBRARIES fusion_tools
#  CATKIN_DEPENDS nav_msgs roscpp rospy sensor_msgs std_msgs tf tf2 tf2_ros tf2_geometry_msgs
#  DEPENDS system_lib
)

include_directories(
  "${PROJECT_SOURCE_DIR}/include"
  ${catkin_INCLUDE_DIRS}
  ${PROJ_INCLUDE_DIRS}
)

add_executable(expo_latlon2xyz
  src/latlon_utm_transform.cpp
  src/latlon_utm_transform_node.cpp
  src/latlon_utm_transform_node_lib.cpp
)

add_dependencies(expo_latlon2xyz
  ${${PROJECT_NAME}_EXPORTED_TARGETS}
  ${catkin_EXPORTED_TARGETS}
)

target_link_libraries(expo_latlon2xyz
  ${catkin_LIBRARIES}
  ${PROJ_LIBRARIES}
)
