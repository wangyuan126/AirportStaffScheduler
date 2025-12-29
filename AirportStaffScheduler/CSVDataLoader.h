/**
 * @file CSVDataLoader.h
 * @brief CSV数据加载器
 * 
 * 从CSV文件读取数据并转换为算法所需的数据结构
 */

#pragma once

#include "CSVReader.h"
#include "vip_first_class_algo/employee_info.h"
#include "vip_first_class_algo/shift.h"
#include "vip_first_class_algo/task_definition.h"
#include "zhuangxie_class/load_employee_info.h"
#include "zhuangxie_class/load_task.h"
#include "CommonAdapterUtils.h"
#include "Task.h"
#include "DateTimeUtils.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <ctime>

namespace AirportStaffScheduler {
namespace CSVLoader {

/**
 * @brief 根据岗位名称推断资质掩码
 * @param position 岗位名称（如"xa内场新人"、"xa配载内场"、"xa装机"等）
 * @return 资质掩码
 */
inline int inferQualificationFromPosition(const std::string& position) {
    std::string pos_lower = position;
    std::transform(pos_lower.begin(), pos_lower.end(), pos_lower.begin(), ::tolower);
    
    int mask = 0;
    
    // 根据岗位关键词推断资质
    if (pos_lower.find("内场") != std::string::npos || pos_lower.find("配载") != std::string::npos) {
        mask |= static_cast<int>(vip_first_class::QualificationMask::HALL_INTERNAL);
        mask |= static_cast<int>(vip_first_class::QualificationMask::FRONT_DESK);
    }
    
    if (pos_lower.find("装机") != std::string::npos) {
        mask |= static_cast<int>(vip_first_class::QualificationMask::EXTERNAL);
    }
    
    // 默认给予厅内资质
    if (mask == 0) {
        mask = static_cast<int>(vip_first_class::QualificationMask::HALL_INTERNAL) |
               static_cast<int>(vip_first_class::QualificationMask::FRONT_DESK);
    }
    
    return mask;
}

/**
 * @brief 从CSV文件加载员工数据（VIP/头等舱）
 * @param filename CSV文件路径
 * @return 员工信息列表
 */
inline std::vector<vip_first_class::EmployeeInfo> loadEmployeesFromCSV(const std::string& filename) {
    std::vector<vip_first_class::EmployeeInfo> employees;
    
    auto rows = CSVUtils::readCSV(filename, true);
    if (rows.empty()) {
        std::cerr << "警告：CSV文件为空或无法读取: " << filename << std::endl;
        return employees;
    }
    
    // 读取表头（第一行）
    std::ifstream header_file(filename);
    std::string header_line;
    if (std::getline(header_file, header_line)) {
        // 去除BOM
        if (header_line.length() >= 3 && 
            static_cast<unsigned char>(header_line[0]) == 0xEF &&
            static_cast<unsigned char>(header_line[1]) == 0xBB &&
            static_cast<unsigned char>(header_line[2]) == 0xBF) {
            header_line = header_line.substr(3);
        }
    }
    header_file.close();
    
    auto header = CSVUtils::parseCSVLine(header_line);
    auto data_map = CSVUtils::csvToMap(header, rows);
    
    for (const auto& row : data_map) {
        vip_first_class::EmployeeInfo emp;
        
        // 提取字段（去除引号）
        std::string emp_id = CSVUtils::trimQuotes(row.count("员工编号") ? row.at("员工编号") : "");
        std::string emp_name = CSVUtils::trimQuotes(row.count("员工姓名") ? row.at("员工姓名") : "");
        std::string position = CSVUtils::trimQuotes(row.count("岗位") ? row.at("岗位") : "");
        
        if (emp_id.empty()) {
            continue;  // 跳过无效数据
        }
        
        emp.setEmployeeId(emp_id);
        emp.setEmployeeName(emp_name);
        
        // 根据岗位推断资质（因为CSV中的资质是航空公司名称列表，不是算法需要的资质类型）
        int qualification_mask = inferQualificationFromPosition(position);
        emp.setQualificationMask(qualification_mask);
        
        employees.push_back(emp);
    }
    
    // std::cout << "从CSV加载了 " << employees.size() << " 个员工" << std::endl;  // 减少输出
    return employees;
}

/**
 * @brief 从CSV文件加载装卸员工数据
 * @param filename CSV文件路径
 * @return 装卸员工信息列表
 */
inline std::vector<zhuangxie_class::LoadEmployeeInfo> loadLoadEmployeesFromCSV(const std::string& filename) {
    std::vector<zhuangxie_class::LoadEmployeeInfo> employees;
    
    // 检查文件是否存在
    std::ifstream test_file(filename);
    if (!test_file.is_open()) {
        std::cerr << "ERROR: Cannot open CSV file: " << filename << std::endl;
        return employees;
    }
    test_file.close();
    
    auto rows = CSVUtils::readCSV(filename, true);
    if (rows.empty()) {
        std::cerr << "WARNING: CSV file is empty or cannot be read: " << filename << std::endl;
        return employees;
    }
    
    // 读取表头
    std::ifstream header_file(filename);
    std::string header_line;
    if (std::getline(header_file, header_line)) {
        if (header_line.length() >= 3 && 
            static_cast<unsigned char>(header_line[0]) == 0xEF &&
            static_cast<unsigned char>(header_line[1]) == 0xBB &&
            static_cast<unsigned char>(header_line[2]) == 0xBF) {
            header_line = header_line.substr(3);
        }
    }
    header_file.close();
    
    auto header = CSVUtils::parseCSVLine(header_line);
    
    // DEBUG: 输出原始表头
    std::cerr << "DEBUG: Raw header size: " << header.size() << std::endl;
    for (size_t i = 0; i < header.size(); ++i) {
        std::cerr << "DEBUG: Raw header[" << i << "]: [" << header[i] << "]" << std::endl;
    }
    
    // 清理表头，去除引号和空格，并创建原始列名到清理后列名的映射
    std::vector<std::string> cleaned_header;
    std::map<std::string, std::string> header_to_cleaned;  // 原始列名 -> 清理后的列名
    for (const auto& h : header) {
        std::string cleaned = CSVUtils::trimQuotes(h);
        cleaned_header.push_back(cleaned);
        header_to_cleaned[h] = cleaned;
        std::cerr << "DEBUG: Header mapping: [" << h << "] -> [" << cleaned << "]" << std::endl;
    }
    
    // 创建清理后列名到原始列名的反向映射（用于查找）
    std::map<std::string, std::string> cleaned_to_header;
    for (const auto& pair : header_to_cleaned) {
        cleaned_to_header[pair.second] = pair.first;
    }
    
    auto data_map = CSVUtils::csvToMap(header, rows);
    
    // 调试：检查表头（使用更宽松的匹配方式）
    bool has_emp_id = false, has_emp_name = false;
    std::string emp_id_key, emp_name_key;
    
    for (const auto& h : cleaned_header) {
        // 去除首尾空格后再比较
        std::string trimmed = h;
        while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t')) {
            trimmed.erase(trimmed.begin());
        }
        while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t')) {
            trimmed.pop_back();
        }
        
        if (trimmed == "员工编号" || trimmed.find("员工编号") != std::string::npos) {
            has_emp_id = true;
            // 需要找到对应的原始列名（带引号的）
            if (cleaned_to_header.count(h)) {
                emp_id_key = cleaned_to_header.at(h);
            } else {
                // 如果cleaned_to_header中没有，尝试从header中查找
                for (const auto& orig_h : header) {
                    if (CSVUtils::trimQuotes(orig_h) == h) {
                        emp_id_key = orig_h;
                        break;
                    }
                }
                if (emp_id_key.empty()) {
                    emp_id_key = h;  // 如果找不到，使用清理后的列名
                }
            }
        }
        if (trimmed == "员工姓名" || trimmed.find("员工姓名") != std::string::npos) {
            has_emp_name = true;
            if (cleaned_to_header.count(h)) {
                emp_name_key = cleaned_to_header.at(h);
            } else {
                for (const auto& orig_h : header) {
                    if (CSVUtils::trimQuotes(orig_h) == h) {
                        emp_name_key = orig_h;
                        break;
                    }
                }
                if (emp_name_key.empty()) {
            emp_name_key = h;
                }
            }
        }
    }
    
    // 如果精确匹配失败，尝试在cleaned_to_header中查找
    if (!has_emp_id) {
        for (const auto& pair : cleaned_to_header) {
            std::string key = pair.first;
            // 去除首尾空格
            while (!key.empty() && (key.front() == ' ' || key.front() == '\t')) {
                key.erase(key.begin());
            }
            while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) {
                key.pop_back();
            }
            if (key == "员工编号" || key.find("员工编号") != std::string::npos) {
                has_emp_id = true;
                emp_id_key = pair.second;
                break;
            }
        }
    }
    
    if (!has_emp_id) {
        // 尝试更宽松的匹配：直接在所有列名中查找
        for (const auto& h : cleaned_header) {
            std::string trimmed = h;
            // 去除首尾空格和引号
            while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t' || trimmed.front() == '"')) {
                trimmed.erase(trimmed.begin());
            }
            while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t' || trimmed.back() == '"')) {
                trimmed.pop_back();
            }
            
            // 检查是否包含"员工"和"编号"
            if ((trimmed.find("员工") != std::string::npos && trimmed.find("编号") != std::string::npos) ||
                trimmed == "员工编号") {
                has_emp_id = true;
                // 找到对应的原始列名
                for (const auto& pair : cleaned_to_header) {
                    if (CSVUtils::trimQuotes(pair.first) == trimmed || pair.first == h) {
                        emp_id_key = pair.second;
                        break;
                    }
                }
                if (emp_id_key.empty()) {
                    emp_id_key = h;
                }
                break;
            }
        }
    }
    
    if (!has_emp_id) {
        std::cerr << "ERROR: CSV file missing required column '员工编号' (employee ID)." << std::endl;
        return employees;
    }
    
    // 确保emp_id_key和emp_name_key有值
    if (emp_id_key.empty()) {
        // 最后尝试：直接使用cleaned_header中的值
        for (const auto& h : cleaned_header) {
            if (h.find("员工编号") != std::string::npos || h == "员工编号") {
                emp_id_key = cleaned_to_header.count(h) ? cleaned_to_header.at(h) : h;
                break;
            }
        }
    }
    if (emp_name_key.empty()) {
        for (const auto& h : cleaned_header) {
            if (h.find("员工姓名") != std::string::npos || h == "员工姓名") {
                emp_name_key = cleaned_to_header.count(h) ? cleaned_to_header.at(h) : h;
                break;
            }
        }
    }
    
    // 用于根据班组名映射到装卸组ID
    std::map<std::string, int> group_name_to_id;
    int next_group_id = 1;
    int skipped_rows = 0;
    
    for (const auto& row : data_map) {
        zhuangxie_class::LoadEmployeeInfo emp;
        
        // 使用清理后的列名查找原始列名，然后从row中获取值
        // 如果之前已经找到了key，直接使用；否则尝试查找
        std::string group_name_key = cleaned_to_header.count("班组名") ? cleaned_to_header.at("班组名") : "班组名";
        std::string position_key = cleaned_to_header.count("岗位") ? cleaned_to_header.at("岗位") : "岗位";
        
        // 尝试在row中直接查找（可能列名有变化）
        if (row.count(emp_id_key) == 0) {
            // 尝试其他可能的列名
            for (const auto& pair : row) {
                std::string key = CSVUtils::trimQuotes(pair.first);
                if (key == "员工编号" || key.find("员工编号") != std::string::npos) {
                    emp_id_key = pair.first;
                    break;
                }
            }
        }
        if (row.count(emp_name_key) == 0) {
            for (const auto& pair : row) {
                std::string key = CSVUtils::trimQuotes(pair.first);
                if (key == "员工姓名" || key.find("员工姓名") != std::string::npos) {
                    emp_name_key = pair.first;
                    break;
                }
            }
        }
        
        std::string emp_id = CSVUtils::trimQuotes(row.count(emp_id_key) ? row.at(emp_id_key) : "");
        std::string emp_name = CSVUtils::trimQuotes(row.count(emp_name_key) ? row.at(emp_name_key) : "");
        std::string group_name = CSVUtils::trimQuotes(row.count(group_name_key) ? row.at(group_name_key) : "");
        std::string position = CSVUtils::trimQuotes(row.count(position_key) ? row.at(position_key) : "");
        
        if (emp_id.empty()) {
            skipped_rows++;
            continue;
        }
        
        emp.setEmployeeId(emp_id);
        emp.setEmployeeName(emp_name);
        
        // 根据班组名分配装卸组ID
        if (!group_name.empty()) {
            if (group_name_to_id.find(group_name) == group_name_to_id.end()) {
                group_name_to_id[group_name] = next_group_id++;
            }
            emp.setLoadGroup(group_name_to_id[group_name]);
        } else {
            emp.setLoadGroup(0);  // 未分组
        }
        
        // 根据岗位推断资质
        int qualification_mask = inferQualificationFromPosition(position);
        // 装卸员工默认具有外场资质
        qualification_mask |= static_cast<int>(vip_first_class::QualificationMask::EXTERNAL);
        emp.setQualificationMask(qualification_mask);
        
        employees.push_back(emp);
    }
    
    if (employees.empty() && skipped_rows > 0) {
        std::cerr << "WARNING: All " << skipped_rows << " rows were skipped due to empty employee ID" << std::endl;
    }
    
    // std::cout << "从CSV加载了 " << employees.size() << " 个装卸员工" << std::endl;  // 减少输出
    return employees;
}

/**
 * @brief 从CSV文件加载班次数据
 * @param filename CSV文件路径
 * @return 班次列表
 */
inline std::vector<vip_first_class::Shift> loadShiftsFromCSV(const std::string& filename) {
    std::vector<vip_first_class::Shift> shifts;
    
    auto rows = CSVUtils::readCSV(filename, true);
    if (rows.empty()) {
        std::cerr << "警告：CSV文件为空或无法读取: " << filename << std::endl;
        return shifts;
    }
    
    // 读取表头
    std::ifstream header_file(filename);
    std::string header_line;
    if (std::getline(header_file, header_line)) {
        if (header_line.length() >= 3 && 
            static_cast<unsigned char>(header_line[0]) == 0xEF &&
            static_cast<unsigned char>(header_line[1]) == 0xBB &&
            static_cast<unsigned char>(header_line[2]) == 0xBF) {
            header_line = header_line.substr(3);
        }
    }
    header_file.close();
    
    auto header = CSVUtils::parseCSVLine(header_line);
    
    // 清理表头，去除引号和空格，并创建映射
    std::vector<std::string> cleaned_header;
    std::map<std::string, std::string> cleaned_to_header;
    for (const auto& h : header) {
        std::string cleaned = CSVUtils::trimQuotes(h);
        cleaned_header.push_back(cleaned);
        cleaned_to_header[cleaned] = h;
    }
    
    auto data_map = CSVUtils::csvToMap(header, rows);
    
    // 根据班组名分组，从上到下的小组分别认为是1-8组
    std::map<std::string, int> group_name_to_id;  // 班组名 -> 组ID (1-8)
    std::map<int, vip_first_class::Shift> group_shifts;  // 组ID -> Shift对象
    int next_group_id = 1;
    
    for (const auto& row : data_map) {
        // 直接从row中查找列名（因为row中的键是原始表头）
        std::string emp_id_key, group_name_key, shift_name_key;
            for (const auto& pair : row) {
                std::string key = CSVUtils::trimQuotes(pair.first);
            if (emp_id_key.empty() && (key == "员工编号" || key.find("员工编号") != std::string::npos)) {
                    emp_id_key = pair.first;
            }
            if (group_name_key.empty() && (key == "班组名" || key.find("班组名") != std::string::npos)) {
                    group_name_key = pair.first;
                }
            if (shift_name_key.empty() && (key == "班次名称" || key.find("班次名称") != std::string::npos)) {
                shift_name_key = pair.first;
            }
        }
        
        std::string emp_id = CSVUtils::trimQuotes(row.count(emp_id_key) ? row.at(emp_id_key) : "");
        std::string group_name = CSVUtils::trimQuotes(row.count(group_name_key) ? row.at(group_name_key) : "");
        std::string shift_name = CSVUtils::trimQuotes(row.count(shift_name_key) ? row.at(shift_name_key) : "");
        
        if (emp_id.empty() || group_name.empty()) {
            continue;
        }
        
        // 如果这个班组名还没有分配组ID，分配一个新的（最多8组）
        if (group_name_to_id.find(group_name) == group_name_to_id.end()) {
            if (next_group_id <= 8) {
                group_name_to_id[group_name] = next_group_id;
                
                // 创建新的Shift对象
                vip_first_class::Shift shift;
                // 根据班次名称推断班次类型
                int shift_type = 0;  // 0=休息, 1=主班, 2=副班
                if (shift_name.find("主") != std::string::npos || shift_name.find("MAIN") != std::string::npos) {
                    shift_type = 1;
                } else if (shift_name.find("副") != std::string::npos || shift_name.find("SUB") != std::string::npos) {
                    shift_type = 2;
                }
                shift.setShiftType(shift_type);
                group_shifts[next_group_id] = shift;
                
                next_group_id++;
            } else {
                // 超过8组，跳过
                continue;
            }
        }
        
        int group_id = group_name_to_id[group_name];
        if (group_shifts.find(group_id) != group_shifts.end()) {
            // 根据位置或其他规则确定position（使用递增的位置）
            int position = group_shifts[group_id].getPositionToEmployeeId().size() + 1;
            group_shifts[group_id].setEmployeeIdAtPosition(position, emp_id);
        }
    }
    
    // 转换为vector，按组ID排序
    for (int i = 1; i <= 8; ++i) {
        if (group_shifts.find(i) != group_shifts.end()) {
            shifts.push_back(group_shifts[i]);
        }
    }
    
    // std::cout << "从CSV加载了 " << shifts.size() << " 个班次" << std::endl;  // 减少输出
    return shifts;
}

/**
 * @brief 从shift.csv中读取员工信息和班组信息
 * @param filename shift.csv文件路径
 * @param employees 输出参数，员工列表
 * @param group_name_to_employees 输出参数，班组名到员工ID列表的映射
 * @return 是否成功加载
 */
inline bool loadEmployeesFromShiftCSV(const std::string& filename,
                                       std::vector<zhuangxie_class::LoadEmployeeInfo>& employees,
                                       std::map<std::string, std::vector<std::string>>& group_name_to_employees) {
    employees.clear();
    group_name_to_employees.clear();
    
    auto rows = CSVUtils::readCSV(filename, true);
    if (rows.empty()) {
        std::cerr << "警告：CSV文件为空或无法读取: " << filename << std::endl;
        return false;
    }
    
    // 读取表头
    std::ifstream header_file(filename);
    std::string header_line;
    if (std::getline(header_file, header_line)) {
        if (header_line.length() >= 3 && 
            static_cast<unsigned char>(header_line[0]) == 0xEF &&
            static_cast<unsigned char>(header_line[1]) == 0xBB &&
            static_cast<unsigned char>(header_line[2]) == 0xBF) {
            header_line = header_line.substr(3);
        }
    }
    header_file.close();
    
    auto header = CSVUtils::parseCSVLine(header_line);
    auto data_map = CSVUtils::csvToMap(header, rows);
    
    // 查找列名
    std::string emp_id_key, emp_name_key, group_name_key;
    for (const auto& row : data_map) {
            for (const auto& pair : row) {
                std::string key = CSVUtils::trimQuotes(pair.first);
            if (emp_id_key.empty() && (key == "员工编号" || key.find("员工编号") != std::string::npos)) {
                emp_id_key = pair.first;
            }
            if (emp_name_key.empty() && (key == "人员姓名" || key.find("人员姓名") != std::string::npos)) {
                emp_name_key = pair.first;
            }
            if (group_name_key.empty() && (key == "班组名" || key.find("班组名") != std::string::npos)) {
                group_name_key = pair.first;
            }
        }
        if (!emp_id_key.empty() && !emp_name_key.empty() && !group_name_key.empty()) {
                    break;
                }
            }
    
    if (emp_id_key.empty() || group_name_key.empty()) {
        std::cerr << "ERROR: shift.csv missing required columns" << std::endl;
        return false;
    }
    
    // 读取员工信息
    std::map<std::string, zhuangxie_class::LoadEmployeeInfo> employee_map;  // 员工ID -> LoadEmployeeInfo
    
    for (const auto& row : data_map) {
        std::string emp_id = CSVUtils::trimQuotes(row.count(emp_id_key) ? row.at(emp_id_key) : "");
        std::string emp_name = CSVUtils::trimQuotes(row.count(emp_name_key) ? row.at(emp_name_key) : "");
        std::string group_name = CSVUtils::trimQuotes(row.count(group_name_key) ? row.at(group_name_key) : "");
        
        if (emp_id.empty() || group_name.empty()) {
            continue;
        }
        
        // 如果员工已存在，跳过（避免重复）
        if (employee_map.find(emp_id) != employee_map.end()) {
            continue;
        }
        
        zhuangxie_class::LoadEmployeeInfo emp;
        emp.setEmployeeId(emp_id);
        emp.setEmployeeName(emp_name);
        // 默认给予外场资质（装卸员工）
        emp.setQualificationMask(static_cast<int>(vip_first_class::QualificationMask::EXTERNAL));
        
        employee_map[emp_id] = emp;
        group_name_to_employees[group_name].push_back(emp_id);
    }
    
    // 转换为vector
    for (const auto& pair : employee_map) {
        employees.push_back(pair.second);
    }
    
    return true;
}

/**
 * @brief 辅助函数：将DateTime转换为从2020-01-01 00:00:00开始的秒数
 * @param dt DateTime对象
 * @return 从2020-01-01 00:00:00开始的秒数
 */
inline long dateTimeToSeconds(const DateTime& dt) {
    // 2020-01-01 00:00:00 的时间点
    std::tm tm_2020 = {};
    tm_2020.tm_year = 120;  // 2020 - 1900
    tm_2020.tm_mon = 0;      // 1月 (0-based)
    tm_2020.tm_mday = 1;
    tm_2020.tm_hour = 0;
    tm_2020.tm_min = 0;
    tm_2020.tm_sec = 0;
    auto time_2020 = std::mktime(&tm_2020);
    auto epoch_2020 = std::chrono::system_clock::from_time_t(time_2020);
    
    // 计算时间差
    auto duration = dt - epoch_2020;
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

/**
 * @brief 从stand_pos.csv加载机位信息
 * @param filename CSV文件路径
 * @return 机位到是否远机位的映射（机位号字符串 -> 是否远机位）
 */
inline std::map<std::string, bool> loadStandPositionsFromCSV(const std::string& filename) {
    std::map<std::string, bool> stand_map;
    
    auto rows = CSVUtils::readCSV(filename, true);
    if (rows.empty()) {
        std::cerr << "WARNING: Stand position CSV file is empty or cannot be read: " << filename << std::endl;
        return stand_map;
    }
    
    // 读取表头
    std::ifstream header_file(filename);
    std::string header_line;
    if (std::getline(header_file, header_line)) {
        if (header_line.length() >= 3 && 
            static_cast<unsigned char>(header_line[0]) == 0xEF &&
            static_cast<unsigned char>(header_line[1]) == 0xBB &&
            static_cast<unsigned char>(header_line[2]) == 0xBF) {
            header_line = header_line.substr(3);
        }
    }
    header_file.close();
    
    auto header = CSVUtils::parseCSVLine(header_line);
    auto data_map = CSVUtils::csvToMap(header, rows);
    
    for (const auto& row : data_map) {
        std::string stand = CSVUtils::trimQuotes(row.count("机位") ? row.at("机位") : "");
        std::string is_remote_str = CSVUtils::trimQuotes(row.count("是否为远机位") ? row.at("是否为远机位") : "");
        
        if (!stand.empty()) {
            bool is_remote = (is_remote_str == "Y" || is_remote_str == "y");
            stand_map[stand] = is_remote;
        }
    }
    
    return stand_map;
}

/**
 * @brief 从task.csv文件加载LoadTask对象（集成Flight和TaskDefinition的字段）
 * @param filename CSV文件路径
 * @param tasks 输出参数，LoadTask对象列表
 * @param stand_pos_file 可选，stand_pos.csv文件路径，用于判断远机位
 * @return 成功返回true，失败返回false
 */
inline bool loadLoadTasksFromCSV(
    const std::string& filename,
    std::vector<zhuangxie_class::LoadTask>& tasks,
    const std::string& stand_pos_file = "") {
    
    tasks.clear();
    
    // 加载机位信息（如果提供了stand_pos_file）
    std::map<std::string, bool> stand_positions;
    if (!stand_pos_file.empty()) {
        stand_positions = loadStandPositionsFromCSV(stand_pos_file);
    }
    
    auto rows = CSVUtils::readCSV(filename, true);
    if (rows.empty()) {
        std::cerr << "WARNING: CSV file is empty or cannot be read: " << filename << std::endl;
        return false;
    }
    
    // 读取表头
    std::ifstream header_file(filename);
    std::string header_line;
    if (std::getline(header_file, header_line)) {
        if (header_line.length() >= 3 && 
            static_cast<unsigned char>(header_line[0]) == 0xEF &&
            static_cast<unsigned char>(header_line[1]) == 0xBB &&
            static_cast<unsigned char>(header_line[2]) == 0xBF) {
            header_line = header_line.substr(3);
        }
    }
    header_file.close();
    
    auto header = CSVUtils::parseCSVLine(header_line);
    auto data_map = CSVUtils::csvToMap(header, rows);
    
    for (const auto& row : data_map) {
        // 辅助函数：在row中查找列名（处理可能的引号或空格）
        auto findColumn = [&row](const std::vector<std::string>& possible_names) -> std::string {
            for (const auto& name : possible_names) {
                // 直接匹配
                if (row.count(name)) {
                    return name;
                }
                // 尝试trimQuotes后匹配
                for (const auto& key : row) {
                    if (CSVUtils::trimQuotes(key.first) == name || key.first.find(name) != std::string::npos) {
                        return key.first;
                    }
                }
            }
            return "";  // 找不到返回空字符串
        };
        
        // 查找列名
        std::string task_id_key = findColumn({"任务ID"});
        std::string task_name_key = findColumn({"任务名称"});
        std::string task_start_time_key = findColumn({"任务开始时间"});
        std::string task_duration_key = findColumn({"任务时长"});
        std::string arrival_estimated_time_key = findColumn({"到达航班预达时间"});
        std::string departure_estimated_time_key = findColumn({"出发航班预离时间"});
        std::string flight_type_key = findColumn({"航班类型"});
        std::string stand_key = findColumn({"机位"});
        std::string min_staff_key = findColumn({"任务对应的航班所需最少人数"});
        std::string cargo_weight_key = findColumn({"任务装卸货量"});
        std::string max_overlap_time_key = findColumn({"任务最大重叠时间"});
        
        // 提取字段
        std::string task_id_str = task_id_key.empty() ? "" : CSVUtils::trimQuotes(row.count(task_id_key) ? row.at(task_id_key) : "");
        std::string task_name = task_name_key.empty() ? "" : CSVUtils::trimQuotes(row.count(task_name_key) ? row.at(task_name_key) : "");
        std::string task_start_time_str = task_start_time_key.empty() ? "" : CSVUtils::trimQuotes(row.count(task_start_time_key) ? row.at(task_start_time_key) : "");
        std::string task_duration_str = task_duration_key.empty() ? "" : CSVUtils::trimQuotes(row.count(task_duration_key) ? row.at(task_duration_key) : "");
        std::string arrival_estimated_time_str = arrival_estimated_time_key.empty() ? "" : CSVUtils::trimQuotes(row.count(arrival_estimated_time_key) ? row.at(arrival_estimated_time_key) : "");
        std::string departure_estimated_time_str = departure_estimated_time_key.empty() ? "" : CSVUtils::trimQuotes(row.count(departure_estimated_time_key) ? row.at(departure_estimated_time_key) : "");
        std::string flight_type_str = flight_type_key.empty() ? "" : CSVUtils::trimQuotes(row.count(flight_type_key) ? row.at(flight_type_key) : "");
        std::string stand_str = stand_key.empty() ? "" : CSVUtils::trimQuotes(row.count(stand_key) ? row.at(stand_key) : "");
        std::string min_staff_str = min_staff_key.empty() ? "" : CSVUtils::trimQuotes(row.count(min_staff_key) ? row.at(min_staff_key) : "");
        std::string cargo_weight_str = cargo_weight_key.empty() ? "" : CSVUtils::trimQuotes(row.count(cargo_weight_key) ? row.at(cargo_weight_key) : "");
        std::string max_overlap_time_str = max_overlap_time_key.empty() ? "" : CSVUtils::trimQuotes(row.count(max_overlap_time_key) ? row.at(max_overlap_time_key) : "");
        
        // 跳过空行
        if (task_id_str.empty() && task_name.empty()) {
            continue;
        }
        
        // 创建LoadTask对象
        zhuangxie_class::LoadTask task;
        
        // 设置任务ID（直接使用CSV中的字符串，如果为空则使用行号）
        static int task_id_counter = 1;  // 静态计数器，确保唯一性
        if (!task_id_str.empty()) {
            task.setTaskId(task_id_str);
        } else {
            // 如果任务ID为空，使用计数器生成唯一ID
            task.setTaskId("task_" + std::to_string(task_id_counter++));
        }
        
        // 设置任务名称
        task.setTaskName(task_name);
        
        // 解析时间（转换为从2020-01-01开始的秒数）
        long task_start_time = 0;
        long task_end_time = 0;
        long arrival_time = 0;
        long departure_time = 0;
        
        try {
            if (!task_start_time_str.empty()) {
                task_start_time = CSVUtils::parseDateTimeString(task_start_time_str);
            }
            
            // 计算任务结束时间 = 开始时间 + 时长（秒）
            if (!task_duration_str.empty()) {
                int duration_minutes = std::stoi(task_duration_str);
                task_end_time = task_start_time + duration_minutes * 60;
        } else {
                task_end_time = task_start_time;
            }
            
            if (!arrival_estimated_time_str.empty()) {
                arrival_time = CSVUtils::parseDateTimeString(arrival_estimated_time_str);
            }
            if (!departure_estimated_time_str.empty()) {
                departure_time = CSVUtils::parseDateTimeString(departure_estimated_time_str);
            }
        } catch (...) {
            continue;
        }
        
        // 设置任务时间（使用CSV的任务时长作为保障时间）
        task.setEarliestStartTime(task_start_time);
        task.setLatestEndTime(task_end_time);
        
        // 设置任务时长
        if (!task_duration_str.empty()) {
            int duration_minutes = std::stoi(task_duration_str);
            task.setDuration(duration_minutes * 60);
        }
        
        // 设置航班时间
        task.setArrivalTime(arrival_time);
        task.setDepartureTime(departure_time);
        
        // 判断航班类型（全部设为国内）
        int flight_type_enum = static_cast<int>(zhuangxie_class::FlightType::DOMESTIC_ARRIVAL);
        if (task_name.find("进港") != std::string::npos) {
            flight_type_enum = static_cast<int>(zhuangxie_class::FlightType::DOMESTIC_ARRIVAL);
        } else if (task_name.find("出港") != std::string::npos) {
            flight_type_enum = static_cast<int>(zhuangxie_class::FlightType::DOMESTIC_DEPARTURE);
        } else if (flight_type_str.find("过站") != std::string::npos) {
            flight_type_enum = static_cast<int>(zhuangxie_class::FlightType::DOMESTIC_TRANSIT);
        }
        task.setFlightType(flight_type_enum);
        
        // 设置机位
        int stand_num = 0;
        if (!stand_str.empty()) {
            try {
                stand_num = std::stoi(stand_str);
            } catch (...) {
                // 如果解析失败，尝试从stand_positions中查找
                // 暂时只设置远机位信息，stand_num保持为0
            }
        }
        task.setStand(stand_num);
        
        // 判断是否远机位
        bool is_remote = false;
        if (!stand_str.empty()) {
            auto it = stand_positions.find(stand_str);
            if (it != stand_positions.end()) {
                is_remote = it->second;
            }
        }
        task.setRemoteStand(is_remote);
        
        // 设置通勤时间，默认为8分钟 = 480秒
        task.setTravelTime(8 * 60);
        
        // 设置需要的人数：优先读取人数，无人数根据吨位判断
        int required_count = 0;
        if (!min_staff_str.empty()) {
            try {
                required_count = std::stoi(min_staff_str);
            } catch (...) {
                required_count = 0;
            }
        }
        
        // 如果没有人数字段，从吨位推算人数
        if (required_count == 0 && !cargo_weight_str.empty()) {
            try {
                double cargo_weight = std::stod(cargo_weight_str);
                // 根据吨位判断所需人数：2.5吨以上需要6人，否则3人
                if (cargo_weight >= 2.5) {
                    required_count = 6;
        } else {
                    required_count = 3;
                }
            } catch (...) {
                required_count = 3;  // 默认3人
            }
        }
        
        // 如果还是没有人数，使用默认值3人
        if (required_count == 0) {
            required_count = 3;
        }
        task.setRequiredCount(required_count);
        
        // LoadTask 类不再有 setMaxOverlapTime 和 setAllowOverlap 方法
        // 任务允许重叠都是Y，最大重叠时间就是任务本身的时间（已在setDuration中处理）
        
        // 设置其他任务属性
        task.setPreferMainShift(true);  // 默认优先主班
        task.setCanNewEmployee(false);  // 默认不允许新员工
        task.setAssigned(false);
        task.setShortStaffed(false);
        
        // 设置资质要求（简化处理，使用默认值）
        task.setRequiredQualification(0);  // 可以根据需要设置
        
        tasks.push_back(task);
    }
    
    return true;
}

/**
 * @brief 从referschedule.csv文件加载LoadTask对象
 * @param filename CSV文件路径
 * @param tasks 输出参数，LoadTask对象列表
 * @param stand_pos_file 可选，stand_pos.csv文件路径，用于判断远机位
 * @return 成功返回true，失败返回false
 * 
 * 注意：此函数用于处理referschedule.csv格式，与task.csv格式不同
 * - 缺少"任务对应的航班所需最少人数"字段，所有任务统一设置为3人
 * - 缺少"任务装卸货量"字段
 * - 缺少"任务最大重叠时间"字段，默认为0（不允许重叠）
 * - 忽略"员工编号"和"车牌号"字段
 */
inline bool loadLoadTasksFromReferscheduleCSV(
    const std::string& filename,
    std::vector<zhuangxie_class::LoadTask>& tasks,
    const std::string& stand_pos_file = "") {
    
    tasks.clear();
    
    // 加载机位信息（如果提供了stand_pos_file）
    std::map<std::string, bool> stand_positions;
    if (!stand_pos_file.empty()) {
        stand_positions = loadStandPositionsFromCSV(stand_pos_file);
    }
    
    auto rows = CSVUtils::readCSV(filename, true);
    if (rows.empty()) {
        std::cerr << "WARNING: CSV file is empty or cannot be read: " << filename << std::endl;
        return false;
    }
    
    // 读取表头
    std::ifstream header_file(filename);
    std::string header_line;
    if (std::getline(header_file, header_line)) {
        if (header_line.length() >= 3 && 
            static_cast<unsigned char>(header_line[0]) == 0xEF &&
            static_cast<unsigned char>(header_line[1]) == 0xBB &&
            static_cast<unsigned char>(header_line[2]) == 0xBF) {
            header_line = header_line.substr(3);
        }
    }
    header_file.close();
    
    auto header = CSVUtils::parseCSVLine(header_line);
    auto data_map = CSVUtils::csvToMap(header, rows);
    
    for (const auto& row : data_map) {
        // 辅助函数：在row中查找列名（处理可能的引号或空格）
        auto findColumn = [&row](const std::vector<std::string>& possible_names) -> std::string {
            for (const auto& name : possible_names) {
                // 直接匹配
                if (row.count(name)) {
                    return name;
                }
                // 尝试trimQuotes后匹配
                for (const auto& key : row) {
                    if (CSVUtils::trimQuotes(key.first) == name || key.first.find(name) != std::string::npos) {
                        return key.first;
                    }
                }
            }
            return "";  // 找不到返回空字符串
        };
        
        // 查找列名
        std::string task_id_key = findColumn({"任务ID"});
        std::string task_name_key = findColumn({"任务名称"});
        std::string task_date_key = findColumn({"任务日期"});
        std::string task_start_time_key = findColumn({"任务开始时间"});
        std::string task_duration_key = findColumn({"任务时长"});
        std::string arrival_estimated_time_key = findColumn({"到达航班预达时间"});
        std::string departure_estimated_time_key = findColumn({"出发航班预离时间"});
        std::string arrival_flight_id_key = findColumn({"到达航班ID"});
        std::string departure_flight_id_key = findColumn({"出发航班ID"});
        std::string arrival_flight_number_key = findColumn({"到达航班号"});
        std::string departure_flight_number_key = findColumn({"出发航班号"});
        std::string flight_type_key = findColumn({"航班类型"});
        std::string stand_key = findColumn({"机位"});
        std::string terminal_key = findColumn({"航站楼"});
        std::string in_out_key = findColumn({"进/出港"});
        
        // 提取字段
        std::string task_id_str = task_id_key.empty() ? "" : CSVUtils::trimQuotes(row.count(task_id_key) ? row.at(task_id_key) : "");
        std::string task_name = task_name_key.empty() ? "" : CSVUtils::trimQuotes(row.count(task_name_key) ? row.at(task_name_key) : "");
        std::string task_date_str = task_date_key.empty() ? "" : CSVUtils::trimQuotes(row.count(task_date_key) ? row.at(task_date_key) : "");
        std::string task_start_time_str = task_start_time_key.empty() ? "" : CSVUtils::trimQuotes(row.count(task_start_time_key) ? row.at(task_start_time_key) : "");
        std::string task_duration_str = task_duration_key.empty() ? "" : CSVUtils::trimQuotes(row.count(task_duration_key) ? row.at(task_duration_key) : "");
        std::string arrival_estimated_time_str = arrival_estimated_time_key.empty() ? "" : CSVUtils::trimQuotes(row.count(arrival_estimated_time_key) ? row.at(arrival_estimated_time_key) : "");
        std::string departure_estimated_time_str = departure_estimated_time_key.empty() ? "" : CSVUtils::trimQuotes(row.count(departure_estimated_time_key) ? row.at(departure_estimated_time_key) : "");
        std::string arrival_flight_id_str = arrival_flight_id_key.empty() ? "" : CSVUtils::trimQuotes(row.count(arrival_flight_id_key) ? row.at(arrival_flight_id_key) : "");
        std::string departure_flight_id_str = departure_flight_id_key.empty() ? "" : CSVUtils::trimQuotes(row.count(departure_flight_id_key) ? row.at(departure_flight_id_key) : "");
        std::string arrival_flight_number_str = arrival_flight_number_key.empty() ? "" : CSVUtils::trimQuotes(row.count(arrival_flight_number_key) ? row.at(arrival_flight_number_key) : "");
        std::string departure_flight_number_str = departure_flight_number_key.empty() ? "" : CSVUtils::trimQuotes(row.count(departure_flight_number_key) ? row.at(departure_flight_number_key) : "");
        std::string flight_type_str = flight_type_key.empty() ? "" : CSVUtils::trimQuotes(row.count(flight_type_key) ? row.at(flight_type_key) : "");
        std::string stand_str = stand_key.empty() ? "" : CSVUtils::trimQuotes(row.count(stand_key) ? row.at(stand_key) : "");
        std::string terminal_str = terminal_key.empty() ? "" : CSVUtils::trimQuotes(row.count(terminal_key) ? row.at(terminal_key) : "");
        std::string in_out_str = in_out_key.empty() ? "" : CSVUtils::trimQuotes(row.count(in_out_key) ? row.at(in_out_key) : "");
        
        // 跳过空行
        if (task_id_str.empty() && task_name.empty()) {
            continue;
        }
        
        // 创建LoadTask对象
        zhuangxie_class::LoadTask task;
        
        // 设置任务ID（直接使用CSV中的字符串）
        if (!task_id_str.empty()) {
            task.setTaskId(task_id_str);
        } else {
            // 如果任务ID为空，跳过该行
            continue;
        }
        
        // 设置任务名称
        task.setTaskName(task_name);
        
        // 设置任务日期
        task.setTaskDate(task_date_str);
        
        // 设置航班信息
        task.setArrivalFlightId(arrival_flight_id_str);
        task.setDepartureFlightId(departure_flight_id_str);
        task.setArrivalFlightNumber(arrival_flight_number_str);
        task.setDepartureFlightNumber(departure_flight_number_str);
        task.setTerminal(terminal_str);
        
        // 解析时间（转换为从2020-01-01开始的秒数）
        long earliest_start_time = 0;
        long latest_end_time = 0;
        long arrival_time = 0;
        long departure_time = 0;
        long duration_seconds = 0;
        
        try {
            // 最早开始时间 = 任务开始时间（从CSV读取）
            if (!task_start_time_str.empty()) {
                earliest_start_time = CSVUtils::parseDateTimeString(task_start_time_str);
            }
            
            // 任务时长（转换为秒）
            if (!task_duration_str.empty()) {
                int duration_minutes = std::stoi(task_duration_str);
                duration_seconds = duration_minutes * 60;
            }
            
            // 最晚结束时间 = 出发航班预离时间 - 5分钟
            if (!departure_estimated_time_str.empty()) {
                departure_time = CSVUtils::parseDateTimeString(departure_estimated_time_str);
                latest_end_time = departure_time - 5 * 60;  // 减去5分钟（300秒）
            } else {
                // 如果没有出发航班预离时间，使用最早开始时间 + 时长作为最晚结束时间
                latest_end_time = earliest_start_time + duration_seconds;
            }
            
            if (!arrival_estimated_time_str.empty()) {
                arrival_time = CSVUtils::parseDateTimeString(arrival_estimated_time_str);
            }
        } catch (...) {
            continue;
        }
        
        // 设置任务时间
        task.setEarliestStartTime(earliest_start_time);
        task.setLatestEndTime(latest_end_time);
        task.setDuration(duration_seconds);
        task.setActualStartTime(0);  // 初始化为0，表示未分配
        
        // 设置航班时间
        task.setArrivalTime(arrival_time);
        task.setDepartureTime(departure_time);
        
        // 判断航班类型（根据任务名称和"进/出港"字段）
        int flight_type_enum = static_cast<int>(zhuangxie_class::FlightType::DOMESTIC_ARRIVAL);
        if (in_out_str == "进" || task_name.find("进港") != std::string::npos) {
            flight_type_enum = static_cast<int>(zhuangxie_class::FlightType::DOMESTIC_ARRIVAL);
        } else if (in_out_str == "出" || task_name.find("出港") != std::string::npos) {
            flight_type_enum = static_cast<int>(zhuangxie_class::FlightType::DOMESTIC_DEPARTURE);
        } else if (flight_type_str.find("过站") != std::string::npos) {
            flight_type_enum = static_cast<int>(zhuangxie_class::FlightType::DOMESTIC_TRANSIT);
        }
        task.setFlightType(flight_type_enum);
        
        // 设置机位
        int stand_num = 0;
        if (!stand_str.empty()) {
            try {
                stand_num = std::stoi(stand_str);
            } catch (...) {
                // 如果解析失败，stand_num保持为0
            }
        }
        task.setStand(stand_num);
        
        // 判断是否远机位
        bool is_remote = false;
        if (!stand_str.empty()) {
            auto it = stand_positions.find(stand_str);
            if (it != stand_positions.end()) {
                is_remote = it->second;
            }
        }
        task.setRemoteStand(is_remote);
        
        // 设置通勤时间，默认为8分钟 = 480秒
        task.setTravelTime(8 * 60);
        
        // 设置需要的人数：所有任务都设置为3人（referschedule.csv中没有人数字段）
        // 注意：忽略"员工编号"和"车牌号"字段
        task.setRequiredCount(3);
        
        // 设置其他任务属性
        task.setPreferMainShift(true);  // 默认优先主班
        task.setCanNewEmployee(false);  // 默认不允许新员工
        task.setAssigned(false);
        task.setShortStaffed(false);
        
        // 设置资质要求（简化处理，使用默认值）
        task.setRequiredQualification(0);  // 可以根据需要设置
        
        tasks.push_back(task);
    }
    
    return true;
}

/**
 * @brief 从task.csv加载贵宾任务
 * @param filename task.csv文件路径
 * @param tasks 输出的任务列表
 * @return 是否成功加载
 */
inline bool loadVIPTasksFromCSV(
    const std::string& filename,
    std::vector<vip_first_class::TaskDefinition>& tasks) {
    
    tasks.clear();
    
    auto rows = CSVUtils::readCSV(filename, true);
    if (rows.empty()) {
        std::cerr << "WARNING: CSV file is empty or cannot be read: " << filename << std::endl;
        return false;
    }
    
    // 读取表头
    std::ifstream header_file(filename);
    std::string header_line;
    if (std::getline(header_file, header_line)) {
        if (header_line.length() >= 3 && 
            static_cast<unsigned char>(header_line[0]) == 0xEF &&
            static_cast<unsigned char>(header_line[1]) == 0xBB &&
            static_cast<unsigned char>(header_line[2]) == 0xBF) {
            header_line = header_line.substr(3);
        }
    }
    header_file.close();
    
    auto header = CSVUtils::parseCSVLine(header_line);
    auto data_map = CSVUtils::csvToMap(header, rows);
    
    for (const auto& row : data_map) {
        // 辅助函数：在row中查找列名（处理可能的引号或空格）
        auto findColumn = [&row](const std::vector<std::string>& possible_names) -> std::string {
            for (const auto& name : possible_names) {
                // 直接匹配
                if (row.count(name)) {
                    return name;
                }
                // 尝试trimQuotes后匹配
                for (const auto& key : row) {
                    if (CSVUtils::trimQuotes(key.first) == name || key.first.find(name) != std::string::npos) {
                        return key.first;
                    }
                }
            }
            return "";  // 找不到返回空字符串
        };
        
        // 查找列名
        std::string task_id_key = findColumn({"任务ID"});
        std::string task_name_key = findColumn({"任务名称"});
        std::string task_date_key = findColumn({"任务日期"});
        std::string task_start_time_key = findColumn({"任务开始时间"});
        std::string task_duration_key = findColumn({"任务时长"});
        std::string arrival_flight_id_key = findColumn({"到达航班ID"});
        std::string departure_flight_id_key = findColumn({"出发航班ID"});
        std::string arrival_flight_number_key = findColumn({"到达航班号"});
        std::string departure_flight_number_key = findColumn({"出发航班号"});
        std::string terminal_key = findColumn({"航站楼"});
        std::string stand_key = findColumn({"机位"});
        std::string required_count_key = findColumn({"任务对应的航班所需最少人数"});
        
        // 提取字段
        std::string task_id_str = task_id_key.empty() ? "" : CSVUtils::trimQuotes(row.count(task_id_key) ? row.at(task_id_key) : "");
        std::string task_name = task_name_key.empty() ? "" : CSVUtils::trimQuotes(row.count(task_name_key) ? row.at(task_name_key) : "");
        std::string task_date_str = task_date_key.empty() ? "" : CSVUtils::trimQuotes(row.count(task_date_key) ? row.at(task_date_key) : "");
        std::string task_start_time_str = task_start_time_key.empty() ? "" : CSVUtils::trimQuotes(row.count(task_start_time_key) ? row.at(task_start_time_key) : "");
        std::string task_duration_str = task_duration_key.empty() ? "" : CSVUtils::trimQuotes(row.count(task_duration_key) ? row.at(task_duration_key) : "");
        std::string arrival_flight_id_str = arrival_flight_id_key.empty() ? "" : CSVUtils::trimQuotes(row.count(arrival_flight_id_key) ? row.at(arrival_flight_id_key) : "");
        std::string departure_flight_id_str = departure_flight_id_key.empty() ? "" : CSVUtils::trimQuotes(row.count(departure_flight_id_key) ? row.at(departure_flight_id_key) : "");
        std::string arrival_flight_number_str = arrival_flight_number_key.empty() ? "" : CSVUtils::trimQuotes(row.count(arrival_flight_number_key) ? row.at(arrival_flight_number_key) : "");
        std::string departure_flight_number_str = departure_flight_number_key.empty() ? "" : CSVUtils::trimQuotes(row.count(departure_flight_number_key) ? row.at(departure_flight_number_key) : "");
        std::string terminal_str = terminal_key.empty() ? "" : CSVUtils::trimQuotes(row.count(terminal_key) ? row.at(terminal_key) : "");
        std::string stand_str = stand_key.empty() ? "" : CSVUtils::trimQuotes(row.count(stand_key) ? row.at(stand_key) : "");
        std::string required_count_str = required_count_key.empty() ? "" : CSVUtils::trimQuotes(row.count(required_count_key) ? row.at(required_count_key) : "");
        
        // 跳过空行
        if (task_id_str.empty() && task_name.empty()) {
            continue;
        }
        
        // 创建TaskDefinition对象
        vip_first_class::TaskDefinition task;
        
        // 设置任务ID（直接使用CSV中的字符串）
        if (!task_id_str.empty()) {
            task.setTaskId(task_id_str);
        } else {
            // 如果任务ID为空，跳过该行
            continue;
        }
        
        // 设置任务名称
        task.setTaskName(task_name);
        
        // 设置任务日期
        task.setTaskDate(task_date_str);
        
        // 设置航班信息
        task.setArrivalFlightId(arrival_flight_id_str);
        task.setDepartureFlightId(departure_flight_id_str);
        task.setArrivalFlightNumber(arrival_flight_number_str);
        task.setDepartureFlightNumber(departure_flight_number_str);
        task.setTerminal(terminal_str);
        
        // 解析时间（转换为从2020-01-01开始的秒数）
        long task_start_time = 0;
        long duration_seconds = 0;
        
        try {
            // 任务开始时间
            if (!task_start_time_str.empty()) {
                task_start_time = CSVUtils::parseDateTimeString(task_start_time_str);
            }
            
            // 任务时长（转换为秒）
            if (!task_duration_str.empty() && task_duration_str != "0") {
                int duration_minutes = std::stoi(task_duration_str);
                duration_seconds = duration_minutes * 60;
                } else {
                // 如果时长为空或0，使用默认值（1小时）
                duration_seconds = 60 * 60;
                }
            } catch (...) {
            continue;
        }
        
        // 设置任务时间
        task.setStartTime(task_start_time);  // 保留用于兼容
        task.setDuration(duration_seconds);
        task.setActualStartTime(0);  // 初始化为0，表示未分配
        
        // 设置机位
        int stand_num = 0;
        if (!stand_str.empty()) {
            try {
                stand_num = std::stoi(stand_str);
            } catch (...) {
                // 如果解析失败，stand_num保持为0
            }
        }
        task.setStand(stand_num);
        
        // 设置需要的人数
        int required_count = 1;  // 默认1人
        if (!required_count_str.empty()) {
            try {
                required_count = std::stoi(required_count_str);
            } catch (...) {
                // 如果解析失败，使用默认值
            }
        }
        task.setRequiredCount(required_count);
        
        // 设置任务属性
        task.setPreferMainShift(true);  // 默认优先主班
        task.setCanNewEmployee(false);  // 默认不允许新员工
        task.setAssigned(false);
        task.setShortStaffed(false);
        
        // 任务是否允许重叠都是Y，如果是Y就认为最大重叠时间就是任务本身的时间
        task.setAllowOverlap(true);  // 设置为Y（true）
        // setAllowOverlap会自动设置max_overlap_time_ = duration_（如果duration_ > 0）
        // 但这里duration_已经设置了，所以需要再次调用setDuration来触发更新
        if (duration_seconds > 0) {
            task.setDuration(duration_seconds);  // 这会触发max_overlap_time_的设置
        }
        
        // 设置资质要求（简化处理，使用默认值）
        task.setRequiredQualification(0);  // 可以根据需要设置
        
        // 设置任务类型（根据任务名称推断，这里简化处理）
        // 可以根据实际需求添加任务类型推断逻辑
        task.setTaskType(vip_first_class::TaskType::DISPATCH);  // 默认类型
        
        tasks.push_back(task);
    }
    
    return true;
}

} // namespace CSVLoader
} // namespace AirportStaffScheduler

