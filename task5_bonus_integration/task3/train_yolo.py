# -*- coding: utf-8 -*-
# 题目3 YOLOv8 训练脚本
# 用法：python train_yolo.py [epochs]   （默认 30，CPU 可跑）
# 流程：准备数据集 -> 训练 yolov8n -> 导出 ONNX 到 task3/armor.onnx（供 myYolo 推理）
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).parent
YAML = ROOT.parent / "datasets" / "armor.yaml"


def main():
    epochs = int(sys.argv[1]) if len(sys.argv) > 1 else 30

    # 数据集未准备时自动运行准备脚本
    if not YAML.exists():
        subprocess.run([sys.executable,
                        str(ROOT.parent / "datasets" / "prepare_dataset.py")],
                       check=True)

    from ultralytics import YOLO
    model = YOLO("yolov8n.pt")  # 首次运行自动下载预训练权重
    model.train(data=str(YAML), epochs=epochs, imgsz=640, device="cpu",
                project=str(ROOT / "runs"), name="armor")

    best = ROOT / "runs" / "armor" / "weights" / "best.pt"
    onnx = YOLO(str(best)).export(format="onnx", imgsz=640, simplify=True)
    shutil.copy(str(onnx), str(ROOT / "armor.onnx"))
    print("exported:", ROOT / "armor.onnx")


if __name__ == "__main__":
    main()
