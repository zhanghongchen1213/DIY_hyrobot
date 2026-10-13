import cv2
import numpy as np
import os
from rknnlite.api import RKNNLite

# -------------------------------------------------------------------------
# 配置参数
# -------------------------------------------------------------------------
OBJ_THRESH = 0.25
NMS_THRESH = 0.45
IMG_SIZE = (640, 640)  # 模型输入尺寸
CLASSES = ['butter']   # 类别名称

# 获取当前脚本所在目录
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
RKNN_MODEL = os.path.join(SCRIPT_DIR, 'yolov8.rknn')
IMG_PATH = os.path.join(SCRIPT_DIR, 'butter.jpg')

# -------------------------------------------------------------------------
# 后处理函数 (Numpy 实现，无需 PyTorch)
# -------------------------------------------------------------------------

def dfl(position):
    # Distribution Focal Loss (DFL) - Numpy implementation
    # position shape: (n, c, h, w) -> (n, 4, 16, h, w)
    n, c, h, w = position.shape
    p_num = 4
    mc = c // p_num
    y = position.reshape(n, p_num, mc, h, w)
    
    # Softmax along dimension 2
    y_max = np.max(y, axis=2, keepdims=True)
    y_exp = np.exp(y - y_max)
    y_softmax = y_exp / np.sum(y_exp, axis=2, keepdims=True)
    
    # Weighted sum
    acc_metrix = np.arange(mc, dtype=np.float32).reshape(1, 1, mc, 1, 1)
    y = (y_softmax * acc_metrix).sum(axis=2)
    return y

def box_process(position):
    # 处理边界框分支
    grid_h, grid_w = position.shape[2:4]
    col, row = np.meshgrid(np.arange(0, grid_w), np.arange(0, grid_h))
    col = col.reshape(1, 1, grid_h, grid_w)
    row = row.reshape(1, 1, grid_h, grid_w)
    grid = np.concatenate((col, row), axis=1)
    stride = np.array([IMG_SIZE[1]//grid_h, IMG_SIZE[0]//grid_w]).reshape(1,2,1,1)

    position = dfl(position)
    box_xy  = grid + 0.5 - position[:,0:2,:,:]
    box_xy2 = grid + 0.5 + position[:,2:4,:,:]
    xyxy = np.concatenate((box_xy*stride, box_xy2*stride), axis=1)

    return xyxy

def filter_boxes(boxes, box_confidences, box_class_probs):
    # 根据阈值过滤框
    box_confidences = box_confidences.reshape(-1)
    candidate, class_num = box_class_probs.shape

    class_max_score = np.max(box_class_probs, axis=-1)
    classes = np.argmax(box_class_probs, axis=-1)

    _class_pos = np.where(class_max_score * box_confidences >= OBJ_THRESH)
    scores = (class_max_score * box_confidences)[_class_pos]

    boxes = boxes[_class_pos]
    classes = classes[_class_pos]

    return boxes, classes, scores

def nms_boxes(boxes, scores):
    # 非极大值抑制 (NMS)
    x = boxes[:, 0]
    y = boxes[:, 1]
    w = boxes[:, 2] - boxes[:, 0]
    h = boxes[:, 3] - boxes[:, 1]

    areas = w * h
    order = scores.argsort()[::-1]

    keep = []
    while order.size > 0:
        i = order[0]
        keep.append(i)

        xx1 = np.maximum(x[i], x[order[1:]])
        yy1 = np.maximum(y[i], y[order[1:]])
        xx2 = np.minimum(x[i] + w[i], x[order[1:]] + w[order[1:]])
        yy2 = np.minimum(y[i] + h[i], y[order[1:]] + h[order[1:]])

        w1 = np.maximum(0.0, xx2 - xx1 + 0.00001)
        h1 = np.maximum(0.0, yy2 - yy1 + 0.00001)
        inter = w1 * h1

        ovr = inter / (areas[i] + areas[order[1:]] - inter)
        inds = np.where(ovr <= NMS_THRESH)[0]
        order = order[inds + 1]
    
    return np.array(keep)

def post_process(input_data):
    # 处理模型输出
    # 预期 input_data 是一个包含 6 个 numpy 数组的列表
    # [box_stride8, cls_stride8, box_stride16, cls_stride16, box_stride32, cls_stride32]
    
    boxes, scores, classes_conf = [], [], []
    default_branch = 3
    pair_per_branch = len(input_data) // default_branch
    
    for i in range(default_branch):
        boxes.append(box_process(input_data[pair_per_branch*i]))
        classes_conf.append(input_data[pair_per_branch*i+1])
        scores.append(np.ones_like(input_data[pair_per_branch*i+1][:,:1,:,:], dtype=np.float32))

    def sp_flatten(_in):
        ch = _in.shape[1]
        _in = _in.transpose(0,2,3,1)
        return _in.reshape(-1, ch)

    boxes = [sp_flatten(_v) for _v in boxes]
    classes_conf = [sp_flatten(_v) for _v in classes_conf]
    scores = [sp_flatten(_v) for _v in scores]

    boxes = np.concatenate(boxes)
    classes_conf = np.concatenate(classes_conf)
    scores = np.concatenate(scores)

    # 过滤
    boxes, classes, scores = filter_boxes(boxes, scores, classes_conf)

    # NMS
    nboxes, nclasses, nscores = [], [], []
    for c in set(classes):
        inds = np.where(classes == c)
        b = boxes[inds]
        c = classes[inds]
        s = scores[inds]
        keep = nms_boxes(b, s)

        if len(keep) != 0:
            nboxes.append(b[keep])
            nclasses.append(c[keep])
            nscores.append(s[keep])

    if not nclasses and not nscores:
        return None, None, None

    boxes = np.concatenate(nboxes)
    classes = np.concatenate(nclasses)
    scores = np.concatenate(nscores)

    return boxes, classes, scores

# -------------------------------------------------------------------------
# 图像处理辅助函数
# -------------------------------------------------------------------------

def letter_box(im, new_shape, pad_color=(0,0,0)):
    # 缩放并填充图片
    shape = im.shape[:2]  # [height, width]
    if isinstance(new_shape, int):
        new_shape = (new_shape, new_shape)

    r = min(new_shape[0] / shape[0], new_shape[1] / shape[1])
    ratio = r
    new_unpad = int(round(shape[1] * r)), int(round(shape[0] * r))
    dw, dh = new_shape[1] - new_unpad[0], new_shape[0] - new_unpad[1]
    dw /= 2
    dh /= 2

    if shape[::-1] != new_unpad:
        im = cv2.resize(im, new_unpad, interpolation=cv2.INTER_LINEAR)
    
    top, bottom = int(round(dh - 0.1)), int(round(dh + 0.1))
    left, right = int(round(dw - 0.1)), int(round(dw + 0.1))
    im = cv2.copyMakeBorder(im, top, bottom, left, right, cv2.BORDER_CONSTANT, value=pad_color)
    
    return im, ratio, (dw, dh)

def draw(image, boxes, scores, classes, ratio, pad_dw, pad_dh):
    for box, score, cl in zip(boxes, scores, classes):
        # 还原坐标
        x1, y1, x2, y2 = box
        
        # 去除 padding
        x1 -= pad_dw
        x2 -= pad_dw
        y1 -= pad_dh
        y2 -= pad_dh
        
        # 缩放回原图
        x1 /= ratio
        x2 /= ratio
        y1 /= ratio
        y2 /= ratio
        
        # 限制边界
        h, w = image.shape[:2]
        x1 = max(0, min(x1, w))
        x2 = max(0, min(x2, w))
        y1 = max(0, min(y1, h))
        y2 = max(0, min(y2, h))
        
        top, left, right, bottom = int(y1), int(x1), int(x2), int(y2)

        print("%s @ (%d %d %d %d) %.3f" % (CLASSES[cl], left, top, right, bottom, score))
        cv2.rectangle(image, (left, top), (right, bottom), (255, 0, 0), 2)
        cv2.putText(image, '{0} {1:.2f}'.format(CLASSES[cl], score),
                    (left, top - 6), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)

# -------------------------------------------------------------------------
# 主函数
# -------------------------------------------------------------------------

def main():
    # 1. 创建 RKNNLite 对象
    rknn_lite = RKNNLite()

    # 2. 加载 RKNN 模型
    print('--> 正在加载 RKNN 模型')
    ret = rknn_lite.load_rknn(RKNN_MODEL)
    if ret != 0:
        print('加载 RKNN 模型失败')
        exit(ret)
    print('完成')

    # 3. 初始化运行时环境
    print('--> 正在初始化运行时环境')
    ret = rknn_lite.init_runtime()
    if ret != 0:
        print('初始化运行时环境失败')
        exit(ret)
    print('完成')

    # 4. 读取并预处理图片
    print(f"当前工作目录: {os.getcwd()}")
    print(f"尝试从以下路径加载图像: {IMG_PATH}")
    
    img_src = cv2.imread(IMG_PATH)
    if img_src is None:
        print("错误: 加载图像失败。")
        exit(-1)

    # Letterbox 预处理
    img, ratio, (dw, dh) = letter_box(img_src.copy(), IMG_SIZE)
    
    # 转换颜色空间 BGR -> RGB (YOLO通常使用RGB训练)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    
    # 增加 Batch 维度
    input_data = np.expand_dims(img, axis=0)

    # 5. 推理
    print('--> 正在运行推理')
    outputs = rknn_lite.inference(inputs=[input_data])
    print('完成')
    
    # 打印输出形状以便调试
    print("DEBUG: 模型输出数量:", len(outputs))
    for i, o in enumerate(outputs):
        print(f"DEBUG: Output {i} shape: {o.shape}")

    # 6. 后处理
    # 检查输出数量
    # 6个输出: [box, cls] * 3 (standard)
    # 9个输出: [box, cls, score_sum] * 3 (with score_sum, ignore 3rd)
    if len(outputs) % 3 == 0:
        print(f"识别到 Split Head 模型格式 (outputs={len(outputs)})，使用 DFL 后处理...")
        boxes, classes, scores = post_process(outputs)
    else:
        print(f"警告: 输出数量 ({len(outputs)}) 不符合预期 (6 或 9)，可能模型导出格式不同。")
        print("尝试继续处理，但可能会失败...")
        # 尝试使用 post_process，看看能否运气好
        try:
            boxes, classes, scores = post_process(outputs)
        except Exception as e:
            print(f"处理失败: {e}")
            rknn_lite.release()
            exit(-1)

    # 7. 绘制结果
    if boxes is not None:
        draw(img_src, boxes, scores, classes, ratio, dw, dh)
        
        # 保存结果
        save_path = os.path.join(SCRIPT_DIR, 'result.jpg')
        cv2.imwrite(save_path, img_src)
        print(f'结果已保存至 {save_path}')
    else:
        print("未检测到目标。")

    # 8. 释放资源
    rknn_lite.release()

if __name__ == '__main__':
    main()
