# -*- coding: utf-8 -*-
# 题目3 数据集准备脚本
# 将 TAber-W/RobomasterDataset（yolo 四点格式）转换为 ultralytics 检测格式：
#   class cx cy w h（归一化外接矩形），类别统一映射为 0=armor
# 输出目录：datasets/armor/{images,labels}/{train,val}，并生成 datasets/armor.yaml
import random
import shutil
from pathlib import Path

HERE = Path(__file__).parent
SRC = HERE / "RobomasterDataset" / "yolo_四点格式"
SUBSETS = ["inside_record_group_1", "inside_record_group_2", "outside_record_group_1"]
DST = HERE / "armor"
VAL_RATIO = 0.1


def convert_label(txt_path):
    """四点格式(class + x1y1x2y2x3y3x4y4) -> 检测框格式(0 cx cy w h)"""
    boxes = []
    for line in txt_path.read_text(encoding="utf-8", errors="ignore").splitlines():
        parts = line.split()
        if len(parts) != 9:
            continue
        vals = list(map(float, parts[1:]))
        xs, ys = vals[0::2], vals[1::2]
        x1, x2, y1, y2 = min(xs), max(xs), min(ys), max(ys)
        cx, cy, w, h = (x1 + x2) / 2, (y1 + y2) / 2, x2 - x1, y2 - y1
        if w > 0 and h > 0:
            boxes.append(f"0 {cx:.6f} {cy:.6f} {w:.6f} {h:.6f}")
    return boxes


def main():
    pairs = []
    for sub in SUBSETS:
        img_dir, lbl_dir = SRC / sub / "images", SRC / sub / "labels"
        if not img_dir.exists():
            continue
        for img in img_dir.iterdir():
            if img.suffix.lower() in (".jpg", ".png"):
                lbl = lbl_dir / (img.stem + ".txt")
                if lbl.exists():
                    pairs.append((img, lbl))

    random.seed(42)
    random.shuffle(pairs)
    n_val = max(1, int(len(pairs) * VAL_RATIO))
    splits = {"val": pairs[:n_val], "train": pairs[n_val:]}

    total = 0
    for split, items in splits.items():
        img_out = DST / "images" / split
        lbl_out = DST / "labels" / split
        img_out.mkdir(parents=True, exist_ok=True)
        lbl_out.mkdir(parents=True, exist_ok=True)
        for img, lbl in items:
            boxes = convert_label(lbl)
            if not boxes:
                continue  # 无有效标注的图片跳过
            name = f"{img.parent.parent.name}_{img.name}"  # 子集前缀防重名
            shutil.copy(img, img_out / name)
            (lbl_out / (Path(name).stem + ".txt")).write_text(
                "\n".join(boxes) + "\n", encoding="utf-8")
            total += 1
        print(f"{split}: {len(list(img_out.iterdir()))} images")

    yaml = DST.parent / "armor.yaml"
    yaml.write_text(
        "train: armor/images/train\n"
        "val: armor/images/val\n"
        "names:\n  0: armor\n",
        encoding="utf-8")
    print(f"total: {total}, yaml -> {yaml}")


if __name__ == "__main__":
    main()
