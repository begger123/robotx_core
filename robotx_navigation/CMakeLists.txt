cmake_minimum_required(VERSION 2.8.3)
project(robotx_navigation)

#include(../../Doxygen/doxygen.cmake)
#include(../../Format/ClangFormat.cmake)

## Compile as C++11, supported in ROS Kinetic and newer
add_compile_options(-std=c++11)

find_package(catkin REQUIRED COMPONENTS
  roscpp
  rospy
  std_msgs
  sensor_msgs
  geometry_msgs
  visualization_msgs
  robotx_msgs
  tf
  tf2_ros
  tf2_geometry_msgs
  nav_msgs
  jsk_recognition_msgs
  #dwa_local_planner
  #global_planner
  yaml_cpp_0_3
  pcl_ros
  message_filters
  robotx_build_flags
  geodesy
)

catkin_package()

###########
## Build ##
###########
include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)

#add_executable(twist_from_fix_velocity_node src/twist_from_fix_velocity_node.cpp src/twist_calculator.cpp)
#target_link_libraries(twist_from_fix_velocity_node ${catkin_LIBRARIES})
#add_dependencies(twist_from_fix_velocity_node robotx_msgs_generate_messages_cpp)
#add_document(twist_from_fix_velocity_node)
#clang_format(twist_from_fix_velocity_node)

#add_executable(geographic_map_server_node src/geographic_map_server_node.cpp src/geographic_map_server.cpp)
#target_link_libraries(geographic_map_server_node ${catkin_LIBRARIES} -lyaml-cpp)
#add_dependencies(geographic_map_server_node robotx_msgs_generate_messages_cpp)
#add_document(geographic_map_server_node)
#clang_format(geographic_map_server_node)

add_executable(robotx_localization_node src/robotx_localization_node.cpp src/robotx_localization.cpp src/particle_filter.cpp)
target_link_libraries(robotx_localization_node ${catkin_LIBRARIES})
add_dependencies(robotx_localization_node robotx_msgs_generate_messages_cpp)
#add_document(robotx_localization_node)
#clang_format(robotx_localization_node)

add_executable(obstacle_map_server_node src/obstacle_map_server_node.cpp src/obstacle_map_server.cpp)
target_link_libraries(obstacle_map_server_node ${catkin_LIBRARIES})
add_dependencies(obstacle_map_server_node robotx_msgs_generate_messages_cpp)
#add_document(obstacle_map_server_node)
#clang_format(obstacle_map_server_node)

#add_executable(costmap_clear_request_sender_node src/costmap_clear_request_sender_node.cpp src/costmap_clear_request_sender.cpp)
#target_link_libraries(costmap_clear_request_sender_node ${catkin_LIBRARIES})
#add_document(costmap_clear_request_sender_node)
#clang_format(costmap_clear_request_sender_node)

#add_executable(ndt_mapping_node src/ndt_mapping_node.cpp src/ndt_mapping.cpp)
#target_link_libraries(ndt_mapping_node ${catkin_LIBRARIES})
#add_document(ndt_mapping_node)
#clang_format(ndt_mapping_node)

add_executable(carrot_planner_node src/carrot_planner_node.cpp src/carrot_planner.cpp)
target_link_libraries(carrot_planner_node ${catkin_LIBRARIES})
add_dependencies(carrot_planner_node robotx_msgs_generate_messages_cpp)

add_executable(waypoint_server_node src/waypoint_server_node.cpp src/waypoint_server.cpp)
target_link_libraries(waypoint_server_node ${catkin_LIBRARIES})
add_dependencies(waypoint_server_node robotx_msgs_generate_messages_cpp)

#add_executable(waypoint_logger_node src/waypoint_logger_node.cpp src/waypoint_logger.cpp)
#target_link_libraries(waypoint_logger_node ${catkin_LIBRARIES})
#add_dependencies(waypoint_logger_node robotx_msgs_generate_messages_cpp)

add_executable(obstacle_avoid_node src/obstacle_avoid_node.cpp src/obstacle_avoid.cpp src/state_lattice_planner.cpp)
target_link_libraries(obstacle_avoid_node ${catkin_LIBRARIES})
add_dependencies(obstacle_avoid_node robotx_msgs_generate_messages_cpp)

add_executable(world_pose_publisher_node src/world_pose_publisher_node.cpp src/world_pose_publisher.cpp)
target_link_libraries(world_pose_publisher_node ${catkin_LIBRARIES})
add_dependencies(world_pose_publisher_node robotx_msgs_generate_messages_cpp)

add_executable(coast_line_publisher_node src/coast_line_publisher_node.cpp src/coast_line_publisher.cpp)
target_link_libraries(coast_line_publisher_node ${catkin_LIBRARIES})
add_dependencies(coast_line_publisher_node robotx_msgs_generate_messages_cpp)

add_executable(field_map_server_node src/field_map_server.cpp src/field_map_server_node.cpp)
target_link_libraries(field_map_server_node ${catkin_LIBRARIES})
add_dependencies(field_map_server_node robotx_msgs_generate_messages_cpp)

add_executable(imu_gravity_removal_node src/imu_gravity_removal.cpp src/imu_gravity_removal_node.cpp)
target_link_libraries(imu_gravity_removal_node ${catkin_LIBRARIES})

#############
## Install ##
#############
# CPP codes
set(NODE_FILES 
  obstacle_avoid_node
  #waypoint_logger_node
  #twist_from_fix_velocity_node 
  #geographic_map_server_node 
  obstacle_map_server_node 
  robotx_localization_node
  field_map_server_node
  #costmap_clear_request_sender_node 
  carrot_planner_node
  waypoint_server_node
  world_pose_publisher_node
  coast_line_publisher_node
  #ndt_mapping_node
  )
foreach(nodes ${NODE_FILES})
  install(TARGETS ${nodes}
    RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
    )
endforeach(nodes)
install(DIRECTORY include/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
  )

# Python scripts
set(PYTHON_SCRPTS 
  scripts/joy_to_cmd.py scripts/motor_command_sender.py scripts/test_cmd_vel_publisher.py scripts/world_frame_publisher.py)
foreach(scripts ${PYTHON_SCRIPTS})
  catkin_install_python(PROGRAMS ${scripts}
    DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
endforeach(scripts)

# Install all files
foreach(dir launch config data)
  install(DIRECTORY ${dir}/
    DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/${dir})
endforeach(dir)

#############
## Testing ##
#############
if (CATKIN_ENABLE_TESTING)
  find_package(roslaunch REQUIRED)
  roslaunch_add_file_check(launch)
endif()
