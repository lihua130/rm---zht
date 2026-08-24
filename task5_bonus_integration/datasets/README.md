# 装甲板训练数据说明

题目 3和题目 5交付包中的 `datasets/armor`包含处理后的 YOLO检测数据：

- 训练集：1138张图片及1138个标签
- 验证集：126张图片及126个标签
- 类别：`0 = armor`
- 标签格式：`class cx cy width height`，坐标均归一化

数据来源为 TAber-W/RobomasterDataset中的三个四点标注子集：

- `inside_record_group_1`
- `inside_record_group_2`
- `outside_record_group_1`

`prepare_dataset.py`将四点标注转换为外接矩形，使用固定随机种子划分训练集和验证集。为了避免在压缩包中重复存放相同图片，交付包仅包含转换后的可训练版本，不重复包含原始四点格式副本。

配置文件为 `armor.yaml`。文件不设置全局 `path`，训练集和验证集路径写为 `armor/images/...`；Ultralytics会以该 YAML所在的 `datasets`目录为基准，解压到任意目录后均可使用。
