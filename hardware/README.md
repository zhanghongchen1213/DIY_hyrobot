# SolidWorks 硬件资源

硬件由本仓库的 [hardware-2026-10-13 Release](https://github.com/zhanghongchen1213/DIY_hyrobot/releases/tag/hardware-2026-10-13) 提供 `Solidworks-2026-10-13.7z`。采用 7z / LZMA2、Ultra (`-mx=9`)、512 MiB 字典设置（7-Zip 根据数据量将归档实际字典设为 384 MiB，覆盖整个原目录的数据规模）、273 字节匹配和 solid 模式进行无损压缩；保留原始 125 个文件、中文名称和目录关系。解压前后逐文件 SHA-256 校验。

```bash
# 在 DIY_hyrobot 根目录执行；需安装 7-Zip
python3 tools/download_resources.py --group Solidworks
python3 tools/download_resources.py --group Solidworks --verify-only
```

解压结果在仓库根目录 `Solidworks/`，其中总装文件是 `Solidworks/黄油机器人/黄油机器人总装.SLDASM`。保持所有零件与装配目录关系。此归档是目录原件的无损备份；未在 SolidWorks 应用中进行 Pack and Go 或验证装配引用，若原模型含目录外引用仍需补齐。

仓库的 `resources/resources-manifest.json` 提供归档及每个原始文件的尺寸、路径和 SHA-256。文件过大，使用同一仓库的 Releases 保存，避免超过 GitHub 的 100 MiB 单文件限制。

最终归档为 287,911,690 字节，比原目录减少约 17.2%。已比较 64 MiB 与 512 MiB 字典，保留后者较小的结果。
