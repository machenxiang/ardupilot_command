/***************************************************************************************************************************
* ardupilot velocity control
*
* Author: mcx
*
* Update Time: 2019.9.25
*
* Introduction:  publish velocity to control the rover

***************************************************************************************************************************/

#include <ros/ros.h>
#include <mavros_msgs/State.h>
#include<mavros_msgs/CommandBool.h>
#include<mavros_msgs/CommandTOL.h>
#include <mavros_msgs/HomePosition.h>
#include <geometry_msgs/PoseStamped.h>
#include <mavros_msgs/WaypointList.h>
#include<mavros_msgs/SetMode.h>
#include <vector>
#include <GeographicLib/Geocentric.hpp>
#include <eigen_conversions/eigen_msg.h>
#include <sensor_msgs/NavSatFix.h>
#include <mavros/frame_tf.h>
#include <std_msgs/Float64.h>
#include <mavros_msgs/PositionTarget.h>
#include<iostream>
using namespace std;
//自定义command消息
#include<ardupilot_command/command.h>

mavros_msgs::State current_state;
//using namespace ardupilot_command;

//command command_now;

//mavros飞控状态获取
//arming flag arming 未成功时继续arm的次数
int arm_flag=0;
void state_cb(const mavros_msgs::State::ConstPtr& msg)
{
    current_state=*msg;
    bool connected=current_state.connected;
    bool armed=current_state.armed;
}

//速度控制
mavros_msgs::PositionTarget move_vel(double x,double y)
{
    mavros_msgs::PositionTarget v;
    v.coordinate_frame=1;
    //需要的参数为速度,1,2,4 position ignore flag,8,16,32 velocity ingnore flag,64,128,256accelcration/force ignore flag
    v.type_mask=1+2+4+/*8+16+32*/+64+128+256+512+1024+2048;
    //pos_target.type_mask = 0b100111000111/*0b100111111000*/;
    v.velocity.x=x;
    v.velocity.y=y;
    return v;

}
//位置控制
mavros_msgs::PositionTarget move_pos(double x,double y)
{
    mavros_msgs::PositionTarget pos;
    pos.coordinate_frame=1;
    pos.type_mask=/*1+2+4+*/8+16+32+64+128+256+512+1024+2048;
    pos.position.x=x;
    pos.position.y=y;
    return pos;
}
int main(int argc,char** argv)
{
    //初始化node
    ros::init(argc,argv,"offb_node");
    ros::NodeHandle nh;
    //setpoint的发布必须快于2Hz
    ros::Rate rate(20.0);
    
    //mavros状态消息订阅
    ros::Subscriber state_sub=nh.subscribe<mavros_msgs::State>("mavros/state",10,state_cb);
    //当mavros一直没有连接上则一直回调，直到连接上
    while(ros::ok()&&!current_state.connected)
    {
        ros::spinOnce();
        rate.sleep();
        ROS_INFO("failed connect to mavros");
    }
    ROS_INFO("success connect to mavros");
    //模式设置
    ros::ServiceClient set_mode_client=nh.serviceClient<mavros_msgs::SetMode>("mavros/set_mode");
    mavros_msgs::SetMode offb_set_mode;
    //custom_mode 代表可以用字符串表示
    offb_set_mode.request.custom_mode="GUIDED";
    if(set_mode_client.call(offb_set_mode)&&offb_set_mode.response.mode_sent)
    {
        ROS_INFO("guided enabled");
    }
    else
    {
        ROS_INFO("unable to set guided");
        return -1;
    }
    sleep(5);
    //arming
    ros::ServiceClient arming_client_i=nh.serviceClient<mavros_msgs::CommandBool>("mavros/cmd/arming");
    mavros_msgs::CommandBool srv_arm_i;
    //arm service 赋值为true表示arm，false则为disarm，赋值之后还要用service的客户端call一次
    srv_arm_i.request.value=true;
    arming_client_i.call(srv_arm_i);
    while(!current_state.armed)
    {
        arm_flag++;
        ROS_INFO("the %d arm failed,arm again ",arm_flag);
        srv_arm_i.request.value=true;
        arming_client_i.call(srv_arm_i);
        rate.sleep();
        ros::spinOnce();
        sleep(1);
        if(current_state.armed)
        {
            break;
        }
        if(arm_flag>19)
        {
            ROS_INFO("arm failed,please reboot");
            return -1;
        }

    }
    ROS_INFO(" arm success  ");
 
    // if(arming_client_i.call(srv_arm_i)&&srv_arm_i.response.success)
    // {
    //     ROS_INFO("arming success");
    // }
    // else
    // {
    //     ROS_INFO("fail arming");
    // }
    sleep(2);
    //***************************用于copter********************************
    //takeoff
    // ros::ServiceClient takeoff_client=nh.serviceClient<mavros_msgs::CommandTOL>("mavros/cmd/takeoff");
    // mavros_msgs::CommandTOL srv_takeoff;
    // srv_takeoff.request.altitude=2;
    // srv_takeoff.request.min_pitch=0;
    // srv_takeoff.request.yaw=0;
    // if(takeoff_client.call(srv_takeoff)&&srv_takeoff.response.success)
    // {
    //     ROS_INFO("takeoff success");
    // }
    // else
    // {
    //     ROS_INFO("fail to takeoff");
    // }
    // sleep(10);
    //********************************************************************
    //速度发布
    ros::Publisher vel_pub=nh.advertise<mavros_msgs::PositionTarget>("mavros/setpoint_raw/local",100);
    ros::Publisher pos_pub=nh.advertise<mavros_msgs::PositionTarget>("/mavros/setpoint_raw/local",10);
    // for(int i=0;i<10;i++)
    // {
        
    //     vel_pub.publish(move_vel(1,0));
    //     sleep(1);
    //     ROS_INFO("publish velocity%i",i);
    // }
    //***********************guided位置控制**********************************
    //到第一个点（0.5,0.5）
    for(int i=0;i<10;i++)
    {
        pos_pub.publish(move_pos(0.5,0.5));
        ROS_INFO("move to 0.5 0.5");
        sleep(1);
    }
    
     //到第二个点（0.5,-0.5）
    for(int i=0;i<10;i++)
    {
        pos_pub.publish(move_pos(0.5,-0.5));
        ROS_INFO("move to 0.5 -0.5");
        sleep(1);
    }

       //到第一个点（-0.5,-0.5）
    for(int i=0;i<10;i++)
    {
        pos_pub.publish(move_pos(-0.5,-0.5));
        ROS_INFO("move to -0.5 -0.5");
        sleep(1);
    }

       //到第一个点（-0.5,0.5）
    for(int i=0;i<10;i++)
    {
        pos_pub.publish(move_pos(-0.5,0.5));
        ROS_INFO("move to -0.5 0.5");
        sleep(1);
    }
 
  
  

    while (ros::ok())
    {
       ros::spinOnce();
       rate.sleep();
    }

    cout<<"hahaaa";
    return 0;
}