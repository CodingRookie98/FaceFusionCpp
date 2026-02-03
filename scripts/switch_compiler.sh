#!/bin/bash
# scripts/switch_compiler.sh
# 一键切换系统默认编译器为 clang (Clang 21) 或 gcc (GCC 13)

set -e

if [ "$EUID" -ne 0 ]; then
  echo "请使用 sudo 运行此脚本"
  exit 1
fi

TARGET=$1

if [[ "$TARGET" != "clang" && "$TARGET" != "gcc" ]]; then
  echo "用法: $0 <clang|gcc>"
  exit 1
fi

if [[ "$TARGET" == "clang" ]]; then
  echo "正在切换到 Clang 21 + LLD 21..."
  update-alternatives --set clang /usr/bin/clang-21
  update-alternatives --set ld /usr/bin/ld.lld-21
  update-alternatives --set cc /usr/bin/clang
  update-alternatives --set c++ /usr/bin/clang++
else
  echo "正在切换到 GCC + BFD..."
  update-alternatives --set cc /usr/bin/gcc
  update-alternatives --set c++ /usr/bin/g++
  update-alternatives --set ld /usr/bin/ld.bfd
fi

echo "切换完成。"
echo "--------------------------------"
echo "当前 cc 版本:"
cc --version | head -n 1
echo "当前 c++ 版本:"
c++ --version | head -n 1
echo "当前 ld 版本:"
ld --version | head -n 1
echo "--------------------------------"
