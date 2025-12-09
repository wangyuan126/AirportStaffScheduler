/**
 * @file task_type.h
 * @brief 任务类型枚举
 * 
 * 定义所有任务类型的枚举，基于 Task.txt 文件生成
 */

#ifndef VIP_FIRST_CLASS_TASK_TYPE_H
#define VIP_FIRST_CLASS_TASK_TYPE_H

namespace vip_first_class {

/**
 * @brief 任务类型枚举
 * 
 * 根据 Task.txt 文件中定义的所有任务类型
 */
enum class TaskType {
    // 调度类
    DISPATCH = 0,                      ///< 调度
    
    // 国内前台类
    DOMESTIC_FRONT_DESK = 1,           ///< 国内前台
    DOMESTIC_FRONT_DESK_ASSIST = 2,    ///< 国内前台协助
    DOMESTIC_FRONT_DESK_ASSIST2 = 3,   ///< 国内前台协助2
    DOMESTIC_FRONT_DESK_EARLY = 4,     ///< 国内前台早班
    
    // 国际前台类
    INTERNATIONAL_FRONT_DESK_EARLY = 5, ///< 国际前台早班
    INTERNATIONAL_FRONT_DESK_LATE = 6,  ///< 国际前台晚班
    
    // 国际厅内类
    INTERNATIONAL_HALL_EARLY = 7,      ///< 国际厅内早班
    INTERNATIONAL_HALL_LATE = 8,       ///< 国际厅内晚班
    
    // 国内厅内类
    DOMESTIC_HALL_EARLY = 9,           ///< 国内厅内早班
    DOMESTIC_HALL_0830_0930 = 10,      ///< 国内厅内08:30-09:30
    DOMESTIC_HALL_0930_1030 = 11,      ///< 国内厅内09:30-10:30
    DOMESTIC_HALL_1030_1130 = 12,      ///< 国内厅内10:30-11:30
    DOMESTIC_HALL_1130_1230 = 13,      ///< 国内厅内11:30-12:30
    DOMESTIC_HALL_1230_1330 = 14,      ///< 国内厅内12:30-13:30
    DOMESTIC_HALL_1330_1430 = 15,      ///< 国内厅内13:30-14:30
    DOMESTIC_HALL_1430_1530 = 16,      ///< 国内厅内14:30-15:30
    DOMESTIC_HALL_1530_1630 = 17,      ///< 国内厅内15:30-16:30
    DOMESTIC_HALL_1630_1730 = 18,      ///< 国内厅内16:30-17:30
    DOMESTIC_HALL_1730_1830 = 19,      ///< 国内厅内17:30-18:30
    DOMESTIC_HALL_1830_1930 = 20,      ///< 国内厅内18:30-19:30
    DOMESTIC_HALL_1930_2030 = 21,      ///< 国内厅内19:30-20:30
    DOMESTIC_HALL_2030_AFTER = 22,     ///< 国内厅内20:30-航后
    
    // 外场类（国内出港）
    EXTERNAL_DOMESTIC_DEPARTURE_FEW = 23,  ///< 外场（国内出港-少人）
    EXTERNAL_DOMESTIC_DEPARTURE_MANY = 24, ///< 外场（国内出港-多人）
    
    // 外场类（国内进港）
    EXTERNAL_DOMESTIC_ARRIVAL_FEW = 25,    ///< 外场（国内进港-少人）
    EXTERNAL_DOMESTIC_ARRIVAL_MANY = 26,   ///< 外场（国内进港-多人）
    
    // 外场类（国际出港）
    EXTERNAL_INTERNATIONAL_DEPARTURE_FEW = 27,  ///< 外场（国际出港-少人）
    EXTERNAL_INTERNATIONAL_DEPARTURE_MANY = 28, ///< 外场（国际出港-多人）
    
    // 外场类（国际进港）
    EXTERNAL_INTERNATIONAL_ARRIVAL_FEW = 29,    ///< 外场（国际进港-少人）
    EXTERNAL_INTERNATIONAL_ARRIVAL_MANY = 30,   ///< 外场（国际进港-多人）
    
    // 操作间类
    OPERATION_ROOM = 31                          ///< 操作间任务
};

}  // namespace vip_first_class

#endif  // VIP_FIRST_CLASS_TASK_TYPE_H

