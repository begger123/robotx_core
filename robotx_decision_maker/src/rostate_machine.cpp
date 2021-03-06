#include <rostate_machine.h>

rostate_machine::rostate_machine(std::string xml_filepath, std::string dot_filepath, std::string state_machine_name)
{
    state_machine_ptr_ = std::make_shared<state_machine>(xml_filepath);
    state_machine_ptr_->draw_state_machine(dot_filepath);
    state_machine_name_ = state_machine_name;
    nh_.param<double>(ros::this_node::getName()+"/publish_rate", publish_rate_, 10);
    current_state_pub_ = nh_.advertise<robotx_msgs::State>(ros::this_node::getName()+"/"+state_machine_name+"/current_state",1);
    state_changed_pub_ = nh_.advertise<robotx_msgs::StateChanged>(ros::this_node::getName()+"/"+state_machine_name+"/state_changed",1);
}

rostate_machine::~rostate_machine()
{

}

void rostate_machine::event_callback_(const ros::MessageEvent<robotx_msgs::Event const>& event)
{
    robotx_msgs::Event msg = *event.getMessage();
    state_info_t old_info = state_machine_ptr_->get_state_info();
    bool result = state_machine_ptr_->try_transition(msg.trigger_event_name);
    state_info_t info = state_machine_ptr_->get_state_info();
    if(!result)
    {
        std::string publisher_name = event.getPublisherName();
        ROS_INFO_STREAM( "from : " << publisher_name << ", failed to transition, current state : "<< info.current_state << ", event_name : " << msg.trigger_event_name);
    }
    else
    {
        robotx_msgs::StateChanged state_changed_msg;
        state_changed_msg.current_state = info.current_state;
        state_changed_msg.old_state = old_info.current_state;
        state_changed_msg.triggered_event = msg.trigger_event_name;
        state_changed_pub_.publish(state_changed_msg);
    }
    return;
}

void rostate_machine::run()
{
    boost::thread publish_thread(boost::bind(&rostate_machine::publish_current_state_, this));
    trigger_event_sub_ = nh_.subscribe(ros::this_node::getName()+"/"+state_machine_name_+"/trigger_event", 10, &rostate_machine::event_callback_,this);
    return;
}

void rostate_machine::publish_current_state_()
{
    ros::Rate rate(publish_rate_);
    while(ros::ok())
    {
        robotx_msgs::State state_msg;
        state_info_t info = state_machine_ptr_->get_state_info();
        state_msg.current_state = info.current_state;
        state_msg.possible_transitions = info.possibe_transitions;
        state_msg.possible_transition_states = info.possibe_transition_states;
        state_msg.header.stamp = ros::Time::now();
        current_state_pub_.publish(state_msg);
        rate.sleep();
    }
    return;
}