# -*- coding: utf-8 -*-
# 生成题2验证用测试视频：移动的蓝色装甲板（双灯条+数字5），中途短暂遮挡
# 覆盖验证点：Kalman 跟踪状态机、数字多帧投票、测距、运动速度箭头、丢失预测与恢复
# 用法：python tools/make_test_video.py [输出路径]
import sys
import cv2
import numpy as np

W, H, FPS, N = 900, 600, 30, 150
OUT = sys.argv[1] if len(sys.argv) > 1 else "test_video.mp4"


def draw_armor(img, cx, cy, num="5"):
    """在 (cx,cy) 处画一块蓝色装甲板：灯条间距140px，灯条长57px
    宽高比 140/57 = 2.45 与真实小装甲板 135/55 = 2.45 一致"""
    blue = (255, 100, 0)
    cv2.rectangle(img, (cx - 80, cy - 28), (cx - 60, cy + 28), blue, -1)  # 左灯条
    cv2.rectangle(img, (cx + 60, cy - 28), (cx + 80, cy + 28), blue, -1)  # 右灯条
    cv2.putText(img, num, (cx - 12, cy + 18), cv2.FONT_HERSHEY_SIMPLEX,
                2.0, blue, 3)


def main():
    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    out = cv2.VideoWriter(OUT, fourcc, FPS, (W, H))
    if not out.isOpened():
        raise RuntimeError("cannot open VideoWriter: " + OUT)

    for i in range(N):
        frame = np.zeros((H, W, 3), np.uint8)
        t = i / (N - 1)
        cx = int(150 + 600 * t)                     # 匀速右移
        cy = 300 + int(40 * np.sin(t * 2 * np.pi))  # 轻微上下浮动
        occluded = 60 <= i <= 66                    # 模拟 7 帧遮挡
        if not occluded:
            draw_armor(frame, cx, cy)
        out.write(frame)

    out.release()
    print("saved", OUT, f"({N} frames @ {FPS}fps)")


if __name__ == "__main__":
    main()
