#include "SchedulingAlgorithm.h"
#include <unordered_map>
#include <algorithm>

using namespace AirportStaffScheduler;

void SchedulingAlgorithm::populateShiftQualifications() {
    // 构建 staff ID -> 资质 的映射
    std::unordered_map<std::string, std::vector<std::string>> staffIdToQuals;
    for (const auto& staff : staffList_) {
        staffIdToQuals[staff.getStaffId()] = staff.getQualifications();
    }

    // 为每个班次设置资质
    for (auto& shift : shifts_) {
        const std::string& staffId = shift.getStaffId();
        auto it = staffIdToQuals.find(staffId);
        if (it != staffIdToQuals.end()) {
            shift.setQualifications(it->second);
        }
        else {
            // 可选：警告或设为空集合
            shift.setQualifications({});
        }
    }
}

void SchedulingAlgorithm::assignTasksToShifts() {
    // TODO: 实现任务分配逻辑
    // - 遍历 tasks_
    // - 根据规则（时间窗口、位置、人员资质、车辆等）将任务分配给合适的班次（shift）
    // - 修改 task 的班次ID 或关联 shift 对象

    // 顺序派工算法示例

    populateShiftQualifications();
    // 1. 任务按开始时间排序
    std::sort(tasks_.begin(), tasks_.end(),
        [](const Task& a, const Task& b) {
            return a.getTaskStartTime() < b.getTaskStartTime();
        });

    // 2. 初始化 shiftPtrs_（如果尚未初始化）
    if (shiftPtrs_.empty()) {
        shiftPtrs_.reserve(shifts_.size());
        for (auto& shift : shifts_) {
            shiftPtrs_.push_back(&shift);
        }
    }

    // 按 latestEndTime 排序（初始值为班次 startTime）
    /*std::sort(shiftPtrs_.begin(), shiftPtrs_.end(),
        [](const Shift* a, const Shift* b) {
            return a->getLatestEndTime() < b->getLatestEndTime();
        });*/

    // 3. 遍历任务
    for (auto& task : tasks_) {
        const auto& requiredQuals = task.getRequiredQualifications();
        Shift* selectedShift = nullptr;
        // 从最早空闲的班次开始找
        for (Shift* shift : shiftPtrs_) {
            if (shift->hasAllQualifications(requiredQuals)) {
                selectedShift = shift;
                break;
            }
        }

        if (!selectedShift) {
            // TODO: 处理无匹配班次
            continue;
        }

        // 分配任务
        selectedShift->assignTask(task);
        task.setAssignedShiftId(selectedShift->getShiftId());

        // 更新 shiftPtrs_：移动该班次到新位置
        auto it = std::find(shiftPtrs_.begin(), shiftPtrs_.end(), selectedShift);
        if (it != shiftPtrs_.end()) {
            Shift* ptr = *it;
            shiftPtrs_.erase(it);

            auto insertPos = std::lower_bound(
                shiftPtrs_.begin(), shiftPtrs_.end(), ptr,
                [](const Shift* a, const Shift* b) {
                    return a->getLatestEndTime() < b->getLatestEndTime();
                }
            );
            shiftPtrs_.insert(insertPos, ptr);
        }
    }
}

void SchedulingAlgorithm::updateTaskTimesFromFlightSchedules() {
    // TODO: 根据航班时间表动态更新任务时间
    // - 建立任务与航班的映射（如通过任务中的航班ID或机位）
    // - 用 flightSchedules_ 中的时间（如 cabinOpenTime, pushbackTime）调整任务的 startTime/endTime
}