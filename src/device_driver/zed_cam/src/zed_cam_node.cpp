///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2018, STEREOLABS.
//
// All rights reserved.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

/*****************************************************************************************
 ** This sample demonstrates how to capture stereo images and calibration parameters    **
 ** from the ZED camera with OpenCV without using the ZED SDK.                          **
 *****************************************************************************************/

// General includes
#include <iostream>
#include <sstream>
#include <string>

// OpenCV includes
#include <opencv2/opencv.hpp>

// convert it to a ros image message
#include <sensor_msgs/Image.h>
#include <sensor_msgs/image_encodings.h>
#include <cv_bridge/cv_bridge.h>
#include <ros/ros.h>

// Namespaces
using namespace cv;
using namespace std;

int main(int argc, char** argv) {

    ros::init(argc, argv, "zed_cam_node");
    ros::NodeHandle nh;
    ros::Publisher pub_img_left_mono_raw = nh.advertise<sensor_msgs::Image>("/zed/left/image_mono", 1);
    ros::Publisher pub_img_right_mono_raw = nh.advertise<sensor_msgs::Image>("/zed/right/image_mono", 1);
    cv::Size2i image_size = cv::Size2i(672, 376);

    /////////// IMPORTANT ///////////////
    // Assuming here that ZED is connected under device 0 (/dev/video0 for Linux or first camera listed on Windows)
    // On laptops, you may have to change to 1 if the first listed camera is the integrated webcam.
    ////////////////////////////////////
    VideoCapture cap(0);
    if (!cap.isOpened())
    {
        std::cerr << "can not open zed camera!" << std::endl;
        return -1;
    }

    cap.grab();
    // Set the video resolution (2*Width * Height)
    cap.set(CV_CAP_PROP_FRAME_WIDTH, image_size.width * 2);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, image_size.height);
    cap.set(CAP_PROP_FPS, 60);
    std::cout << "frames = " << cap.get(CAP_PROP_FPS) << std::endl;
    cap.grab(); 

    Mat frame, left_raw, left_rect, right_raw, right_rect, left_mono, right_mono;
    cv_bridge::CvImage img_bridge;
    sensor_msgs::Image img_msg;
    int count=0;
    while (ros::ok()) {
        // Get a new frame from camera
        cap >> frame;

        if (++count % 3 == 0)
        {
            // Extract left and right images from side-by-side
            left_raw = frame(cv::Rect(0, 0, frame.cols / 2, frame.rows));
            right_raw = frame(cv::Rect(frame.cols / 2, 0, frame.cols / 2, frame.rows));

            std_msgs::Header header;         // empty header
            header.stamp = ros::Time::now(); // time

            // left mono raw
            cv::cvtColor(left_raw, left_mono, cv::COLOR_BGR2GRAY);
            img_bridge = cv_bridge::CvImage(header, sensor_msgs::image_encodings::MONO8, left_mono);
            img_bridge.toImageMsg(img_msg);         // from cv_bridge to sensor_msgs::Image
            pub_img_left_mono_raw.publish(img_msg); //

            // right mono raw
            cv::cvtColor(right_raw, right_mono, cv::COLOR_BGR2GRAY);
            img_bridge = cv_bridge::CvImage(header, sensor_msgs::image_encodings::MONO8, right_mono);
            img_bridge.toImageMsg(img_msg);          // from cv_bridge to sensor_msgs::Image
            pub_img_right_mono_raw.publish(img_msg); //
        }
    }
    cap.release();
    return 0;
}
