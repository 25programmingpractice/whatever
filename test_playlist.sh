#!/bin/bash
# 播放列表功能测试脚本

echo "=== Whatever 音乐播放器测试 ==="
echo "检查音乐文件..."

MUSIC_DIR="/Users/yhx/Documents/ZjmTempMusic"
if [ -d "$MUSIC_DIR" ]; then
    echo "✅ 音乐目录存在: $MUSIC_DIR"
    echo "音乐文件列表:"
    ls -la "$MUSIC_DIR"
else
    echo "❌ 音乐目录不存在: $MUSIC_DIR"
fi

echo ""
echo "检查构建文件..."
if [ -f "./build/whatever.app/Contents/MacOS/whatever" ]; then
    echo "✅ 应用程序已构建"
else
    echo "❌ 应用程序未找到，请先构建项目"
fi

echo ""
echo "=== 功能测试说明 ==="
echo "1. 启动应用程序后，播放列表应该显示音乐文件"
echo "2. 双击任意歌曲开始播放"
echo "3. 使用'上一曲'和'下一曲'按钮测试播放控制"
echo "4. 验证时间显示和进度条功能"
echo "5. 测试音量控制"

echo ""
echo "启动应用程序进行测试..."
