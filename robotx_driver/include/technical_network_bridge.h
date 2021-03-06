#ifndef HEARTBEAT_PUBLISHER_H_INCLUDED
#define HEARTBEAT_PUBLISHER_H_INCLUDED

// headers in this package
#include <robotx_msgs/Heartbeat.h>
#include <robotx_msgs/ScanTheCodeReport.h>
#include <robotx_msgs/EntranceAndExitGatesReport.h>
#include <robotx_msgs/IdentifySymbolsAndDockReport.h>
#include <robotx_msgs/TechnicalDirectorNetworkStatus.h>
#include <tcp_client.h>

// headers in ros
#include <ros/ros.h>

// headers in boost
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

// headers in stl
#include <mutex>

/**
 * @brief technical_network_bridge class
 *
 */
class technical_network_bridge {
 public:
  /**
   * @brief Construct a new heartbeat publisher object
   *
   */
  technical_network_bridge();
  /**
   * @brief Destroy the heartbeat publisher object
   *
   */
  ~technical_network_bridge();
  void run();
 private:
  /**
   * @brief boost::thread object for TCP/IP thread.
   *
   */
  boost::thread tcp_thread;
  boost::thread io_service_thread;
  /**
   * @brief callback function for subscribing ~/heartbeat ROS topic.
   *
   * @param msg content for the message.
   */
  void heartbeat_callback(const robotx_msgs::Heartbeat::ConstPtr &msg);
  void entrance_and_exit_gates_report_callback_(const robotx_msgs::EntranceAndExitGatesReport::ConstPtr &msg);
  void identify_symbols_and_dock_report_callback_(const robotx_msgs::IdentifySymbolsAndDockReport::ConstPtr &msg);
  void scan_the_code_report_callback_(const robotx_msgs::ScanTheCodeReport::ConstPtr &msg);
  void run_io_service_();
  /**
   * @brief function for publishing ~/connection_status ROS topic.
   *
   */
  void publish_connection_status_message();
  /**
   * @brief function for sending message.
   *
   */
  void publish_heartbeat_message();
  /**
   * @brief generaging heartbeat message
   *
   */
  void update_heartbeat_message();
  /**
   * @brief calculating checksum
   *
   * @sa https://gist.github.com/DevNaga/fce8e166f4335fa777650493cb9246db
   *
   * @param data string data for the message.
   * @return std::string cakculating result of the checksum.
   */
  std::string generate_checksum(const char *data);
  /**
   * @brief node handle for the ROS nodes.
   *
   */
  ros::NodeHandle nh_;
  // members for subscription
  /**
   * @brief ROS subscriber for ~heartbeat topic
   *
   */
  ros::Subscriber heartbeat_sub_;
  /**
   * @brief parameter for heartbeat message data.
   *
   */
  robotx_msgs::Heartbeat heartbeat_msg_;
  /**
   * @brief parameter for message which send through TCP/IP transfer.
   *
   */
  std::string heartbeat_tcp_send_msg_;
  /**
   * @brief mutex for exclusion control
   *
   */
  std::mutex mtx_;
  // members for publish connection status
  /**
   * @brief publisher for ~/connection_status ROS topic.
   *
   */
  ros::Publisher connection_status_pub_;
  // parametes for heartbeat connection
  /**
   * @brief TCP/IP client class
   *
   */
  tcp_client *client_;
  /**
   * @brief io_service for tcp_ip client
   * @sa technical_network_bridge::client_
   */
  boost::asio::io_service io_service_;
  /**
   * @brief target TCP/IP server IP address.
   * @sa technical_network_bridge::client_
   */
  std::string ip_address_;
  /**
   * @brief target TCP/IP server network port
   * @sa technical_network_bridge::client_
   */
  int port_;
  /**
   * @brief message publish rate of TCP/IP client.
   *
   */
  double heartbeat_publish_rate_;
  // flags
  /**
   * @brief parameter for checking message recieved.
   *
   */
  volatile bool message_recieved_;

  void get_local_time_(std::string& hst_hh, std::string& hst_mm, std::string& hst_ss);
  void get_local_date_(std::string& hst_dd, std::string& hst_mm, std::string& hst_yy);
  std::string team_id_;
  std::string hostname_;
  bool use_hostname_;
  ros::Subscriber entrance_and_exit_gates_report_sub_;
  ros::Subscriber identify_symbols_and_dock_report_sub_;
  ros::Subscriber scan_the_code_report_sub_;
};

#endif  // HEARTBEAT_PUBLISHER_H_INCLUDED
