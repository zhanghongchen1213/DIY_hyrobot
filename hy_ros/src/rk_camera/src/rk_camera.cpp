#include "rk_camera.h"
#include <rclcpp_components/register_node_macro.hpp>
#include "im2d.h"

/**
 * @brief RkCamera 构造函数
 * 
 * 执行流程：
 * 1. 声明并获取 ROS 参数（设备ID、分辨率、帧率等）。
 * 2. 创建 ROS 图像发布者。
 * 3. 初始化 GStreamer 库。
 * 4. 构建 GStreamer 管道字符串。
 *    - 全硬件加速路径：v4l2src (硬件采集) -> videoconvert (格式转换) -> appsink (内存映射)
 *    - 格式固定为 NV12，这是 Rockchip RGA/RKNPU 的首选格式，避免了 CPU 软转换。
 * 5. 启动 GStreamer 管道并绑定回调函数 on_new_sample。
 */
RkCamera::RkCamera(const rclcpp::NodeOptions & options)
: Node("rk_camera", options)
{
    // 1. 声明参数 (Declare parameters)
    this->declare_parameter("device_id", 0);
    this->declare_parameter("width", 1920);
    this->declare_parameter("height", 1080);
    this->declare_parameter("framerate", 30);
    this->declare_parameter("image_topic", "/yolo/image_raw");
    this->declare_parameter("debug_fps", false);
    this->declare_parameter("rotation_180", false);

    // 获取参数 (Get parameters)
    int device_id = this->get_parameter("device_id").as_int();
    width_ = this->get_parameter("width").as_int();
    height_ = this->get_parameter("height").as_int();
    int framerate = this->get_parameter("framerate").as_int();
    std::string topic_name = this->get_parameter("image_topic").as_string();
    debug_fps_ = this->get_parameter("debug_fps").as_bool();
    rotation_180_ = this->get_parameter("rotation_180").as_bool();

    // 参数回调 (Parameter callback)
    params_callback_handle_ = this->add_on_set_parameters_callback(
        [this](const std::vector<rclcpp::Parameter> & parameters) {
            rcl_interfaces::msg::SetParametersResult result;
            result.successful = true;
            for (const auto & param : parameters) {
                if (param.get_name() == "debug_fps") {
                    debug_fps_ = param.as_bool();
                    RCLCPP_INFO(this->get_logger(), "调试帧率日志已%s", debug_fps_ ? "开启" : "关闭");
                }
                if (param.get_name() == "rotation_180") {
                    rotation_180_ = param.as_bool();
                    RCLCPP_INFO(this->get_logger(), "旋转180度已%s", rotation_180_ ? "开启" : "关闭");
                }
            }
            return result;
        }
    );

    RCLCPP_INFO(this->get_logger(), "初始化 RkCamera，设备=/dev/video%d, %dx%d@%d, 旋转=%s",
        device_id, width_, height_, framerate, rotation_180_ ? "ON" : "OFF");

    // 2. 创建发布者 (Create Publisher)
    // 使用 sensor_msgs::msg::Image 类型
    pub_ = this->create_publisher<sensor_msgs::msg::Image>(topic_name, 10);

    // 3. 初始化 GStreamer (Initialize GStreamer)
    if (!gst_is_initialized()) {
        gst_init(nullptr, nullptr);
    }

    // 4. 构建 Pipeline (Build Pipeline)
    // 关键点：format=NV12
    // NV12 是 YUV420SP 格式，Rockchip 平台的硬件单元（ISP, RGA, VPU, NPU）均原生支持此格式。
    // 使用 NV12 可以避免在 CPU 上进行高开销的 RGB 转换。
    std::ostringstream ss;
    ss << "v4l2src device=/dev/video" << device_id << " ! "
       << "videoconvert ! "
       << "video/x-raw,format=NV12,width=" << width_ << ",height=" << height_ << ",framerate=" << framerate << "/1 ! "
       << "appsink name=sink emit-signals=true max-buffers=1 drop=true";
    
    std::string pipe_str = ss.str();
    RCLCPP_INFO(this->get_logger(), "GStreamer 管道: %s", pipe_str.c_str());

    GError *error = nullptr;
    pipeline_ = gst_parse_launch(pipe_str.c_str(), &error);
    
    if (error) {
        RCLCPP_ERROR(this->get_logger(), "GStreamer 管道解析失败: %s", error->message);
        g_error_free(error);
        return;
    }

    if (!pipeline_) {
        RCLCPP_ERROR(this->get_logger(), "无法创建 GStreamer 管道");
        return;
    }

    // 获取 sink 元素并设置回调 (Setup appsink callback)
    // 当有新的一帧数据到达 appsink 时，触发 on_new_sample 回调
    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline_), "sink");
    if (sink) {
        g_signal_connect(sink, "new-sample", G_CALLBACK(on_new_sample_stub), this);
        gst_object_unref(sink);
    } else {
        RCLCPP_ERROR(this->get_logger(), "无法获取 sink 元素");
        return;
    }

    // 5. 启动 Pipeline (Start Pipeline)
    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        RCLCPP_ERROR(this->get_logger(), "无法启动管道 (设置状态为 PLAYING 失败)");
    } else {
        RCLCPP_INFO(this->get_logger(), "GStreamer 管道已启动");
    }

    // 创建定时器检查 GStreamer 总线消息（错误处理）
    timer_ = this->create_wall_timer(
        std::chrono::milliseconds(100),
        [this]() {
            if (!pipeline_) return;
            GstBus *bus = gst_element_get_bus(pipeline_);
            if (bus) {
                GstMessage *msg = gst_bus_pop_filtered(bus, 
                    static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_WARNING));
                if (msg) {
                    GError *err;
                    gchar *debug_info;
                    switch (GST_MESSAGE_TYPE(msg)) {
                        case GST_MESSAGE_ERROR:
                            gst_message_parse_error(msg, &err, &debug_info);
                            RCLCPP_ERROR(this->get_logger(), "GStreamer 错误: %s", err->message);
                            RCLCPP_ERROR(this->get_logger(), "调试信息: %s", debug_info ? debug_info : "无");
                            g_clear_error(&err);
                            g_free(debug_info);
                            break;
                        case GST_MESSAGE_EOS:
                            RCLCPP_INFO(this->get_logger(), "GStreamer 流结束 (EOS)");
                            break;
                        case GST_MESSAGE_WARNING:
                            gst_message_parse_warning(msg, &err, &debug_info);
                            RCLCPP_WARN(this->get_logger(), "GStreamer 警告: %s", err->message);
                            RCLCPP_WARN(this->get_logger(), "调试信息: %s", debug_info ? debug_info : "无");
                            g_clear_error(&err);
                            g_free(debug_info);
                            break;
                        default:
                            break;
                    }
                    gst_message_unref(msg);
                }
                gst_object_unref(bus);
            }
        }
    );
}

RkCamera::~RkCamera()
{
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
    }
}

GstFlowReturn RkCamera::on_new_sample_stub(GstElement* sink, gpointer user_data)
{
    return static_cast<RkCamera*>(user_data)->on_new_sample(sink);
}

/**
 * @brief 处理新帧的回调函数
 * 
 * 这里的逻辑完全符合零拷贝设计理念（尽管这里有一步不可避免的内存拷贝，原因见下）：
 * 1. GStreamer 产生 NV12 数据。
 * 2. 创建 UniquePtr 管理的 ROS 消息。
 * 3. 填充消息头。
 * 4. 拷贝数据：从 GStreamer Buffer -> ROS Msg Vector。
 *    注意：这一步拷贝在当前架构下通常是必须的，因为 GStreamer Buffer 的生命周期由 GStreamer 管理，
 *    而 ROS 消息需要拥有自己的数据副本以确保生命周期安全。
 *    但在发布后，ROS 内部使用的是 UniquePtr 传递，后续节点（如 rk_inference）收到的是指针，
 *    不会再次发生拷贝。
 */
GstFlowReturn RkCamera::on_new_sample(GstElement* sink)
{
    // 1. 获取样本 (Pull sample)
    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (!sample) {
        RCLCPP_WARN(this->get_logger(), "无法获取样本 (pull sample 失败)");
        return GST_FLOW_ERROR;
    }

    // 调试日志
    static int frame_count = 0;
    if (++frame_count % 30 == 0) {
        if (debug_fps_) {
            RCLCPP_INFO(this->get_logger(), "正在处理第 %d 帧", frame_count);
        }
    }

    // 2. 获取 Buffer (Get buffer)
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    // 3. 映射内存 (Map memory)
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        // 4. 创建 UniquePtr 消息 (Create UniquePtr Msg)
        // 使用 make_unique 创建，完全符合文档中关于零拷贝发布的建议。
        auto msg = std::make_unique<sensor_msgs::msg::Image>();
        
        msg->header.stamp = this->now();
        msg->header.frame_id = "camera_link";
        msg->height = height_;
        msg->width = width_;
        msg->encoding = "nv12"; // 关键：标识数据格式为 NV12
        msg->is_bigendian = false;
        msg->step = width_;     // NV12 Y平面步长 = 宽度

        // 5. 数据填充 (Fill data)
        // 这里的逻辑已更新为使用 RGA 硬件进行旋转拷贝 (Hardware Rotate + Copy)
        msg->data.resize(map.size);
        
        rga_buffer_t src = {};
        src.vir_addr = (void*)map.data;
        src.fd = -1;
        src.width = width_;
        src.height = height_;
        src.wstride = width_;
        src.hstride = height_;
        src.format = RK_FORMAT_YCbCr_420_SP;

        rga_buffer_t dst = {};
        dst.vir_addr = (void*)&msg->data[0];
        dst.fd = -1;
        dst.width = width_;
        dst.height = height_;
        dst.wstride = width_;
        dst.hstride = height_;
        dst.format = RK_FORMAT_YCbCr_420_SP;

        // 硬件旋转 180 度 (Hardware Rotation 180 degree)
        if (rotation_180_) {
            int ret = imrotate(src, dst, IM_HAL_TRANSFORM_ROT_180);
            if (ret <= 0) {
                 RCLCPP_ERROR(this->get_logger(), "RGA imrotate failed: %d, fallback to memcpy", ret);
                 memcpy(&msg->data[0], map.data, map.size);
            }
        } else {
            // 直接拷贝 (No rotation)
            // 虽然可以直接 memcpy，但如果未来想做其他 RGA 操作（如 crop/resize），可以继续用 imcopy
            // 这里为了简单和效率，直接 memcpy
            memcpy(&msg->data[0], map.data, map.size);
        }

        // 6. 发布消息 (Publish)
        // 关键点：使用 std::move(msg) 转移所有权。
        // 当使用 ComposableNodeContainer 时，后续节点收到的是这个指针，无需再次拷贝数据。
        pub_->publish(std::move(msg));
        
        gst_buffer_unmap(buffer, &map);
    }
    
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

// 注册组件
RCLCPP_COMPONENTS_REGISTER_NODE(RkCamera)
