//
// Created by songjin on 18-5-25.
//

#include "uav_controller/RosWrapperUAV.h"
#include "nav_msgs/GetPlan.h"
#include <geometry_msgs/Twist.h>
#include "ros_common/RosMath.h"
#include <iostream>
RosWrapperUAV::RosWrapperUAV():
        vision_pose_ok_flag(true)
{
    vision_pose_sub_ = n_.subscribe("/vision_pose/pose", 1, &RosWrapperUAV::vision_pose_callback, this);
    mavros_position_pub_ = n_.advertise<geometry_msgs::PoseStamped>("/mavros/setpoint_position/local",1);
    mavros_attitute_pub_ = n_.advertise<geometry_msgs::PoseStamped>("/mavros/setpoint_attitude/local",1);
    mavros_velocity_pub_ = n_.advertise<geometry_msgs::Twist>("/mavros/setpoint_velocity/cmd_vel_unstamped", 1);
    mavros_vision_pose_pub_ = n_.advertise<geometry_msgs::PoseStamped>("/mavros/mocap/pose",1);
    //uav_local_pose_sub_ = n_.subscribe("/mavros/local_position/pose",1, &RosWrapperUAV::uav_local_pose_callback, this);
    // for test
    uav_local_pose_sub_ = n_.subscribe("/mavros/mocap/pose",1, &RosWrapperUAV::uav_local_pose_callback, this);
    uav_pose_pub_.pose.orientation.w = 1.0;
    // control uav thread
    t_uav_control_loop = std::thread(&RosWrapperUAV::uav_control_loop, this, 20);
}

void RosWrapperUAV::vision_pose_callback(const geometry_msgs::PoseStamped &vision_pose)
{
    // 对视觉定位结果进行处理,防止炸鸡

    // 将vision_pose转化为uav_pose
    // vision_pose:相机在建图坐标系
    // uav_pose:无人机在世界坐标系
    geometry_msgs::PoseStamped uav_pose = vision_pose;
    uav_pose.header.stamp = ros::Time::now();
    // uav_pose.header.frame_id = "world";

    // 如果orb定位lost,三个位置分量返回const
    //double vision_x = uav_pose.pose.position.x;
    //double vision_y = uav_pose.pose.position.y;
    //double vision_z = uav_pose.pose.position.z;
    if(RosMath::calDistance(uav_pose_pub_, uav_pose) == 0)
    {
        vision_pose_ok_flag = false;
        ROS_ERROR("vision localization lost!!!");
        return;
    }

    // roll和pitch角不会超过45度,M_PI_4
    double roll = 0, pitch = 0, yaw = 0;
    RosMath::getRPYFromPoseStamp(uav_pose, roll, pitch, yaw);
    if(fabs(roll) > M_PI_4 || fabs(pitch) > M_PI_4)
    {
        vision_pose_ok_flag = false;
        ROS_ERROR("roll or pitch angel is greater then 45 degrees");
        return;
    }

    // 两次的pose位置变化不能大于0.5
    if (RosMath::calDistance(uav_pose_pub_, uav_pose) > 0.5)
    {
        vision_pose_ok_flag = false;
        ROS_ERROR("distance of two vision estimated position is greater than 0.5m");
        return;
    }

    if(vision_pose_ok_flag)
        uav_pose_pub_ = uav_pose;
    mavros_vision_pose_pub_.publish(uav_pose_pub_);
    vision_pose_ok_flag = true;
}

void RosWrapperUAV::uav_local_pose_callback(const geometry_msgs::PoseStamped &msg)
{
    uav_pose_ = msg;
}
void RosWrapperUAV::uav_control_loop(int loop_rate) {
    ros::Rate rate(loop_rate);
    while (ros::ok()) {
        mavros_position_pub_.publish(goal_pose_);
        rate.sleep();
    }
}
geometry_msgs::PoseStamped RosWrapperUAV::getCurrentPoseStamped() {
    return uav_pose_;
}

void RosWrapperUAV::fly_to_goal(const geometry_msgs::PoseStamped &goal_pose)
{
    if(!vision_pose_ok_flag)
    {
        ROS_ERROR("vision pose is not available, try to let uav being balance");
        geometry_msgs::PoseStamped balance_pose; 
	    balance_pose.pose.orientation.w = 1.0;
	    mavros_attitute_pub_.publish(balance_pose);
        return;
    }
    goal_pose_ = goal_pose;
}
