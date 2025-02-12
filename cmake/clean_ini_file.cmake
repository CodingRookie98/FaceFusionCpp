if (NOT INI_FILE_PATH)
    message(FATAL_ERROR "INI_FILE_PATH is empty.")
endif ()

message(STATUS "Cleaning ini file...")
if (EXISTS ${INI_FILE_PATH})
    message(STATUS "Cleaning ini file : ${INI_FILE_PATH}")
    file(READ ${INI_FILE_PATH} file_content)
    # 将多行内容转换为以;分隔的列表
    string(REGEX REPLACE "\r?\n" ";" lines "${file_content}")
    # 初始化一个空列表来存储处理后的行
    set(processed_lines "")
    foreach (line IN LISTS lines)
        # 去除行首和行尾的空白字符
        string(STRIP "${line}" stripped_line)
        # 如果行 "#" 开头，则跳过（注释行）
        if (stripped_line MATCHES "^[#]")
            continue()
        endif ()
        # 删除最后一个=后的内容
        string(REGEX REPLACE "((.*)=).*" "\\1" stripped_line "${stripped_line}")
        # 将处理后的行添加到列表中
        list(APPEND processed_lines "${stripped_line}")
    endforeach ()
    # 将处理后的行重新组合成字符串
    string(REPLACE ";" "\n" processed_content "${processed_lines}")
    # 将处理后的内容写回源文件
    file(WRITE ${INI_FILE_PATH} "${processed_content}")
else ()
    message(FATAL_ERROR "File not exists : ${INI_FILE_PATH}")
endif ()