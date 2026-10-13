#include "audio_rkllm.h"
#include <chrono>
#include <functional>
#include <iostream>

using namespace std::chrono_literals;

// 初始化静态成员
RKLLMNode* RKLLMNode::global_node_instance_ = nullptr;

RKLLMNode::RKLLMNode()
    : Node("audio_rkllm_node"), 
      current_state_(State::IDLE), 
      trigger_detected_(false),
      running_(true)
{
    // 设置全局实例指针
    global_node_instance_ = this;

    // ------------------- 参数读取 -------------------
    try {
        // 模型路径配置 (强制无默认值)
        llm_model_path_ = this->declare_parameter<std::string>("llm_model_path");
        vision_model_path_ = this->declare_parameter<std::string>("vision_model_path");

        // 推理参数配置
        max_new_tokens_ = this->declare_parameter<int>("max_new_tokens");
        max_context_len_ = this->declare_parameter<int>("max_context_len");
        top_k_ = this->declare_parameter<int>("top_k");
        top_p_ = this->declare_parameter<double>("top_p");
        temperature_ = this->declare_parameter<double>("temperature");
        repeat_penalty_ = this->declare_parameter<double>("repeat_penalty");
        frequency_penalty_ = this->declare_parameter<double>("frequency_penalty");
        presence_penalty_ = this->declare_parameter<double>("presence_penalty");
        skip_special_token_ = this->declare_parameter<bool>("skip_special_token");
        base_domain_id_ = this->declare_parameter<int>("base_domain_id");
        embed_flash_ = this->declare_parameter<int>("embed_flash");

        // 视觉相关参数配置
        camera_flip_ = this->declare_parameter<bool>("camera_flip");
        enable_multimodal_ = this->declare_parameter<bool>("enable_multimodal");
        
        // 语言控制参数
        force_english_ = this->declare_parameter<bool>("force_english");

        // 多模态特殊Token配置
        img_start_token_ = this->declare_parameter<std::string>("img_start_token");
        img_end_token_ = this->declare_parameter<std::string>("img_end_token");
        img_pad_token_ = this->declare_parameter<std::string>("img_pad_token");

        // ------------------- ROS 接口参数 -------------------
        // 订阅 ASR 文本话题
        std::string asr_topic = this->declare_parameter<std::string>("sub_asr_text_topic");
        
        // 发布 LLM 结果话题
        std::string result_topic = this->declare_parameter<std::string>("pub_llm_result_topic");

        // ------------------- 初始化 RKLLM -------------------
        if (!init_rkllm()) {
            RCLCPP_ERROR(this->get_logger(), "RKLLM模型初始化失败！");
            throw std::runtime_error("RKLLM Init Failed");
        }

        // ------------------- ROS 接口初始化 -------------------
        sub_asr_ = this->create_subscription<std_msgs::msg::String>(
            asr_topic, 10, std::bind(&RKLLMNode::asr_callback, this, std::placeholders::_1));

        pub_llm_result_ = this->create_publisher<std_msgs::msg::String>(result_topic, 10);

        // ------------------- 启动工作线程 -------------------
        worker_thread_ = std::thread(&RKLLMNode::worker_loop, this);
        
        RCLCPP_INFO(this->get_logger(), "RKLLM节点初始化成功。正在监听话题：%s", asr_topic.c_str());

    } catch (const rclcpp::exceptions::ParameterUninitializedException& e) {
        RCLCPP_FATAL(this->get_logger(), "启动失败：必要参数缺失！请在launch文件中配置所有参数。错误详情：%s", e.what());
        throw;
    } catch (const std::exception& e) {
        RCLCPP_FATAL(this->get_logger(), "启动失败：发生异常：%s", e.what());
        throw;
    }
}

RKLLMNode::~RKLLMNode()
{
    // 清空全局实例指针
    if (global_node_instance_ == this) {
        global_node_instance_ = nullptr;
    }

    // 停止工作线程
    running_ = false;
    queue_cv_.notify_all();
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }

    // 销毁 RKLLM 句柄
    if (llm_handle_ != nullptr) {
        rkllm_destroy(llm_handle_);
    }
}

// ------------------- RKLLM 核心逻辑 -------------------

bool RKLLMNode::init_rkllm()
{
    if (llm_model_path_.empty()) {
        RCLCPP_ERROR(this->get_logger(), "未提供模型路径！");
        return false;
    }

    RCLCPP_INFO(this->get_logger(), "正在初始化RKLLM，加载模型：%s", llm_model_path_.c_str());

    RKLLMParam param = rkllm_createDefaultParam();
    param.model_path = llm_model_path_.c_str();
    param.max_new_tokens = max_new_tokens_;
    param.max_context_len = max_context_len_;
    
    // 常用默认参数设置 (使用ROS参数覆盖)
    param.top_k = top_k_;
    param.top_p = top_p_;
    param.temperature = temperature_;
    param.repeat_penalty = repeat_penalty_;
    param.frequency_penalty = frequency_penalty_;
    param.presence_penalty = presence_penalty_;
    param.skip_special_token = skip_special_token_;
    param.extend_param.base_domain_id = base_domain_id_;
    param.extend_param.embed_flash = embed_flash_;

    // 初始化 RKLLM，绑定回调函数
    int ret = rkllm_init(&llm_handle_, &param, rkllm_callback);
    if (ret != 0) {
        RCLCPP_ERROR(this->get_logger(), "rkllm_init初始化失败，错误码：%d", ret);
        return false;
    }
    
    RCLCPP_INFO(this->get_logger(), "RKLLM初始化成功！");
    return true;
}

// ------------------- 消息处理逻辑 -------------------

void RKLLMNode::asr_callback(const std_msgs::msg::String::SharedPtr msg)
{
    const std::string& content = msg->data;  // 使用引用避免拷贝

    if (content == "START") {
        // 状态机复位，开始新的一轮收集
        current_state_ = State::COLLECTING;
        text_buffer_.clear();
        trigger_detected_ = false;
        captured_image_path_.clear();
        RCLCPP_INFO(this->get_logger(), "收到START信号。开始收集文本...");
    } 
    else if (content == "END") {
        if (current_state_ == State::COLLECTING) {
            current_state_ = State::PROCESSING;
            RCLCPP_INFO(this->get_logger(), "收到END信号。开始处理指令...");
            
            // 构建指令对象
            LLMInstruction instr;
            instr.text_content = text_buffer_;
            instr.has_image = trigger_detected_;
            if (trigger_detected_) {
                instr.image_path = captured_image_path_;
            }
            instr.timestamp = this->now();

            // 放入队列
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                instruction_queue_.push(instr);
            }
            queue_cv_.notify_one();
            
            // 重置状态
            current_state_ = State::IDLE;
        }
    } 
    else {
        // 中间文本内容
        if (current_state_ == State::COLLECTING) {
            text_buffer_ += content;
            
            // 实时检测触发词 (仅当还未触发过时检测)
            if (!trigger_detected_ && check_trigger_word(content)) {
                RCLCPP_INFO(this->get_logger(), "检测到触发词！正在拍照...");
                
                // 执行拍照 (阻塞操作，但这是在回调线程中，应该很快)
                std::string path = capture_image();
                if (!path.empty()) {
                    trigger_detected_ = true;
                    captured_image_path_ = path;
                    RCLCPP_INFO(this->get_logger(), "图像已捕获：%s", path.c_str());
                } else {
                    RCLCPP_WARN(this->get_logger(), "图像捕获失败。");
                }
            }
        }
    }
}

bool RKLLMNode::check_trigger_word(const std::string& text)
{
    // 简单的子串查找
    if (text.find("看一下") != std::string::npos || 
        text.find("拍张照片") != std::string::npos ||
        text.find("看下") != std::string::npos ||
        text.find("拍照") != std::string::npos || 
        text.find("看") != std::string::npos ) {
        return true;
    }
    return false;
}

std::string RKLLMNode::capture_image()
{
    // 打开默认摄像头 (索引 0)
    // 注意：如果在容器或受限环境中，可能需要检查设备权限
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        RCLCPP_ERROR(this->get_logger(), "无法打开摄像头0");
        // 尝试索引 1，以防万一
        cap.open(1);
        if (!cap.isOpened()) {
             RCLCPP_ERROR(this->get_logger(), "尝试打开摄像头1失败");
             return "";
        }
    }

    // 设置分辨率 (尝试设置高清)
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);

    cv::Mat frame;
    // 丢弃前几帧以等待自动曝光稳定
    for(int i = 0; i < 5; i++) {
        cap >> frame;
    }
    
    // 正式拍摄
    cap >> frame;

    if (frame.empty()) {
        RCLCPP_ERROR(this->get_logger(), "捕获到空帧！");
        return "";
    }

    // 根据参数翻转图像
    if (!camera_flip_) {
        // 如果设置为 False，进行 180 度旋转 (水平+垂直翻转)
        cv::flip(frame, frame, -1);
        RCLCPP_INFO(this->get_logger(), "图像已旋转180度。");
    }

    // 保存图像
    if (cv::imwrite(image_save_path_, frame)) {
        return image_save_path_;
    } else {
        RCLCPP_ERROR(this->get_logger(), "保存图像失败：%s", image_save_path_.c_str());
        return "";
    }
}

// ------------------- 工作线程：顺序处理推理任务 -------------------

void RKLLMNode::worker_loop()
{
    while (running_) {
        LLMInstruction current_instr;
        
        // 等待队列有数据
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] { return !instruction_queue_.empty() || !running_; });
            
            if (!running_) break;
            
            current_instr = instruction_queue_.front();
            instruction_queue_.pop();
        }

        // 开始处理
        RCLCPP_INFO(this->get_logger(), "正在处理指令：%s [图片：%s]", 
            current_instr.text_content.c_str(), 
            current_instr.has_image ? "Yes" : "No");

        std::string final_response = run_inference(current_instr);
        
        RCLCPP_INFO(this->get_logger(), "推理结果完整文本：%s", final_response.c_str());
        
        // 发布结束信号，告诉下游（TTS）本次生成结束
        // 这一步对于流式处理非常重要，下游需要知道什么时候断句
        // 按照约定，发送 "END" 作为结束信号
        
        std_msgs::msg::String eos_msg;
        eos_msg.data = "END";
        pub_llm_result_->publish(eos_msg);
        
        RCLCPP_INFO(this->get_logger(), "推理完成。流式输出已全部发布 (发送 END 信号)。");

        // 清理临时文件
        if (current_instr.has_image && !current_instr.image_path.empty()) {
            // 在实际生产中，可能希望保留图片用于调试，这里按需求删除
            // remove(current_instr.image_path.c_str());
        }
    }
}

std::string RKLLMNode::run_inference(const LLMInstruction& instruction)
{
    if (llm_handle_ == nullptr) {
        return "错误：模型未初始化。";
    }

    // 初始化 infer 参数结构体
    // 参考 test.cpp:124
    RKLLMInferParam infer_param;
    memset(&infer_param, 0, sizeof(RKLLMInferParam));

    infer_param.mode = RKLLM_INFER_GENERATE;
    // 参考 test.cpp:162 (keep_history = 0)
    infer_param.keep_history = 0;
    
    // 构建 Prompt
    // 如果有图片，Qwen2-VL 等多模态模型通常需要特殊的 Prompt 格式
    std::string prompt;
    
    // 参考 test.cpp:120, 初始化 rkllm_input
    RKLLMInput input;
    memset(&input, 0, sizeof(RKLLMInput));

    if (instruction.has_image) {
        // 多模态输入模式
        if (enable_multimodal_) {
            // 目前暂未集成 RKNN 视觉编码器，因此暂时降级为纯文本模式或报错
            RCLCPP_WARN(this->get_logger(), "已启用多模态参数(enable_multimodal=True)，但当前代码暂未集成RKNN视觉编码器，无法处理多模态输入。将仅使用文本Prompt。");
            // 临时回退到纯文本模式，避免编译报错
            input.input_type = RKLLM_INPUT_PROMPT;
            prompt = instruction.text_content; 
        } else {
            // 虽然有图片，但多模态未启用，降级为纯文本
            RCLCPP_INFO(this->get_logger(), "检测到图片但多模态功能未启用(enable_multimodal=False)，仅使用文本Prompt。");
            input.input_type = RKLLM_INPUT_PROMPT;
            prompt = instruction.text_content;
        }
    } else {
        // 纯文本模式
        input.input_type = RKLLM_INPUT_PROMPT;
        prompt = instruction.text_content;
    }
    
    // 如果启用强制英文输出，追加指令
    if (force_english_) {
        prompt += " Please answer in English.";
    }

    input.prompt_input = (char*)prompt.c_str();
    // 参考 test.cpp:197
    input.role = "user"; // 或者是 "system" 等，视模型模板而定

    // 响应缓冲区，用于存储回调函数返回的生成文本
    std::string response_buffer;
    
    // 执行推理
    // rkllm_run 是阻塞调用，直到生成结束
    // 我们将 response_buffer 的指针作为 userdata 传入，回调函数会将生成的文本追加到 buffer 中
    int ret = rkllm_run(llm_handle_, &input, &infer_param, &response_buffer);
    
    if (ret != 0) {
        RCLCPP_ERROR(this->get_logger(), "rkllm_run运行失败，错误码：%d", ret);
        return "推理过程中发生错误。";
    }
    
    return response_buffer;
}

// 静态回调函数实现
// 负责处理 RKLLM 的流式输出或最终输出
int RKLLMNode::rkllm_callback(RKLLMResult *result, void *userdata, LLMCallState state)
{
    // 获取节点实例
    RKLLMNode* node = global_node_instance_;
    if (!node) return 0;

    // 仅在正常运行状态且有文本输出时处理
    if (state == RKLLM_RUN_NORMAL && result->text != nullptr) {
        // 1. 累积完整文本到 buffer (保留原有逻辑，用于最后可能的完整性校验或记录)
        if (userdata != nullptr) {
            std::string *buffer = static_cast<std::string*>(userdata);
            *buffer += result->text;
        }

        // 2. 实现流式发布 (关键修改)
        // 直接通过 ROS 发布当前生成的 token
        std_msgs::msg::String chunk_msg;
        chunk_msg.data = result->text;
        node->pub_llm_result_->publish(chunk_msg);
        
        // 可选：打印调试信息，显示实时生成的字符
        // RCLCPP_INFO(node->get_logger(), "Stream: %s", result->text);
    }
    // 注意：如果是 RKLLM_RUN_FINISH，通常不需要特殊处理，run 函数会返回
    return 0;
}

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<RKLLMNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
