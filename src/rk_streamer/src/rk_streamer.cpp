#include "rk_streamer.h"
#include <rclcpp_components/register_node_macro.hpp>

RkStreamer::RkStreamer(const rclcpp::NodeOptions & options)
: Node("rk_streamer", options)
{
    // 声明参数
    this->declare_parameter("width", 1920);
    this->declare_parameter("height", 1080);
    this->declare_parameter("fps", 30);
    this->declare_parameter("sub_topic", "/yolo/image_infer");
    
    // GStreamer 管道配置
    // 恢复 MPP 硬件编码
    // 策略：appsrc -> videoconvert -> mpph264enc -> h264parse -> rtspclientsink
    // 移除 protocols=tcp 以允许自动协商，添加 async=false 避免状态阻塞
    // 更新：强制使用 protocols=tcp 以解决 go2rtc 连接问题，并显式设置 defaults
    // 修正：移除不支持的 async=false 属性，恢复 config-interval=-1 (IDR 帧发送 SPS/PPS)
    std::string default_pipeline = "appsrc name=source ! video/x-raw,format=NV12 ! videoconvert ! mpph264enc ! h264parse config-interval=-1 ! rtspclientsink location=rtsp://127.0.0.1:8554/camera protocols=tcp";
    this->declare_parameter("pipeline", default_pipeline);

    // 获取参数
    width_ = this->get_parameter("width").as_int();
    height_ = this->get_parameter("height").as_int();
    fps_ = this->get_parameter("fps").as_int();
    std::string topic = this->get_parameter("sub_topic").as_string();
    pipeline_str_ = this->get_parameter("pipeline").as_string();

    RCLCPP_INFO(this->get_logger(), "初始化 RkStreamer, 订阅: %s", topic.c_str());
    RCLCPP_INFO(this->get_logger(), "GStreamer 管道配置 (生效值): %s", pipeline_str_.c_str());

    // 创建订阅者
    sub_ = image_transport::create_subscription(
        this, 
        topic, 
        std::bind(&RkStreamer::image_callback, this, std::placeholders::_1), 
        "raw", 
        rmw_qos_profile_sensor_data
    );

    // 初始化 GStreamer
    if (!gst_is_initialized()) {
        gst_init(nullptr, nullptr);
    }

    // 启动 GLib 主循环 (RTSP 插件依赖此循环处理网络 I/O)
    main_loop_ = g_main_loop_new(nullptr, FALSE);
    main_loop_thread_ = std::thread([this]() {
        g_main_loop_run(main_loop_);
    });

    // 尝试初始化
    if (width_ > 0 && height_ > 0) {
        init_gst_pipeline();
    }
}

RkStreamer::~RkStreamer()
{
    if (pipeline_obj_) {
        gst_element_set_state(pipeline_obj_, GST_STATE_NULL);
        gst_object_unref(pipeline_obj_);
    }

    if (main_loop_) {
        g_main_loop_quit(main_loop_);
        if (main_loop_thread_.joinable()) {
            main_loop_thread_.join();
        }
        g_main_loop_unref(main_loop_);
    }
}

void RkStreamer::init_gst_pipeline()
{
    if (is_initialized_) return;

    GError *error = nullptr;
    
    // 1. 解析管道字符串
    std::string final_pipeline = pipeline_str_;
    if (final_pipeline.find("name=source") == std::string::npos) {
        size_t pos = final_pipeline.find("appsrc");
        if (pos != std::string::npos) {
            final_pipeline.replace(pos, 6, "appsrc name=source");
            RCLCPP_WARN(this->get_logger(), "自动为 appsrc 添加 name=source: %s", final_pipeline.c_str());
        }
    }

    pipeline_obj_ = gst_parse_launch(final_pipeline.c_str(), &error);
    
    if (error) {
        RCLCPP_ERROR(this->get_logger(), "GStreamer 管道解析失败: %s", error->message);
        g_error_free(error);
        return;
    }

    if (!pipeline_obj_) {
        RCLCPP_ERROR(this->get_logger(), "无法创建 GStreamer 管道对象");
        return;
    }

    // 2. 获取 appsrc 元素
    appsrc_ = gst_bin_get_by_name(GST_BIN(pipeline_obj_), "source");
    if (!appsrc_) {
        RCLCPP_ERROR(this->get_logger(), "无法在管道中找到 'source' (appsrc) 元素");
        gst_object_unref(pipeline_obj_);
        pipeline_obj_ = nullptr;
        return;
    }

    // [关键修正] 将 Pipeline 的消息总线挂载到 GLib 主循环
    // 如果不挂载，RTSP 插件的网络事件将无法被 MainLoop 处理
    GstBus *bus = gst_element_get_bus(pipeline_obj_);
    gst_bus_add_watch(bus, [](GstBus *bus, GstMessage *msg, gpointer data) -> gboolean {
        RkStreamer *node = (RkStreamer *)data;
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR: {
                GError *err;
                gchar *debug;
                gst_message_parse_error(msg, &err, &debug);
                RCLCPP_ERROR(node->get_logger(), "Pipeline Error: %s", err->message);
                if (debug) {
                    RCLCPP_ERROR(node->get_logger(), "Debug Info: %s", debug);
                }
                g_error_free(err);
                g_free(debug);
                break;
            }
            case GST_MESSAGE_WARNING: {
                GError *err;
                gchar *debug;
                gst_message_parse_warning(msg, &err, &debug);
                RCLCPP_WARN(node->get_logger(), "Pipeline Warning: %s", err->message);
                if (debug) {
                    RCLCPP_WARN(node->get_logger(), "Debug Info: %s", debug);
                }
                g_error_free(err);
                g_free(debug);
                break;
            }
            default:
                break;
        }
        return TRUE;
    }, this);
    gst_object_unref(bus);

    // 3. 配置 appsrc
    // 显式设置 caps，确保 appsrc 知道输入数据的确切格式
    // 关键修正：设置 is-live=true 和 format=TIME，对齐 gst-launch 行为
    g_object_set(G_OBJECT(appsrc_), 
        "stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
        "format", GST_FORMAT_TIME,
        "is-live", TRUE,
        "do-timestamp", TRUE, // 让 appsrc 辅助处理时间戳
        NULL);
    
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
        "format", G_TYPE_STRING, "NV12",
        "width", G_TYPE_INT, width_,
        "height", G_TYPE_INT, height_,
        "framerate", GST_TYPE_FRACTION, fps_, 1,
        NULL);
    gst_app_src_set_caps(GST_APP_SRC(appsrc_), caps);
    gst_caps_unref(caps);

    // 4. 启动管道
    GstStateChangeReturn ret = gst_element_set_state(pipeline_obj_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        RCLCPP_ERROR(this->get_logger(), "无法将管道设置为 PLAYING 状态");
        gst_object_unref(pipeline_obj_); // appsrc_ 由 pipeline 管理，无需单独 unref
        pipeline_obj_ = nullptr;
        appsrc_ = nullptr;
        return;
    }

    RCLCPP_INFO(this->get_logger(), "GStreamer 管道已启动");
    is_initialized_ = true;
}

void RkStreamer::image_callback(const sensor_msgs::msg::Image::ConstSharedPtr & msg)
{
    // 延迟初始化
    if (!is_initialized_) {
        if (width_ <= 0) width_ = msg->width;
        if (height_ <= 0) height_ = msg->height;
        init_gst_pipeline();
    }

    if (!is_initialized_ || !appsrc_) {
        return;
    }

    // 检查数据大小 (NV12: w * h * 1.5)
    size_t expected_size = width_ * height_ * 3 / 2;
    if (msg->data.size() < expected_size) {
        RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, 
            "数据大小不足 NV12: expected %zu, got %zu", expected_size, msg->data.size());
        return;
    }

    // [调试] 打印接收到的图像信息和时间戳
    RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 5000,
        "接收图像: %dx%d, size=%zu, encoding=%s", msg->width, msg->height, msg->data.size(), msg->encoding.c_str());



    // 创建 GstBuffer
    // 注意：这里需要拷贝数据，因为 ROS 消息内存生命周期由 ROS 管理，而 GST 需要自己的 buffer
    // 或者可以使用 gst_buffer_new_wrapped_full 配合 destroy notify，但拷贝最安全
    GstBuffer *buffer = gst_buffer_new_allocate(NULL, expected_size, NULL);
    if (!buffer) {
        RCLCPP_ERROR(this->get_logger(), "无法分配 GstBuffer");
        return;
    }

    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        memcpy(map.data, msg->data.data(), expected_size);
        gst_buffer_unmap(buffer, &map);
    } else {
        RCLCPP_ERROR(this->get_logger(), "无法映射 GstBuffer 内存");
        gst_buffer_unref(buffer);
        return;
    }

    // 设置时间戳
    // GStreamer 对时间戳非常敏感，必须设置 PTS 和 Duration
    // 使用内置计数器生成连续的时间戳，避免因 ROS 消息抖动导致丢帧
    static GstClockTime timestamp = 0;
    GstClockTime duration = gst_util_uint64_scale_int(1, GST_SECOND, fps_);

    GST_BUFFER_PTS(buffer) = timestamp;
    GST_BUFFER_DTS(buffer) = timestamp;
    GST_BUFFER_DURATION(buffer) = duration;

    // [调试] 打印 buffer 时间戳信息
    RCLCPP_DEBUG(this->get_logger(), "Push buffer: pts=%lu, duration=%lu", timestamp, duration);

    timestamp += duration;

    // 推送数据
    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc_), buffer);
    if (ret != GST_FLOW_OK) {
        RCLCPP_WARN(this->get_logger(), "gst_app_src_push_buffer 失败: %d", ret);
    } else {
        // [调试] 推送成功
        RCLCPP_DEBUG(this->get_logger(), "Buffer pushed successfully");
    }
}

RCLCPP_COMPONENTS_REGISTER_NODE(RkStreamer)
