#include <technical_network_bridge.h>

// hearers in stl
#include <stdio.h>
#include <string.h>

technical_network_bridge::technical_network_bridge() {
  message_recieved_ = false;
  nh_.param<std::string>(ros::this_node::getName() + "/team_id", team_id_, "OUXTP");
  nh_.param<std::string>(ros::this_node::getName() + "/ip_address", ip_address_, "127.0.0.1");
  nh_.param<std::string>(ros::this_node::getName() + "/hostname", hostname_, "local");
  nh_.param<bool>(ros::this_node::getName() + "/use_hostname", use_hostname_, false);
  nh_.param<int>(ros::this_node::getName() + "/port", port_, 12345);
  nh_.param<double>(ros::this_node::getName() + "/heartbeat_publish_rate", heartbeat_publish_rate_, 1);
  if(use_hostname_)
  {
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::resolver::query query(hostname_, std::to_string(port_));
    boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
    boost::asio::ip::tcp::endpoint endpoint = iter->endpoint();
    boost::asio::ip::address address = endpoint.address();
    ip_address_ = address.to_string();
  }
  client_ = new tcp_client(io_service_, ip_address_, port_);

  connection_status_pub_ = nh_.advertise<robotx_msgs::TechnicalDirectorNetworkStatus>(
      ros::this_node::getName() + "/connection_status", 1);
  heartbeat_sub_ = nh_.subscribe("/heartbeat", 1, &technical_network_bridge::heartbeat_callback, this);
  entrance_and_exit_gates_report_sub_ = 
    nh_.subscribe("/entrance_and_exit_gates_report",1,&technical_network_bridge::entrance_and_exit_gates_report_callback_,this);
  identify_symbols_and_dock_report_sub_ = 
    nh_.subscribe("/identify_symbols_and_dock_report",1,&technical_network_bridge::identify_symbols_and_dock_report_callback_,this);
  scan_the_code_report_sub_ = 
    nh_.subscribe("/scan_the_code_report",1,&technical_network_bridge::scan_the_code_report_callback_,this);
}

technical_network_bridge::~technical_network_bridge() {}

void technical_network_bridge::run(){
  tcp_thread = boost::thread(&technical_network_bridge::publish_heartbeat_message, this);
  io_service_thread = boost::thread(&technical_network_bridge::run_io_service_, this);
}

void technical_network_bridge::run_io_service_(){
  io_service_.run();
  return;
}

void technical_network_bridge::entrance_and_exit_gates_report_callback_(const robotx_msgs::EntranceAndExitGatesReport::ConstPtr &msg)
{
  std::string tcp_send_msg = "$RXGAT,";
  std::string date_dd,data_mm,data_yy;
  get_local_date_(date_dd,data_mm,data_yy);
  tcp_send_msg = tcp_send_msg + date_dd + data_mm + data_yy + ",";
  std::string hh,mm,ss;
  get_local_time_(hh,mm,ss);
  tcp_send_msg = tcp_send_msg + hh + mm + ss + ",";
  tcp_send_msg = tcp_send_msg + team_id_ + ",";
  // TODO
  return;
}

void technical_network_bridge::identify_symbols_and_dock_report_callback_(const robotx_msgs::IdentifySymbolsAndDockReport::ConstPtr &msg)
{
  std::string tcp_send_msg = "RXDOK,";
  std::string date_dd,data_mm,data_yy;
  get_local_date_(date_dd,data_mm,data_yy);
  tcp_send_msg = tcp_send_msg + date_dd + data_mm + data_yy + ",";
  std::string hh,mm,ss;
  get_local_time_(hh,mm,ss);
  tcp_send_msg = tcp_send_msg + hh + mm + ss + ",";
  tcp_send_msg = tcp_send_msg + team_id_ + ",";
  if(msg->color == msg->RED)
  {
    tcp_send_msg = tcp_send_msg + "R,";
  }
  if(msg->color == msg->BLUE)
  {
    tcp_send_msg = tcp_send_msg + "B,";
  }
  if(msg->color == msg->GREEN)
  {
    tcp_send_msg = tcp_send_msg + "G,";
  }
  if(msg->shape == msg->CRUCIFORM)
  {
    tcp_send_msg = tcp_send_msg + "CRUCI";
  }
  if(msg->shape == msg->TRIANGLE)
  {
    tcp_send_msg = tcp_send_msg + "TRIAN";
  }
  if(msg->shape == msg->CIRCLE)
  {
    tcp_send_msg = tcp_send_msg + "CIRCL";
  }
  tcp_send_msg = "$" + tcp_send_msg + "*" + generate_checksum(tcp_send_msg.c_str()) + "\r\n";
  client_->send(tcp_send_msg);
  ROS_INFO_STREAM("publish Identify Sybmols and Dock message -> " << tcp_send_msg);
  return;
}

void technical_network_bridge::publish_heartbeat_message() {
  ros::Rate loop_rate(heartbeat_publish_rate_);
  while (ros::ok()) {
    publish_connection_status_message();
    mtx_.lock();
    if (message_recieved_ == true) {
      client_->send(heartbeat_tcp_send_msg_);
      ROS_INFO_STREAM("publish Heartbeat Message message -> " << heartbeat_tcp_send_msg_);
    }
    mtx_.unlock();
    loop_rate.sleep();
  }
}

void technical_network_bridge::scan_the_code_report_callback_(const robotx_msgs::ScanTheCodeReport::ConstPtr &msg){
  std::string tcp_send_msg = "RXCOD,";
  std::string date_dd,data_mm,data_yy;
  get_local_date_(date_dd,data_mm,data_yy);
  tcp_send_msg = tcp_send_msg + date_dd + data_mm + data_yy + ",";
  std::string hh,mm,ss;
  get_local_time_(hh,mm,ss);
  tcp_send_msg = tcp_send_msg + hh + mm + ss + ",";
  tcp_send_msg = tcp_send_msg + team_id_ + ",";
  std::string light_pattern_str;
  for(int i=0; i<3; i++)
  {
    if(msg->light_pattern[i] == msg->RED)
    {
      light_pattern_str = light_pattern_str + "R";
    }
    if(msg->light_pattern[i] == msg->BLUE)
    {
      light_pattern_str = light_pattern_str + "B";
    }
    if(msg->light_pattern[i] == msg->GREEN)
    {
      light_pattern_str = light_pattern_str + "G";
    }
  }
  light_pattern_str = "$" + light_pattern_str + "*" + generate_checksum(light_pattern_str.c_str()) + "\r\n";
  return;
}

void technical_network_bridge::heartbeat_callback(const robotx_msgs::Heartbeat::ConstPtr &msg) {
  message_recieved_ = true;
  mtx_.lock();
  heartbeat_msg_ = *msg;
  update_heartbeat_message();
  mtx_.unlock();
}

std::string technical_network_bridge::generate_checksum(const char *data) {
  int crc = 0;
  // the first $ sign and the last two bytes of original CRC + the * sign
  for (int i = 0; i < strlen(data); i++) {
    crc ^= data[i];
  }
  std::stringstream checksum;
  checksum << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << std::right << crc;  // like "0A"
  std::dec;
  return checksum.str();
}

void technical_network_bridge::update_heartbeat_message() {
  heartbeat_tcp_send_msg_ = "RXHRB,";
  std::string date_dd,data_mm,data_yy;
  get_local_date_(date_dd,data_mm,data_yy);
  heartbeat_tcp_send_msg_ = heartbeat_tcp_send_msg_ + date_dd + data_mm + data_yy + ",";
  std::string hh,mm,ss;
  get_local_time_(hh,mm,ss);
  heartbeat_tcp_send_msg_ = heartbeat_tcp_send_msg_ + hh + mm + ss + ",";
  heartbeat_tcp_send_msg_ = heartbeat_tcp_send_msg_ + std::to_string(heartbeat_msg_.latitude) + ",";
  if (heartbeat_msg_.north_or_south == heartbeat_msg_.NORTH) {
    heartbeat_tcp_send_msg_ = heartbeat_tcp_send_msg_ + "N,";
  } else {
    heartbeat_tcp_send_msg_ = heartbeat_tcp_send_msg_ + "S,";
  }
  heartbeat_tcp_send_msg_ = heartbeat_tcp_send_msg_ + std::to_string(heartbeat_msg_.longitude) + ",";
  if (heartbeat_msg_.east_or_west == heartbeat_msg_.EAST) {
    heartbeat_tcp_send_msg_ = heartbeat_tcp_send_msg_ + "E,";
  } else {
    heartbeat_tcp_send_msg_ = heartbeat_tcp_send_msg_ + "W,";
  }
  heartbeat_tcp_send_msg_ = heartbeat_tcp_send_msg_ + team_id_ + ",";
  heartbeat_tcp_send_msg_ = heartbeat_tcp_send_msg_ + std::to_string(heartbeat_msg_.vehicle_mode + 1) + ",";
  heartbeat_tcp_send_msg_ = heartbeat_tcp_send_msg_ + "1";
  std::string checksum = generate_checksum(heartbeat_tcp_send_msg_.c_str());
  heartbeat_tcp_send_msg_ = "$" + heartbeat_tcp_send_msg_ + "*" + checksum + "\r\n";
}

void technical_network_bridge::publish_connection_status_message() {
  bool connection_status = client_->get_connection_status();
  robotx_msgs::TechnicalDirectorNetworkStatus connection_status_msg;
  if (connection_status == true) {
    connection_status_msg.status = connection_status_msg.CONNECTED;
  } else {
    connection_status_msg.status = connection_status_msg.CONNECTION_LOST;
  }
  connection_status_msg.port = port_;
  connection_status_msg.ip_address = ip_address_;
  connection_status_pub_.publish(connection_status_msg);
}

void technical_network_bridge::get_local_date_(std::string& hst_dd, std::string& hst_mm, std::string& hst_yy)
{
  std::time_t t = std::time(nullptr);
  std::tm tm = *std::localtime(&t);
  hst_yy = std::to_string(tm.tm_year+1990).substr(2,3);
  if(tm.tm_mday < 9)
  {
    hst_mm = "0" + std::to_string(tm.tm_mday);
  }
  else
  {
    hst_mm = std::to_string(tm.tm_mday);
  }
  if(tm.tm_mon < 9)
  {
    hst_mm = "0" + std::to_string(tm.tm_mon);
  }
  else
  {
    hst_mm = std::to_string(tm.tm_mon);
  }
}

void technical_network_bridge::get_local_time_(std::string& hst_hh, std::string& hst_mm, std::string& hst_ss)
{
  std::time_t t = std::time(nullptr);
  std::tm tm = *std::localtime(&t);
  if (tm.tm_hour < 9)
    hst_hh = "0" + std::to_string(tm.tm_hour);
  else
    hst_hh = std::to_string(tm.tm_hour);
  if (tm.tm_min < 9)
    hst_mm = "0" + std::to_string(tm.tm_min);
  else
    hst_mm = std::to_string(tm.tm_min);
  if (tm.tm_sec < 9)
    hst_ss = "0" + std::to_string(tm.tm_sec);
  else
    hst_ss = std::to_string(tm.tm_sec);
}

