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
#include<geometry_msgs/Twist.h>


using namespace std;
//自定义command消息
//#include<ardupilot_command/command.h>

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
geometry_msgs::Twist TwistVel;
void velocity_cb(const geometry_msgs::Twist::ConstPtr& msg)
{ 
  TwistVel.linear.x=msg->linear.x*4;
  TwistVel.linear.y=msg->linear.y*4;
  TwistVel.linear.z=msg->linear.z*4;
  TwistVel.angular.x=msg->angular.x*4;
  TwistVel.angular.x=msg->angular.x*4;
  TwistVel.angular.x=msg->angular.x*4;
  ROS_INFO("hah");

}


//速度控制

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
 
    ros::Publisher TwistVelocity_pub=nh.advertise<geometry_msgs::Twist>("/mavros/setpoint_velocity/cmd_vel_unstamped",10);
    ros::Subscriber velocity_sub=nh.subscribe<geometry_msgs::Twist>("/dock_drive/velocity",10,velocity_cb);


    while (ros::ok())
    {      
        TwistVel.angular.x=0;
        TwistVel.angular.y=0;
        TwistVel.angular.z=1;
        TwistVel.linear.x=0.1;         
        TwistVel.linear.y=0;
        TwistVel.linear.z=0;
            
        TwistVelocity_pub.publish(TwistVel);
        cout<<"linear\n"<<"x= "<<TwistVel.linear.x<<" y= "<<TwistVel.linear.y<<" z= "<<TwistVel.linear.z<<"\nangular\n"<<" x="<<
        TwistVel.angular.x<<" y= "<<TwistVel.angular.y<<" z= "<<TwistVel.angular.z<<endl;
        sleep(1);
            //rate.sleep();
        
        ros::spinOnce();
        rate.sleep();
    }


    
    return 0;
}