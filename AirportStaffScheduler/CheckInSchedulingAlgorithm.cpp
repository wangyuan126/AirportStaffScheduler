// CheckInSchedulingAlgorithm.cpp
#include "CheckInSchedulingAlgorithm.h"
#include <algorithm>
#include <numeric>

namespace AirportStaffScheduler {


    void CheckInSchedulingAlgorithm::preprocessTasks() {
        
    }

    void CheckInSchedulingAlgorithm::assignTasksToShiftImpl() {
        // 顺序派工示例
        const int numTasks = static_cast<int>(tasks_.size());
        const int numShift = static_cast<int>(shiftList_.size());
        if (numTasks == 0 || numShift == 0) return;

        // 步骤1: 创建任务索引代理，并按开始时间排序
        std::vector<int> taskIndices(numTasks);
        std::iota(taskIndices.begin(), taskIndices.end(), 0);
        std::sort(taskIndices.begin(), taskIndices.end(),
            [this](int a, int b) {
                return tasks_[a].getTaskStartTime() < tasks_[b].getTaskStartTime();
            });

        // 步骤2: 创建班次索引代理
        std::vector<int> shiftIndices(numShift);
        std::iota(shiftIndices.begin(), shiftIndices.end(), 0);

        // 步骤3: 遍历每个任务（按时间顺序）
        for (int taskIdx : taskIndices) {
            if (isTaskAssigned(taskIdx)) continue;

            const auto& task = tasks_[taskIdx];
            const auto& requiredQuals = task.getRequiredQualifications();

            // 在 staffIndices 中找第一个满足条件的员工
            int selectedShiftIdx = -1;
            for (int shiftIdx : shiftIndices) {
                const auto& shift = shiftList_[shiftIdx];
                const auto& staff = staffList_[shiftToStaffIndex_[shiftIdx]];
                // 检查资质
                if (staff.hasAllQualifications(requiredQuals)) {
                    selectedShiftIdx = shiftIdx;
                    break;
                }
            }

            if (selectedShiftIdx == -1) {
                // TODO: 记录未分配任务（可扩展为告警/降级策略）
                continue;
            }

            assignTaskToShift(taskIdx, selectedShiftIdx);

            auto it = std::find(shiftIndices.begin(), shiftIndices.end(), selectedShiftIdx);
            // Step 2: 移除该索引
            int movedStaffIdx = *it;
            shiftIndices.erase(it);

            // Step 3: 找到新的插入位置（保持按 latestEndTime 升序）
            auto insertPos = std::lower_bound(
                shiftIndices.begin(),
                shiftIndices.end(),
                movedStaffIdx,
                [this](int a, int b) {
                    return shiftList_[a].getLatestEndTime() < shiftList_[b].getLatestEndTime();
                }
            );
            // Step 4: 插入
            shiftIndices.insert(insertPos, movedStaffIdx);
        }
    }

    void CheckInSchedulingAlgorithm::validateAssignmentResult() {
        
    }

} // namespace AirportStaffScheduler