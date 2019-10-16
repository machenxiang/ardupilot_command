/***************************************************************************************************************************
* ardupilot velocity control
*
* Author: mcx
*
* Update Time: 2019.10.8
*
* Introduction:  publish odom to the device

***************************************************************************************************************************/
#include <ros/ros.h>
#include<nav_msgs/Odometry.h>
#include<tf/transform_broadcaster.h>
#include<mavros_msgs/HomePosition.h>
#include<geometry_msgs/PoseStamped.h>
#include<geometry_msgs/Vector3.h>
#include<geometry_msgs/Quaternion.h>
#include<tf/transform_datatypes.h>
#include<iostream>
using namespace std;

#include <mavros_msgs/State.h>


nav_msgs::Odometry odom;
//普通四元数
geometry_msgs::Quaternion orientation;


void local_pos_cb(const geometry_msgs::PoseStamped::ConstPtr& msg)
{
    //获取位置信息中的位姿数据
    orientation=msg->pose.orientation;
    //普通四元数转化为tf四元数
    // tf::quaternionMsgToTF(orientation,tf_orientation);
    // tf::Matrix3x3(tf_orientation).getRPY(roll,pitch,yaw);
    // rpy.x=roll;
    // rpy.y=pitch;
    // rpy.z=yaw;

}
ros::Time current_time;



int main(int argc,char **argv)
{
    ros::init(argc,argv,"pub_odom");
    ros::NodeHandle nh;
    //ros::Subscriber imu_sub;
    ros::Subscriber local_pos_sub=nh.subscribe<geometry_msgs::PoseStamped>("mavros/local_position/pose",100,local_pos_cb);
    ros::Publisher odom_pub=nh.advertise<nav_msgs::Odometry>("odom",50);
    while(ros::ok())
    {
        ros::Rate rate(10);

        current_time=ros::Time::now();
        odom.header.stamp=current_time;
        odom.header.frame_id="odom";
        odom.child_frame_id="base_footprint";
        odom.pose.pose.position.x=0;
        odom.pose.pose.position.y=0;
        odom.pose.pose.position.z=0;
        odom.pose.pose.orientation=orientation;
        odom_pub.publish(odom);
        cout<<"odom.x= "<<odom.pose.pose.orientation.x<<" odom.y= "<<odom.pose.pose.orientation.y<<" odom.z= "<<odom.pose.pose.orientation.z<<" odom.w="<<odom.pose.pose.orientation.w<<endl;
        ROS_INFO("hah");
        ros::spinOnce();
        rate.sleep();
    }



    return 0;
}