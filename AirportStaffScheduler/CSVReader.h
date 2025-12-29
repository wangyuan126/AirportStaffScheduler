/**
 * @file CSVReader.h
 * @brief CSV文件读取工具
 * 
 * 提供从CSV文件读取数据并转换为算法所需格式的工具函数
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace AirportStaffScheduler {
namespace CSVUtils {

/**
 * @brief 解析CSV行
 * @param line CSV行字符串
 * @return 解析后的字段列表
 */
inline std::vector<std::string> parseCSVLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;
    
    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        
        if (c == '"') {
            // 处理转义的引号
            if (in_quotes && i + 1 < line.length() && line[i + 1] == '"') {
                field += '"';
                ++i;
            } else {
                in_quotes = !in_quotes;
            }
        } else if (c == ',' && !in_quotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(field);  // 最后一个字段
    
    return fields;
}

/**
 * @brief 读取CSV文件
 * @param filename CSV文件路径
 * @param skip_header 是否跳过第一行（表头）
 * @return 行列表，每行是一个字段列表
 */
inline std::vector<std::vector<std::string>> readCSV(const std::string& filename, bool skip_header = true) {
    std::vector<std::vector<std::string>> rows;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "错误：无法打开CSV文件: " << filename << std::endl;
        return rows;
    }
    
    std::string line;
    bool first_line = true;
    
    while (std::getline(file, line)) {
        // 跳过BOM（如果存在）
        if (first_line && line.length() >= 3 && 
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line = line.substr(3);
        }
        
        // 去除行尾的\r（Windows换行符）
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (skip_header && first_line) {
            first_line = false;
            continue;
        }
        
        if (!line.empty()) {
            rows.push_back(parseCSVLine(line));
        }
        
        first_line = false;
    }
    
    file.close();
    return rows;
}

/**
 * @brief 将字段列表转换为映射表（使用表头作为键）
 * @param header 表头字段列表
 * @param rows 数据行列表
 * @return 每行数据映射到字段名的映射表列表
 */
inline std::vector<std::map<std::string, std::string>> csvToMap(
    const std::vector<std::string>& header,
    const std::vector<std::vector<std::string>>& rows) {
    
    std::vector<std::map<std::string, std::string>> result;
    
    for (const auto& row : rows) {
        std::map<std::string, std::string> row_map;
        for (size_t i = 0; i < header.size() && i < row.size(); ++i) {
            row_map[header[i]] = row[i];
        }
        result.push_back(row_map);
    }
    
    return result;
}

/**
 * @brief 去除字符串首尾空格和引号
 */
inline std::string trimQuotes(const std::string& str) {
    std::string result = str;
    
    // 去除首尾空格
    while (!result.empty() && (result.front() == ' ' || result.front() == '\t')) {
        result.erase(result.begin());
    }
    while (!result.empty() && (result.back() == ' ' || result.back() == '\t')) {
        result.pop_back();
    }
    
    // 去除首尾引号
    if (result.length() >= 2 && result.front() == '"' && result.back() == '"') {
        result = result.substr(1, result.length() - 2);
    }
    
    return result;
}

/**
 * @brief 解析时间字符串（格式：YYYY-MM-DD HH:MM:SS 或 HH:MM）
 * @param time_str 时间字符串
 * @return 从当天00:00:00开始的秒数（只计算小时和分钟，忽略日期）
 * @note 这个函数只返回从当天00:00:00开始的秒数，不包含日期偏移
 */
inline long parseDateTimeString(const std::string& time_str) {
    std::string trimmed = trimQuotes(time_str);
    if (trimmed.empty()) {
        return 0;
    }
    
    // 如果是 "航后" 或类似标识
    if (trimmed.find("航后") != std::string::npos) {
        return -1;
    }
    
    // 尝试解析 YYYY-MM-DD HH:MM:SS 格式
    if (trimmed.find("-") != std::string::npos && trimmed.find(":") != std::string::npos) {
        // 提取时间部分
        size_t space_pos = trimmed.find(" ");
        if (space_pos != std::string::npos) {
            std::string time_part = trimmed.substr(space_pos + 1);
            size_t colon_pos = time_part.find(":");
            if (colon_pos != std::string::npos) {
                int hours = std::stoi(time_part.substr(0, colon_pos));
                size_t colon_pos2 = time_part.find(":", colon_pos + 1);
                if (colon_pos2 != std::string::npos) {
                    int minutes = std::stoi(time_part.substr(colon_pos + 1, colon_pos2 - colon_pos - 1));
                    int seconds = 0;
                    // 尝试解析秒数部分
                    if (colon_pos2 + 1 < time_part.length()) {
                        std::string secs_str = time_part.substr(colon_pos2 + 1);
                        if (!secs_str.empty()) {
                            try {
                                seconds = std::stoi(secs_str);
                            } catch (...) {
                                seconds = 0;
                            }
                        }
                    }
                    return hours * 3600 + minutes * 60 + seconds;
                }
            }
        }
    }
    
    // 尝试解析 HH:MM 格式
    size_t colon_pos = trimmed.find(":");
    if (colon_pos != std::string::npos) {
        int hours = std::stoi(trimmed.substr(0, colon_pos));
        int minutes = std::stoi(trimmed.substr(colon_pos + 1));
        return hours * 3600 + minutes * 60;
    }
    
    return 0;
}

} // namespace CSVUtils
} // namespace AirportStaffScheduler

