#include "SchedulingAlgorithm.h"
#include <unordered_map>
#include <algorithm>

using namespace AirportStaffScheduler;

void SchedulingAlgorithm::assignTasksToShifts() {
    // TODO: 实现任务分配逻辑
    // - 遍历 tasks_
    // - 根据规则（时间窗口、位置、人员资质、车辆等）将任务分配给合适的班次（shift）
    // - 修改 task 的班次ID 或关联 shift 对象

    // 顺序派工算法示例
    // 1. 任务按开始时间排序
    std::sort(tasks_.begin(), tasks_.end(),
        [](const Task& a, const Task& b) {
            return a.getTaskStartTime() < b.getTaskStartTime();
        });

    // 2. 初始化 staffPtrs_（如果尚未初始化）
    if (staffPtrs_.empty()) {
        staffPtrs_.reserve(staffList_.size());
        for (auto& staff : staffList_) {
            staffPtrs_.push_back(&staff);
        }
    }

    // 3. 遍历任务
    for (auto& task : tasks_) {
        const auto& requiredQuals = task.getRequiredQualifications();
        Staff* selectedStaff = nullptr;
        // 从最早空闲的员工开始找
        for (Staff* staff : staffPtrs_) {
            if (staff->hasAllQualifications(requiredQuals)) {
                selectedStaff = staff;
                break;
            }
        }

        if (!selectedStaff) {
            // TODO: 处理无匹配班次
            continue;
        }

        // 分配任务
        selectedStaff->assignTask(task);
        task.setAssignedStaffId(selectedStaff->getStaffId());

        // 更新 shiftPtrs_：移动该员工到新位置
        auto it = std::find(staffPtrs_.begin(), staffPtrs_.end(), selectedStaff);
        if (it != staffPtrs_.end()) {
            Staff* ptr = *it;
            staffPtrs_.erase(it);

            auto insertPos = std::lower_bound(
                staffPtrs_.begin(), staffPtrs_.end(), ptr,
                [](const Staff* a, const Staff* b) {
                    return a->getLatestEndTime() < b->getLatestEndTime();
                }
            );
            staffPtrs_.insert(insertPos, ptr);
        }
    }
}

void SchedulingAlgorithm::updateTaskTimesFromFlightSchedules() {
    // TODO: 根据航班时间表动态更新任务时间
    // - 建立任务与航班的映射（如通过任务中的航班ID或机位）
    // - 用 flightSchedules_ 中的时间（如 cabinOpenTime, pushbackTime）调整任务的 startTime/endTime
}