// headers in this package
#include <robotx_hardware_interface.h>
#include <robotx_msgs/Heartbeat.h>

// headers in STL
#include <time.h>
#include <cmath>

robotx_hardware_interface::robotx_hardware_interface()
    : params_(robotx_hardware_interface::parameters()),
      remote_operated_if(
          boost::bind(&robotx_hardware_interface::recieve_remote_oprated_motor_command, this, _1)) {
  kill_cmd_flag_ = false;
  right_thrust_cmd_str_pub_ = nh_.advertise<std_msgs::String>("/left_thrust_driver_node/msg", 1);
  left_thrust_cmd_str_pub_ = nh_.advertise<std_msgs::String>("/right_thrust_driver_node/msg", 1);
  heartbeat_pub_ = nh_.advertise<robotx_msgs::Heartbeat>("/heartbeat", 1);
  if (params_.target == params_.ALL || params_.target == params_.SIMULATION) {
    right_thrust_cmd_pub_ = nh_.advertise<std_msgs::Float32>("/right_thrust_cmd", 1);
    left_thrust_cmd_pub_ = nh_.advertise<std_msgs::Float32>("/left_thrust_cmd", 1);
    light_cmd_pub_ = nh_.advertise<std_msgs::String>("/serial_sender_node/msg", 1);;
    left_thrust_joint_pub_ =
        nh_.advertise<std_msgs::Float64>("/left_engine_position_controller/command", 1);
    right_thrust_joint_pub_ =
        nh_.advertise<std_msgs::Float64>("/right_engine_position_controller/command", 1);
    last_motor_cmd_msg_.data.resize(4);
    last_motor_cmd_msg_.data[0] = 0;
    last_motor_cmd_msg_.data[1] = 0;
    last_motor_cmd_msg_.data[2] = 0;
    last_motor_cmd_msg_.data[3] = 0;
    last_manual_motor_cmd_msg_.data.resize(4);
    last_manual_motor_cmd_msg_.data[0] = 0;
    last_manual_motor_cmd_msg_.data[1] = 0;
    last_manual_motor_cmd_msg_.data[2] = 0;
    last_manual_motor_cmd_msg_.data[3] = 0;
  }
  if (params_.target == params_.ALL || params_.target == params_.HARDWARE) {
    left_motor_cmd_client_ptr_ =
        new tcp_client(io_service_, params_.left_motor_ip, params_.left_motor_port, params_.timeout);
    right_motor_cmd_client_ptr_ =
        new tcp_client(io_service_, params_.right_motor_ip, params_.right_motor_port, params_.timeout);
    io_service_thread_ = boost::thread(boost::bind(&boost::asio::io_service::run, &io_service_));
    left_thrust_joint_pub_ =
        nh_.advertise<std_msgs::Float64>("/left_engine_position_controller/command", 1);
    right_thrust_joint_pub_ =
        nh_.advertise<std_msgs::Float64>("/right_engine_position_controller/command", 1);
    last_motor_cmd_msg_.data.resize(4);
    last_motor_cmd_msg_.data[0] = 0;
    last_motor_cmd_msg_.data[1] = 0;
    last_motor_cmd_msg_.data[2] = 0;
    last_motor_cmd_msg_.data[3] = 0;
    last_manual_motor_cmd_msg_.data.resize(4);
    last_manual_motor_cmd_msg_.data[0] = 0;
    last_manual_motor_cmd_msg_.data[1] = 0;
    last_manual_motor_cmd_msg_.data[2] = 0;
    last_manual_motor_cmd_msg_.data[3] = 0;
  }
  thruster_diagnostic_updater_.setHardwareID("thruster");
  thruster_diagnostic_updater_.add("left_connection_status", this,
                                   &robotx_hardware_interface::update_left_thruster_connection_status_);
  thruster_diagnostic_updater_.add("right_connection_status", this,
                                   &robotx_hardware_interface::update_right_thruster_connection_status_);
  current_task_number_ = 0;
  fix_sub_ = nh_.subscribe("/sc30_driver_node/fix", 1, &robotx_hardware_interface::fix_callback_, this);
  motor_command_sub_ =
      nh_.subscribe("/wam_v/motor_command", 1, &robotx_hardware_interface::motor_command_callback_, this);
  control_state_sub_ =
      nh_.subscribe("/robotx_state_machine_node/control_state_machine/current_state", 1, &robotx_hardware_interface::control_state_callback_, this);
  kill_cmd_sub_ = 
      nh_.subscribe("/kill", 1, &robotx_hardware_interface::kill_cmd_callback_, this);
}

void robotx_hardware_interface::run(){
  send_command_thread_ = boost::thread(boost::bind(&robotx_hardware_interface::send_command_, this));
  publish_heartbeat_thread_ =
      boost::thread(boost::bind(&robotx_hardware_interface::publish_heartbeat_, this));
  return;
}

robotx_hardware_interface::~robotx_hardware_interface() {
  io_service_thread_.join();
  send_command_thread_.join();
  publish_heartbeat_thread_.join();
}

void robotx_hardware_interface::current_task_number_callback_(std_msgs::UInt8 msg) {
  current_task_number_ = msg.data;
  return;
}

void robotx_hardware_interface::fix_callback_(sensor_msgs::NavSatFix msg) {
  last_fix_msg_ = msg;
  return;
}

void robotx_hardware_interface::control_state_callback_(robotx_msgs::State msg)
{
  current_control_state_ = msg;
  return;
}

// msg = [left_thruster_cmd left_thruster_joint_angle right_thruster_cmd
// right_thruster_joint_angle]
void robotx_hardware_interface::motor_command_callback_(std_msgs::Float64MultiArray msg) {
  if (msg.data.size() == 4) {
    last_motor_cmd_msg_ = msg;
  }
  return;
}

void robotx_hardware_interface::update_left_thruster_connection_status_(
    diagnostic_updater::DiagnosticStatusWrapper &stat) {
  if (params_.target == params_.SIMULATION) {
    stat.summary(diagnostic_msgs::DiagnosticStatus::OK, "connection success");
    stat.add("connection_status", "OK");
    return;
  }
  if (params_.target == params_.ALL || params_.target == params_.HARDWARE) {
    if (left_motor_cmd_client_ptr_->get_connection_status()) {
      stat.summary(diagnostic_msgs::DiagnosticStatus::OK, "connection success");
      stat.add("connection_status", "OK");
    } else {
      stat.summary(diagnostic_msgs::DiagnosticStatus::ERROR, "connection failed");
      stat.add("connection_status", "Fail");
    }
    return;
  }
  stat.summary(diagnostic_msgs::DiagnosticStatus::ERROR, "control target does not match");
  stat.add("connection_status", "");
}

void robotx_hardware_interface::update_right_thruster_connection_status_(
    diagnostic_updater::DiagnosticStatusWrapper &stat) {
  if (params_.target == params_.SIMULATION) {
    stat.summary(diagnostic_msgs::DiagnosticStatus::OK, "connection success");
    stat.add("connection_status", "OK");
    return;
  }
  if (params_.target == params_.ALL || params_.target == params_.HARDWARE) {
    if (right_motor_cmd_client_ptr_->get_connection_status()) {
      stat.summary(diagnostic_msgs::DiagnosticStatus::OK, "connection success");
      stat.add("connection_status", "OK");
    } else {
      stat.summary(diagnostic_msgs::DiagnosticStatus::ERROR, "connection failed");
      stat.add("connection_status", "Fail");
    }
    return;
  }
  stat.summary(diagnostic_msgs::DiagnosticStatus::ERROR, "control target does not match");
  stat.add("connection_status", "");
}

void robotx_hardware_interface::kill_cmd_callback_(const std_msgs::Empty::ConstPtr msg)
{
  kill_cmd_flag_ = true;
  return;
}

void robotx_hardware_interface::send_command_() {
  ros::Rate rate(params_.frequency);
  while (ros::ok()) {
    thruster_diagnostic_updater_.update();
    mtx_.lock();
    if(kill_cmd_flag_)
    {
      rate.sleep();
      continue;
    }
    if (params_.target == params_.ALL || params_.target == params_.SIMULATION) {
      if (current_control_state_.current_state == "remote_operated") {
        std_msgs::Float32 left_drive_cmd_,right_drive_cmd_;
        left_drive_cmd_.data = last_manual_motor_cmd_msg_.data[0];
        right_drive_cmd_.data = last_manual_motor_cmd_msg_.data[2];
        std_msgs::Float64 left_thrust_joint_cmd_;
        left_thrust_joint_cmd_.data = last_manual_motor_cmd_msg_.data[1];
        std_msgs::Float64 right_thrust_joint_cmd_;
        right_thrust_joint_cmd_.data = last_manual_motor_cmd_msg_.data[3];
        left_thrust_cmd_pub_.publish(left_drive_cmd_);
        right_thrust_cmd_pub_.publish(right_drive_cmd_);
        left_thrust_joint_pub_.publish(left_thrust_joint_cmd_);
        right_thrust_joint_pub_.publish(right_thrust_joint_cmd_);
        std_msgs::String light_cmd;
        light_cmd.data = "r";
        light_cmd_pub_.publish(light_cmd);
        left_thrust_joint_pub_.publish(left_thrust_joint_cmd_);
        right_thrust_joint_pub_.publish(right_thrust_joint_cmd_);
        std_msgs::String left_thrust_cmd_str_,right_thrust_cmd_str_;
        left_thrust_cmd_str_.data = std::to_string(left_thrust_joint_cmd_.data)+"\n";
        right_thrust_cmd_str_.data = std::to_string(right_thrust_joint_cmd_.data)+"\n";
      }
      if (current_control_state_.current_state == "autonomous") {
        std_msgs::Float32 left_drive_cmd_,right_drive_cmd_;
        left_drive_cmd_.data = last_motor_cmd_msg_.data[0];
        right_drive_cmd_.data = last_motor_cmd_msg_.data[2];
        std_msgs::Float64 left_thrust_joint_cmd_;
        left_thrust_joint_cmd_.data = last_motor_cmd_msg_.data[1];
        std_msgs::Float64 right_thrust_joint_cmd_;
        right_thrust_joint_cmd_.data = last_motor_cmd_msg_.data[3];
        left_thrust_cmd_pub_.publish(left_drive_cmd_);
        right_thrust_cmd_pub_.publish(right_drive_cmd_);
        left_thrust_joint_pub_.publish(left_thrust_joint_cmd_);
        right_thrust_joint_pub_.publish(right_thrust_joint_cmd_);
        std_msgs::String light_cmd;
        light_cmd.data = "a";
        light_cmd_pub_.publish(light_cmd);
        left_thrust_joint_pub_.publish(left_thrust_joint_cmd_);
        right_thrust_joint_pub_.publish(right_thrust_joint_cmd_);
        std_msgs::String left_thrust_cmd_str_,right_thrust_cmd_str_;
        left_thrust_cmd_str_.data = std::to_string(left_thrust_joint_cmd_.data)+"\n";
        right_thrust_cmd_str_.data = std::to_string(right_thrust_joint_cmd_.data)+"\n";
      }
    }
    if (params_.target == params_.ALL || params_.target == params_.HARDWARE) {
      if (current_control_state_.current_state == "remote_operated") {
        left_motor_cmd_client_ptr_->send(last_manual_motor_cmd_msg_.data[0]);
        right_motor_cmd_client_ptr_->send(last_manual_motor_cmd_msg_.data[2]);
        std_msgs::Float64 left_thrust_joint_cmd_;
        left_thrust_joint_cmd_.data = last_manual_motor_cmd_msg_.data[1]*M_PI_2;
        std_msgs::Float64 right_thrust_joint_cmd_;
        right_thrust_joint_cmd_.data = last_manual_motor_cmd_msg_.data[3]*M_PI_2;
        left_thrust_joint_pub_.publish(left_thrust_joint_cmd_);
        right_thrust_joint_pub_.publish(right_thrust_joint_cmd_);
        std_msgs::String left_thrust_cmd_str_,right_thrust_cmd_str_;
        left_thrust_cmd_str_.data = std::to_string(left_thrust_joint_cmd_.data)+"\n";
        right_thrust_cmd_str_.data = std::to_string(right_thrust_joint_cmd_.data)+"\n";
      }
      if (current_control_state_.current_state == "autonomous") {
        left_motor_cmd_client_ptr_->send(last_motor_cmd_msg_.data[0]);
        right_motor_cmd_client_ptr_->send(last_motor_cmd_msg_.data[2]);
        std_msgs::Float64 left_thrust_joint_cmd_;
        left_thrust_joint_cmd_.data = last_motor_cmd_msg_.data[1]*M_PI_2;
        std_msgs::Float64 right_thrust_joint_cmd_;
        right_thrust_joint_cmd_.data = last_motor_cmd_msg_.data[3]*M_PI_2;
        left_thrust_joint_pub_.publish(left_thrust_joint_cmd_);
        right_thrust_joint_pub_.publish(right_thrust_joint_cmd_);
        left_thrust_joint_pub_.publish(left_thrust_joint_cmd_);
        right_thrust_joint_pub_.publish(right_thrust_joint_cmd_);
        std_msgs::String left_thrust_cmd_str_,right_thrust_cmd_str_;
        left_thrust_cmd_str_.data = std::to_string(left_thrust_joint_cmd_.data)+"\n";
        right_thrust_cmd_str_.data = std::to_string(right_thrust_joint_cmd_.data)+"\n";
      }
    }
    mtx_.unlock();
    rate.sleep();
  }
}

void robotx_hardware_interface::recieve_remote_oprated_motor_command(std_msgs::Float64MultiArray msg) {
  last_manual_motor_cmd_msg_ = msg;
  return;
}

void robotx_hardware_interface::publish_heartbeat_() {
  ros::Rate rate(1);
  while (ros::ok()) {
    robotx_msgs::Heartbeat heartbeat_msg;
    if (last_fix_msg_.latitude > 0)
      heartbeat_msg.north_or_south = heartbeat_msg.NORTH;
    else
      heartbeat_msg.north_or_south = heartbeat_msg.SOUTH;
    heartbeat_msg.latitude = std::fabs(last_fix_msg_.latitude);
    if (heartbeat_msg.longitude > 0)
      heartbeat_msg.east_or_west = heartbeat_msg.EAST;
    else
      heartbeat_msg.east_or_west = heartbeat_msg.WEST;
    heartbeat_msg.longitude = std::fabs(last_fix_msg_.longitude);
    if (current_control_state_.current_state == "remote_operated") heartbeat_msg.vehicle_mode = heartbeat_msg.REMOTE_OPERATED;
    if (current_control_state_.current_state == "autonomous") heartbeat_msg.vehicle_mode = heartbeat_msg.AUTONOMOUS;
    if (current_control_state_.current_state == "emergency") heartbeat_msg.vehicle_mode = heartbeat_msg.EMERGENCY;
    heartbeat_msg.current_task_number = current_task_number_;
    heartbeat_pub_.publish(heartbeat_msg);
    rate.sleep();
  }
}