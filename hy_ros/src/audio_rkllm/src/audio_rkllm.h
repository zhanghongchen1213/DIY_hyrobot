#ifndef AUDIO_RKLLM_H_
#define AUDIO_RKLLM_H_

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "rkllm.h"
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>

// 定义指令结构体
struct LLMInstruction {
    std::string text_content; // 完整的指令文本
    std::string image_path;   // 图片路径（如果为空则表示纯文本指令）
    bool has_image;           // 是否包含图片
    rclcpp::Time timestamp;   // 指令生成时间
};

class RKLLMNode : public rclcpp::Node
{
public:
    RKLLMNode();
    ~RKLLMNode();

private:
    // ------------------- ROS 接口 -------------------
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_asr_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_llm_result_;
    
    // ASR 消息回调
    void asr_callback(const std_msgs::msg::String::SharedPtr msg);

    // ------------------- 状态机与消息处理 -------------------
    enum class State {
        IDLE,           // 空闲状态，等待 START
        COLLECTING,     // 正在收集文本
        PROCESSING      // 正在处理 END（准备推理数据）
    };
    
    State current_state_;
    std::string text_buffer_; // 文本收集缓冲区
    bool trigger_detected_;   // 本次对话是否检测到触发词
    std::string captured_image_path_; // 本次对话拍摄的照片路径

    // ------------------- 并发处理与队列 -------------------
    std::queue<LLMInstruction> instruction_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::thread worker_thread_; // 后台推理线程
    std::atomic<bool> running_; // 线程运行标志

    // 工作线程函数
    void worker_loop();

    // ------------------- 功能函数 -------------------
    // 检测触发词
    bool check_trigger_word(const std::string& text);
    
    // 拍摄图像
    std::string capture_image();

    // ------------------- RKLLM 推理 -------------------
    LLMHandle llm_handle_ = nullptr;
    
    // 初始化模型
    bool init_rkllm();
    
    // 执行推理
    std::string run_inference(const LLMInstruction& instruction);
    
    // RKLLM 回调函数 (静态)
    // 根据 SDK 定义修改为 int 返回
    static int rkllm_callback(RKLLMResult *result, void *userdata, LLMCallState state);

    // 静态成员变量，用于保存当前活动的节点实例指针，以便在静态回调中使用 (如果 userdata 无法满足需求)
    static RKLLMNode* global_node_instance_;

    // ------------------- 参数配置 -------------------
    std::string llm_model_path_;
    std::string vision_model_path_; // 视觉编码器模型（如果有）
    
    // RKLLMParam 参数
    int max_new_tokens_;
    int max_context_len_;
    int top_k_;
    double top_p_;
    double temperature_;
    double repeat_penalty_;
    double frequency_penalty_;
    double presence_penalty_;
    bool skip_special_token_;
    int base_domain_id_;
    int embed_flash_;

    bool camera_flip_;
    bool enable_multimodal_; // 是否启用多模态功能
    bool force_english_;     // 是否强制输出英文
    std::string image_save_path_ = "/tmp/capture.jpg";
    
    // 模型特定 token
    std::string img_start_token_;
    std::string img_end_token_;
    std::string img_pad_token_;
};

#endif
