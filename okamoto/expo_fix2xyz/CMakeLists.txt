cmake_minimum_required(VERSION 3.0.2)
project(expo_fix2xyz)

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
  geometry_msgs
  expo_msgs
  expo_fix_msgs
  expo_crowd_msgs
  expo
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

add_executable(expo_fix2xyz
  src/fix_xyz_transform.cpp
  src/fix2xyz_node.cpp
  src/fix2xyz_node_lib.cpp
)
add_executable(expo_xyz2fix
  src/fix_xyz_transform.cpp
  src/xyz2fix_node.cpp
  src/xyz2fix_node_lib.cpp
)
add_executable(expo_tftest
  src/tf_test.cpp
)

add_dependencies(expo_fix2xyz
  ${${PROJECT_NAME}_EXPORTED_TARGETS}
  ${catkin_EXPORTED_TARGETS}
)
add_dependencies(expo_xyz2fix
  ${${PROJECT_NAME}_EXPORTED_TARGETS}
  ${catkin_EXPORTED_TARGETS}
)
add_dependencies(expo_tftest
  ${${PROJECT_NAME}_EXPORTED_TARGETS}
  ${catkin_EXPORTED_TARGETS}
)

target_link_libraries(expo_fix2xyz
  ${catkin_LIBRARIES}
  ${PROJ_LIBRARIES}
)
target_link_libraries(expo_xyz2fix
  ${catkin_LIBRARIES}
  ${PROJ_LIBRARIES}
)
target_link_libraries(expo_tftest
  ${catkin_LIBRARIES}
  ${PROJ_LIBRARIES}
)
