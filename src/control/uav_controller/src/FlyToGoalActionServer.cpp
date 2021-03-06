//
// Created by songjin on 18-5-25.
//

#include "uav_controller/FlyToGoalActionServer.h"
#include "ros_common/RosMath.h"
#include <thread>
#include "nav_msgs/Path.h"

FlyToGoalActionServer::FlyToGoalActionServer(std::string name, RosWrapperUAV *ros_uav) : as_(nh_, name, boost::bind(&FlyToGoalActionServer::executeCB, this, _1), false),
                                                                                         action_name_(name),
                                                                                         p_ros_uav_(ros_uav)
{
    as_.start();
    planned_path_pub_ = nh_.advertise<nav_msgs::Path>("/planned_path", 1);
}

void FlyToGoalActionServer::executeCB(const uav_controller::FlyToGoalGoalConstPtr &goal)
{
    nav_msgs::Path path;
    geometry_msgs::PoseStamped current_pose = p_ros_uav_->getCurrentPoseStamped();
    geometry_msgs::PoseStamped current_destination_pose;
    // publish info to the console for the user
    ROS_INFO("%s: Executing, the goal position x = %f, y = %f, z = %f, yaw = %f",
             action_name_.c_str(), goal->goal_pose.pose.position.x,
             goal->goal_pose.pose.position.y,
             goal->goal_pose.pose.position.z,
             RosMath::getYawFromPoseStamp(goal->goal_pose) * 180 / 3.14);

    //        
    bool success = false;
    while (!success)
    {
        geometry_msgs::PoseStamped current_pose = p_ros_uav_->getCurrentPoseStamped();
        if (!generatePath(goal->fly_type,
                          goal->step_length,
                          current_pose,
                          goal->goal_pose,
                          path))
        {
            ROS_ERROR("plan path error!");
            result_.is_reachable = false;
            as_.setAborted(result_);
            return;
        }
        // for test
        // success = true;
        // break;
        //
        auto ite_path = path.poses.begin();
        int time_count = 0; 
        ros::Rate rate(20);
        while (ite_path != path.poses.end())
        {
            // check that preempt has not been requested by the client
            if (as_.isPreemptRequested() || !ros::ok())
            {
                ROS_INFO("%s: Preempted", action_name_.c_str());
                // set the action state to preempted
                as_.setPreempted(result_);
                success = false;
                break;
            }
            current_pose = p_ros_uav_->getCurrentPoseStamped();
            current_destination_pose = *ite_path;
            // call uav fly to goal method with correspond velocity
            p_ros_uav_->fly_to_goal(current_destination_pose);

            // publish the feedback
            /*
            feedback_.distance = (float)RosMath::calDistance(current_pose, goal->goal_pose);
            as_.publishFeedback(feedback_);
            */
            // distance:0.1m  angle:30
            double current_yaw = RosMath::getYawFromPoseStamp(current_pose);
            double current_destination_yaw = RosMath::getYawFromPoseStamp(current_destination_pose);
            if (RosMath::calDistance(current_destination_pose, current_pose) < 0.1 && 
               fabs(current_yaw - current_destination_yaw) < 30.0/180.0*3.14 )
            {
                   ite_path++;
                   ROS_INFO("arrive a waypoint");
            }
            time_count++;
            rate.sleep();
        }
        if (ite_path == path.poses.end())
            success = true;
    }
    if (success)
    {
        current_pose = p_ros_uav_->getCurrentPoseStamped();
        result_.is_reachable = true;
        ROS_INFO("%s: Succeeded", action_name_.c_str());
        // set the action state to succeeded
        as_.setSucceeded(result_);
    }
}

bool FlyToGoalActionServer::generatePath(const std::string &path_planner_name,
                                         double step_length,
                                         const geometry_msgs::PoseStamped &start_pose,
                                         const geometry_msgs::PoseStamped &goal_pose,
                                         nav_msgs::Path &path)
{
    ros::ServiceClient planner_client_;
    planner_client_ = nh_.serviceClient<nav_msgs::GetPlan>(path_planner_name);
    nav_msgs::GetPlan srv;
    srv.request.start = start_pose;
    srv.request.goal = goal_pose;
    srv.request.tolerance = step_length;
    geometry_msgs::PoseStamped last_pose = start_pose;
    if (planner_client_.call(srv))
    {
        std::cout << "\n call planned service ok! \n" << std::endl;
        for (auto pose : srv.response.plan.poses)
        {
            // when we plan the path in the plane, let the head of the uav forward
            if (path_planner_name == "rrt_planner_server")
            {
                double delta_x = pose.pose.position.x - last_pose.pose.position.x;
                double delta_y = pose.pose.position.y - last_pose.pose.position.y;
                double yaw = atan2(delta_y, delta_x);
                RosMath::setPoseStampYawAngle(pose, yaw);
                last_pose = pose;
            }

            double yaw = RosMath::getYawFromPoseStamp(pose);
            /*
            ROS_INFO("get path success, x = %.3f, y = %.3f, z = %.3f, yaw = %.3f",
                     pose.pose.position.x,
                     pose.pose.position.y,
                     pose.pose.position.z,
                     yaw * 180 / 3.14);
            */
        }
        path = srv.response.plan;
        path.header.frame_id = "map";
        planned_path_pub_.publish(path);
        return true;
    }
    else
    {
        ROS_ERROR("Failed to call path planner service");
        return false;
    }
}