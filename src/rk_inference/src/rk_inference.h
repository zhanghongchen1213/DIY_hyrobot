#ifndef RK_INFERENCE_H_
#define RK_INFERENCE_H_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include "rknn_api.h"
#include "im2d.h"
#include <vector>
#include <mutex>

// RKNN 模型相关参数
#define OBJ_CLASS_NUM     1
#define NMS_THRESH        0.45
#define BOX_THRESH        0.5
#define MODEL_WIDTH       640
#define MODEL_HEIGHT      640

// 简单的检测结果结构体
struct DetectObject {
    int id;
    float prob;
    float x; // left
    float y; // top
    float w;
    float h;
    const char* class_name;
};

class RkInference : public rclcpp::Node {
public:
    explicit RkInference(const rclcpp::NodeOptions & options);
    ~RkInference();

private:
    void topic_callback(sensor_msgs::msg::Image::UniquePtr msg);
    
    // 初始化 RKNN
    int init_rknn_model(const std::string& model_path);
    // 后处理
    void post_process(int width, int height);
    // NMS
    void nms(std::vector<DetectObject>& src, std::vector<DetectObject>& dst);

    // 参数
    std::string sub_topic_;
    std::string pub_topic_;
    std::string model_path_;
    float conf_threshold_;
    float nms_threshold_;

    // ROS 接口
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_annotated_;

    // RKNN 上下文
    rknn_context ctx_ = 0;
    bool is_rknn_init_ = false;
    
    // RKNN 输入输出内存
    rknn_input_output_num io_num_;
    rknn_tensor_attr* input_attrs_ = nullptr;
    rknn_tensor_attr* output_attrs_ = nullptr;
    rknn_tensor_attr* input_native_attrs_ = nullptr;
    rknn_tensor_attr* output_native_attrs_ = nullptr;
    rknn_tensor_mem* input_mems_[1] = {nullptr};
    std::vector<rknn_tensor_mem*> output_mems_;
    
    // 模型输入 Buffer (RGA 目标内存)
    void* model_input_buf_ = nullptr;
    size_t model_input_size_ = 0;

    // RGA 对齐缓冲 (用于解决 ROS 消息内存未对齐问题)
    void* aligned_buf_ = nullptr;
    size_t aligned_buf_size_ = 0;

    // 检测结果
    std::vector<DetectObject> detect_results_;
};

#endif // RK_INFERENCE_H_
