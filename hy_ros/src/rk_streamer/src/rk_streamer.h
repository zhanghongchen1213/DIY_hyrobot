#ifndef RK_STREAMER_H
#define RK_STREAMER_H

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <image_transport/image_transport.hpp>
#include <memory>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

class RkStreamer : public rclcpp::Node {
public:
    explicit RkStreamer(const rclcpp::NodeOptions & options);
    ~RkStreamer();

private:
    void image_callback(const sensor_msgs::msg::Image::ConstSharedPtr & msg);
    void init_gst_pipeline();

    image_transport::Subscriber sub_;
    
    // GStreamer elements
    GstElement *pipeline_obj_ = nullptr;
    GstElement *appsrc_ = nullptr;
    GMainLoop *main_loop_ = nullptr;
    std::thread main_loop_thread_;
    
    // Parameters
    int width_;
    int height_;
    int fps_;
    std::string pipeline_str_;
    bool is_initialized_ = false;
};

#endif // RK_STREAMER_H
