# 项目代码文件说明

本项目包含以下核心代码文件，用于实现地服任务调度系统的基本数据模型与调度逻辑：

- **`Task.h` / `Task.cpp`**  
  定义任务（Task）类，包含任务ID、名称、位置、类型、开始/结束时间、所需资质列表、是否必做、是否锁定等属性。

- **`Shift.h` / `Shift.cpp`**  
  定义班次（Shift）类，表示员工的工作时段，包含班次ID、关联的员工ID、工作起止时间、已分配任务的最晚结束时间，以及缓存的资质集合（从对应员工复制而来）。

- **`Staff.h` / `Staff.cpp`**  
  定义员工（Staff）类，包含员工ID、姓名、资质列表等信息。

- **`FlightSchedule.h`**  
  定义航班计划（FlightSchedule）结构体，包含航班号、登机开始时间、舱门开启/关闭时间、推拖时间、桥撤时间、UCTOT 等关键时间节点。

- **`GateCounterInfo.h`**  
  定义机位/柜台信息（GateCounterInfo），包含编号、相邻机位/柜台、区域等属性。

- **`TravelTime.h`**  
  定义任务间的路程时间（TravelTime）。

- **`VehicleInfo.h`**  
  定义车辆信息（VehicleInfo），如车辆ID、类型、可用状态等。

- **`TemporaryTask.h`**  
  定义临时任务（TemporaryTask），用于表示非计划内、动态插入的任务。

- **`SchedulingAlgorithm.h` / `SchedulingAlgorithm.cpp`**  
  实现核心调度算法。

- **`DateTimeUtils.h` / `DateTimeUtils.cpp`**  
  提供时间字符串（如 `"2024/01/01 08:30"`）与 `std::chrono::system_clock::time_point` 之间的转换工具函数。

- **`StringUtils.h` / `StringUtils.cpp`**  
  提供通用字符串处理函数，如分割、去首尾空格等。

- **`main_test.cpp`**  
  测试入口文件，用于构造示例数据并调用调度算法进行验证。
