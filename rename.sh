#!/bin/bash

# 指定源名字和新名字
source_name="OpNovice"
new_name="PMTLC"

# 使用find命令递归查找所有包含源名字的文件
find . -type f -name "*$source_name*" | while read file; do
    # 获取文件所在目录
    dir=$(dirname "$file")
    # 获取文件名
    base=$(basename "$file")
    # 替换文件名中的源名字为新名字
    new_base=${base/$source_name/$new_name}
    # 打印出可能修改的文件名
    echo "将重命名文件: $file -> $dir/$new_base"
done

# 提示用户确认
read -p "是否继续重命名这些文件? (Y/y 继续): " confirm

if [[ $confirm == [Yy] ]]; then
    # 再次遍历并重命名文件
    find . -type f -name "*$source_name*" | while read file; do
        dir=$(dirname "$file")
        base=$(basename "$file")
        new_base=${base/$source_name/$new_name}
        mv "$file" "$dir/$new_base"
    done
    echo "文件重命名完成。"
else
    echo "操作已取消。"
fi