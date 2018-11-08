#ifndef ROBOXT_NAVIGATION_PLANNER_H_INCLUDED
#define ROBOXT_NAVIGATION_PLANNER_H_INCLUDED

//headers in ROS
#include <ros/ros.h>
#include <geometry_msgs/Twist.h>

//headers in this package
#include <robotx_msgs/State.h>
#include <robotx_msgs/Event.h>

//headers in boost
#include <boost/optional.hpp>

//headers in STL
#include <mutex>

class robotx_navigation_planner
{
public:
    robotx_navigation_planner();
    ~robotx_navigation_planner();
    void run();
private:
    void publish_cmd_vel_();
    void current_state_callback_(const robotx_msgs::StateConstPtr msg);
    void waypoint_planner_cmd_callback_(const geometry_msgs::Twist msg);
    void obstacle_avoid_cmd_callback_(const geometry_msgs::Twist msg);
    ros::NodeHandle nh_;
    ros::Publisher trigger_event_pub_;
    ros::Publisher cmd_vel_pub_;
    ros::Subscriber current_state_sub_;
    ros::Subscriber waypoint_planner_cmd_sub_;
    ros::Subscriber obstacle_avoid_cmd_sub_;
    std::string current_state_topic_;
    std::string trigger_event_topic_;
    std::string waypoint_planner_cmd_topic_;
    std::string obstacle_avoid_cmd_topic_;
    double publish_rate_;
    boost::optional<geometry_msgs::Twist> waypoint_planner_cmd_;
    boost::optional<geometry_msgs::Twist> obstacle_avoid_cmd_;
    std::mutex mtx_;
};
#endif  //ROBOXT_NAVIGATION_PLANNER_H_INCLUDED