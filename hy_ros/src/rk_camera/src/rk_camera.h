#ifndef RK_CAMERA_H
#define RK_CAMERA_H

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <memory>
#include <sstream>

class RkCamera : public rclcpp::Node {
public:
    explicit RkCamera(const rclcpp::NodeOptions & options);
    ~RkCamera();

private:
    static GstFlowReturn on_new_sample_stub(GstElement* sink, gpointer user_data);
    GstFlowReturn on_new_sample(GstElement* sink);

    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_;
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr params_callback_handle_;
    GstElement* pipeline_ = nullptr;
    int width_;
    int height_;
    bool debug_fps_ = false;
    bool rotation_180_ = true;
};

#endif // RK_CAMERA_H
