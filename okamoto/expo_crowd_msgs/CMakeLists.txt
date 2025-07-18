cmake_minimum_required(VERSION 3.0.2)
project(expo_crowd_msgs)


find_package(catkin REQUIRED COMPONENTS
  geometry_msgs
  message_generation
  message_runtime
  std_msgs
)

add_message_files(
  FILES
  PersonArrow.msg
  Crowd.msg
  PersonArrowFix.msg
  CrowdFix.msg
  PersonArrowEX.msg
  CrowdEX.msg
  PersonArrowFixEX.msg
  CrowdFixEX.msg
  LostItem.msg
  LostItemFix.msg
  Losts.msg
  LostsFix.msg
)

generate_messages(
  DEPENDENCIES
  geometry_msgs
  std_msgs
)

catkin_package(
)

include_directories(
# include
  ${catkin_INCLUDE_DIRS}
)