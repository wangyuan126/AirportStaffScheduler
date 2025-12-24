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
#include "zhuangxie_class/flight.h"
#include "CommonAdapterUtils.h"
#include <iostream>
#include <sstream>
#include <algorithm>

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
    
    // 清理表头，去除引号和空格，并创建原始列名到清理后列名的映射
    std::vector<std::string> cleaned_header;
    std::map<std::string, std::string> header_to_cleaned;  // 原始列名 -> 清理后的列名
    for (const auto& h : header) {
        std::string cleaned = CSVUtils::trimQuotes(h);
        cleaned_header.push_back(cleaned);
        header_to_cleaned[h] = cleaned;
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
            emp_id_key = h;  // 保存原始列名
        }
        if (trimmed == "员工姓名" || trimmed.find("员工姓名") != std::string::npos) {
            has_emp_name = true;
            emp_name_key = h;
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
        std::cerr << "ERROR: CSV file missing required column '员工编号' (employee ID). Available columns: ";
        for (size_t i = 0; i < cleaned_header.size(); ++i) {
            std::cerr << cleaned_header[i];
            if (i < cleaned_header.size() - 1) std::cerr << ", ";
        }
        std::cerr << std::endl;
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
        std::string emp_id_key = cleaned_to_header.count("员工编号") ? cleaned_to_header.at("员工编号") : "员工编号";
        std::string group_name_key = cleaned_to_header.count("班组名") ? cleaned_to_header.at("班组名") : "班组名";
        std::string shift_name_key = cleaned_to_header.count("班次名称") ? cleaned_to_header.at("班次名称") : "班次名称";
        
        // 尝试在row中直接查找（可能列名有变化）
        if (row.count(emp_id_key) == 0) {
            for (const auto& pair : row) {
                std::string key = CSVUtils::trimQuotes(pair.first);
                if (key == "员工编号" || key.find("员工编号") != std::string::npos) {
                    emp_id_key = pair.first;
                    break;
                }
            }
        }
        if (row.count(group_name_key) == 0) {
            for (const auto& pair : row) {
                std::string key = CSVUtils::trimQuotes(pair.first);
                if (key == "班组名" || key.find("班组名") != std::string::npos) {
                    group_name_key = pair.first;
                    break;
                }
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
 * @brief 从任务CSV文件加载航班数据（装卸调度）
 * @param filename CSV文件路径
 * @return 航班列表
 */
inline std::vector<zhuangxie_class::Flight> loadFlightsFromTaskCSV(const std::string& filename) {
    std::vector<zhuangxie_class::Flight> flights;
    
    auto rows = CSVUtils::readCSV(filename, true);
    if (rows.empty()) {
        std::cerr << "WARNING: CSV file is empty or cannot be read: " << filename << std::endl;
        return flights;
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
    
    // 按到达航班ID或出发航班ID分组，合并进港和出港信息
    std::map<std::string, zhuangxie_class::Flight> flight_map;
    
    for (const auto& row : data_map) {
        // 查找需要的列名
        std::string task_name_key, arrival_flight_id_key, departure_flight_id_key, 
                    arrival_time_key, departure_time_key, stand_key, min_staff_key;
        
        // 查找任务名称
        if (cleaned_to_header.count("任务名称")) {
            task_name_key = cleaned_to_header.at("任务名称");
        } else {
            for (const auto& pair : row) {
                std::string key = CSVUtils::trimQuotes(pair.first);
                if (key.find("任务名称") != std::string::npos) {
                    task_name_key = pair.first;
                    break;
                }
            }
        }
        
        // 查找到达航班ID
        if (cleaned_to_header.count("到达航班ID")) {
            arrival_flight_id_key = cleaned_to_header.at("到达航班ID");
        } else {
            for (const auto& pair : row) {
                std::string key = CSVUtils::trimQuotes(pair.first);
                if (key.find("到达航班ID") != std::string::npos) {
                    arrival_flight_id_key = pair.first;
                    break;
                }
            }
        }
        
        // 查找出发航班ID
        if (cleaned_to_header.count("出发航班ID")) {
            departure_flight_id_key = cleaned_to_header.at("出发航班ID");
        } else {
            for (const auto& pair : row) {
                std::string key = CSVUtils::trimQuotes(pair.first);
                if (key.find("出发航班ID") != std::string::npos) {
                    departure_flight_id_key = pair.first;
                    break;
                }
            }
        }
        
        // 查找到达航班预达时间
        if (cleaned_to_header.count("到达航班预达时间")) {
            arrival_time_key = cleaned_to_header.at("到达航班预达时间");
        } else {
            for (const auto& pair : row) {
                std::string key = CSVUtils::trimQuotes(pair.first);
                if (key.find("到达航班预达时间") != std::string::npos) {
                    arrival_time_key = pair.first;
                    break;
                }
            }
        }
        
        // 查找出发航班预离时间
        if (cleaned_to_header.count("出发航班预离时间")) {
            departure_time_key = cleaned_to_header.at("出发航班预离时间");
        } else {
            for (const auto& pair : row) {
                std::string key = CSVUtils::trimQuotes(pair.first);
                if (key.find("出发航班预离时间") != std::string::npos) {
                    departure_time_key = pair.first;
                    break;
                }
            }
        }
        
        // 查找机位
        if (cleaned_to_header.count("机位")) {
            stand_key = cleaned_to_header.at("机位");
        } else {
            for (const auto& pair : row) {
                std::string key = CSVUtils::trimQuotes(pair.first);
                if (key.find("机位") != std::string::npos) {
                    stand_key = pair.first;
                    break;
                }
            }
        }
        
        // 查找任务对应的航班所需最少人数
        if (cleaned_to_header.count("任务对应的航班所需最少人数")) {
            min_staff_key = cleaned_to_header.at("任务对应的航班所需最少人数");
        } else {
            for (const auto& pair : row) {
                std::string key = CSVUtils::trimQuotes(pair.first);
                if (key.find("任务对应的航班所需最少人数") != std::string::npos || 
                    key.find("所需最少人数") != std::string::npos) {
                    min_staff_key = pair.first;
                    break;
                }
            }
        }
        
        // 提取数据
        std::string task_name = task_name_key.empty() ? "" : CSVUtils::trimQuotes(row.count(task_name_key) ? row.at(task_name_key) : "");
        std::string arrival_flight_id = arrival_flight_id_key.empty() ? "" : CSVUtils::trimQuotes(row.count(arrival_flight_id_key) ? row.at(arrival_flight_id_key) : "");
        std::string departure_flight_id = departure_flight_id_key.empty() ? "" : CSVUtils::trimQuotes(row.count(departure_flight_id_key) ? row.at(departure_flight_id_key) : "");
        std::string arrival_time_str = arrival_time_key.empty() ? "" : CSVUtils::trimQuotes(row.count(arrival_time_key) ? row.at(arrival_time_key) : "");
        std::string departure_time_str = departure_time_key.empty() ? "" : CSVUtils::trimQuotes(row.count(departure_time_key) ? row.at(departure_time_key) : "");
        std::string stand_str = stand_key.empty() ? "" : CSVUtils::trimQuotes(row.count(stand_key) ? row.at(stand_key) : "");
        std::string min_staff_str = min_staff_key.empty() ? "" : CSVUtils::trimQuotes(row.count(min_staff_key) ? row.at(min_staff_key) : "");
        
        // 根据任务名称判断航班类型
        int flight_type = -1;
        if (task_name.find("进港卸机") != std::string::npos) {
            flight_type = static_cast<int>(zhuangxie_class::FlightType::DOMESTIC_ARRIVAL);  // 0
        } else if (task_name.find("出港装机") != std::string::npos) {
            flight_type = static_cast<int>(zhuangxie_class::FlightType::DOMESTIC_DEPARTURE);  // 1
        } else {
            // 如果任务名称不匹配，跳过
            continue;
        }
        
        // 使用到达或出发航班ID作为唯一标识
        std::string flight_key = !arrival_flight_id.empty() ? arrival_flight_id : departure_flight_id;
        if (flight_key.empty()) {
            // 如果没有航班ID，使用时间作为后备标识
            flight_key = arrival_time_str + "_" + departure_time_str;
            if (flight_key == "_") {
                continue;  // 至少需要一个时间或ID
            }
        }
        
        // 创建或获取Flight对象
        if (flight_map.find(flight_key) == flight_map.end()) {
            zhuangxie_class::Flight flight;
            flight_map[flight_key] = flight;
        }
        
        zhuangxie_class::Flight& flight = flight_map[flight_key];
        
        // 设置航班类型
        flight.setFlightType(flight_type);
        
        // 设置进港时间（到达航班预达时间）
        if (!arrival_time_str.empty()) {
            long arrival_time = CSVUtils::parseDateTimeString(arrival_time_str);
            flight.setArrivalTime(arrival_time);
        }
        
        // 设置出港时间（出发航班预离时间）
        if (!departure_time_str.empty()) {
            long departure_time = CSVUtils::parseDateTimeString(departure_time_str);
            flight.setDepartureTime(departure_time);
        }
        
        // 设置VIP通勤时间，默认为5分钟 = 300秒
        flight.setVipTravelTime(5 * 60);  // 5分钟 = 300秒
        
        // 设置货量：如果"任务对应的航班所需最少人数"为3，则认为是1.5吨，否则认为是6吨
        double cargo_weight = 6.0;  // 默认6吨
        if (!min_staff_str.empty()) {
            try {
                int min_staff = std::stoi(min_staff_str);
                if (min_staff == 3) {
                    cargo_weight = 1.5;
                } else {
                    cargo_weight = 6.0;
                }
            } catch (...) {
                // 解析失败，使用默认值6吨
                cargo_weight = 6.0;
            }
        }
        
        // 根据航班类型设置货量
        if (flight_type == static_cast<int>(zhuangxie_class::FlightType::DOMESTIC_ARRIVAL)) {
            // 进港卸机：设置进港货量
            flight.setArrivalCargo(cargo_weight);
            flight.setDepartureCargo(0.0);
        } else if (flight_type == static_cast<int>(zhuangxie_class::FlightType::DOMESTIC_DEPARTURE)) {
            // 出港装机：设置出港货量
            flight.setArrivalCargo(0.0);
            flight.setDepartureCargo(cargo_weight);
        }
        
        // 设置机位
        if (!stand_str.empty()) {
            try {
                int stand = std::stoi(stand_str);
                flight.setStand(stand);
            } catch (...) {
                // 解析失败，忽略
            }
        }
        
        // report_time_不考虑，保持默认值0
    }
    
    // 转换为vector
    for (auto& pair : flight_map) {
        flights.push_back(pair.second);
    }
    
    return flights;
}

} // namespace CSVLoader
} // namespace AirportStaffScheduler

