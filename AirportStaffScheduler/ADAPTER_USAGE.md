# 公共类适配器使用说明

本文档说明如何使用 `CommonAdapterUtils.h` 中的转换工具，使 `vip_first_class_algo` 和 `zhuangxie_class` 模块能够复用 `AirportStaffScheduler` 目录下的公共类。

## 概述

适配器工具提供了以下转换功能：

1. **时间转换**：`DateTime` (std::chrono::system_clock::time_point) ? `long` (从2020-01-01 00:00:00开始的秒数)
2. **资质转换**：字符串列表 ? 位掩码
3. **类适配**：公共类 ? 模块内部类

## 转换函数列表

### 时间转换

```cpp
// DateTime -> long (秒数)
long DateTimeToSeconds(const DateTime& dt);

// long (秒数) -> DateTime
DateTime SecondsToDateTime(long seconds);
```

### 资质转换

```cpp
// 资质字符串列表 -> 位掩码
int QualificationsToMask(const std::vector<std::string>& quals);

// 位掩码 -> 资质字符串列表
std::vector<std::string> MaskToQualifications(int mask);
```

### 类适配

```cpp
// Staff -> EmployeeInfo
vip_first_class::EmployeeInfo StaffToEmployeeInfo(const Staff& staff);

// EmployeeInfo -> Staff
Staff EmployeeInfoToStaff(const vip_first_class::EmployeeInfo& emp_info);

// Task -> TaskDefinition
vip_first_class::TaskDefinition TaskToTaskDefinition(
    const Task& task,
    vip_first_class::TaskType task_type = vip_first_class::TaskType::DISPATCH);

// TaskDefinition -> Task
Task TaskDefinitionToTask(const vip_first_class::TaskDefinition& task_def);

// Shift -> vip_first_class::Shift
vip_first_class::Shift ShiftToVipShift(
    const AirportStaffScheduler::Shift& shift,
    int position = 1);

// vip_first_class::Shift -> Shift列表
std::vector<AirportStaffScheduler::Shift> VipShiftToShifts(
    const vip_first_class::Shift& vip_shift,
    const std::string& shift_id_prefix = "",
    const DateTime& start_time = GetEpochTime(),
    const DateTime& end_time = GetEpochTime(),
    const std::string& bound_terminal = "");
```

## 模块接口

### vip_first_class_algo 模块

新增了 `scheduleTasksFromCommon` 函数，可以直接使用公共类：

```cpp
void TaskScheduler::scheduleTasksFromCommon(
    const std::vector<AirportStaffScheduler::Task>& tasks,
    const std::vector<AirportStaffScheduler::Shift>& shifts,
    const std::vector<AirportStaffScheduler::Staff>& staffs);
```

**使用示例：**

```cpp
#include "vip_first_class_algo/task_scheduler.h"
#include "Staff.h"
#include "Task.h"
#include "Shift.h"

// 创建公共类对象
std::vector<AirportStaffScheduler::Staff> staffs;
std::vector<AirportStaffScheduler::Task> tasks;
std::vector<AirportStaffScheduler::Shift> shifts;

// ... 填充数据 ...

// 使用适配器接口进行调度
vip_first_class::TaskScheduler scheduler;
scheduler.scheduleTasksFromCommon(tasks, shifts, staffs);
```

### zhuangxie_class 模块

新增了 `scheduleLoadTasksFromCommon` 函数：

```cpp
void LoadScheduler::scheduleLoadTasksFromCommon(
    const std::vector<AirportStaffScheduler::Staff>& staffs,
    const std::vector<AirportStaffScheduler::Task>& tasks,
    const std::vector<AirportStaffScheduler::Shift>& common_shifts,
    long default_travel_time = 480,
    const vector<ShiftBlockPeriod>& block_periods = vector<ShiftBlockPeriod>());
```

**使用示例：**

```cpp
#include "zhuangxie_class/load_scheduler.h"
#include "Staff.h"
#include "Task.h"
#include "Shift.h"

// 创建公共类对象
std::vector<AirportStaffScheduler::Staff> staffs;
std::vector<AirportStaffScheduler::Task> tasks;
std::vector<AirportStaffScheduler::Shift> shifts;

// ... 填充数据 ...

// 使用适配器接口进行调度
zhuangxie_class::LoadScheduler scheduler;
scheduler.scheduleLoadTasksFromCommon(staffs, tasks, shifts);
```

## 注意事项

1. **时间基准**：所有时间转换基于 2020-01-01 00:00:00 作为基准点。

2. **资质映射**：资质字符串支持多种格式：
   - 中文：厅内资质、外场资质、前台资质、调度资质
   - 英文：HALL_INTERNAL、EXTERNAL、FRONT_DESK、DISPATCH
   - 包含关键词的字符串也会被识别（如"厅内"、"外场"等）

3. **Shift转换**：
   - `AirportStaffScheduler::Shift` 代表单个员工的班次
   - `vip_first_class::Shift` 包含多个位置到员工的映射
   - 转换时会将多个单员工Shift按班次类型分组

4. **TaskType映射**：从 `Task` 转换到 `TaskDefinition` 时，需要指定 `TaskType`。当前实现使用默认值，实际使用时可能需要根据 `task_name` 进行更精确的映射。

5. **航班信息**：`zhuangxie_class` 模块需要 `Flight` 对象，当前适配器实现中这部分需要根据实际业务逻辑进行完善。

## 文件位置

- 适配器工具：`AirportStaffScheduler/CommonAdapterUtils.h`
- vip_first_class_algo 接口：`vip_first_class_algo/task_scheduler.h` 和 `task_scheduler.cpp`
- zhuangxie_class 接口：`zhuangxie_class/load_scheduler.h` 和 `load_scheduler.cpp`


