#!/bin/bash
# scripts/setup_compiler.sh
# 注册 gcc 和 clang 到 update-alternatives 系统，以便一键切换。

set -e

# 确保以 sudo 运行
if [ "$EUID" -ne 0 ]; then
  echo "请使用 sudo 运行此脚本"
  exit 1
fi

echo "开始配置编译器 update-alternatives..."

# 1. 注册 clang-21 和 lld-21
echo "配置 Clang 21 和 LLD 21..."
update-alternatives --install /usr/bin/clang clang /usr/bin/clang-21 210 \
    --slave /usr/bin/clang++ clang++ /usr/bin/clang++-21 \
    --slave /usr/bin/ld.lld ld.lld /usr/bin/ld.lld-21

# 2. 注册 ld (链接器)
echo "配置 ld (链接器)..."
# 注册 ld.lld-21 和 ld.bfd 为 ld 的选项
update-alternatives --install /usr/bin/ld ld /usr/bin/ld.lld-21 210
update-alternatives --install /usr/bin/ld ld /usr/bin/ld.bfd 100

# 3. 注册 cc -> clang/gcc
echo "配置 cc (C 编译器)..."
update-alternatives --install /usr/bin/cc cc /usr/bin/clang 210 \
    --slave /usr/share/man/man1/cc.1.gz cc.1.gz /usr/share/man/man1/clang.1.gz
update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 100 \
    --slave /usr/share/man/man1/cc.1.gz cc.1.gz /usr/share/man/man1/gcc.1.gz

# 4. 注册 c++ -> clang++/g++
echo "配置 c++ (C++ 编译器)..."
update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 210 \
    --slave /usr/share/man/man1/c++.1.gz c++.1.gz /usr/share/man/man1/clang++.1.gz
update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 100 \
    --slave /usr/share/man/man1/c++.1.gz c++.1.gz /usr/share/man/man1/g++.1.gz

echo "配置完成。"
echo "默认已设置为 Clang 21 + LLD 21 (优先级最高)。"
echo "你可以使用 ./scripts/switch_compiler.sh <clang|gcc> 来切换。"

# 验证当前版本
echo "--------------------------------"
echo "当前 cc 版本:"
cc --version | head -n 1
echo "当前 c++ 版本:"
c++ --version | head -n 1
echo "当前 ld 版本:"
ld --version | head -n 1
echo "--------------------------------"
