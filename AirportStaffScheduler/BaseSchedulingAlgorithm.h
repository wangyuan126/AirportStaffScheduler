// BaseSchedulingAlgorithm.h (终极精简版)
#pragma once
#include <vector>
#include "Task.h"
#include "Staff.h"
#include "Shift.h"
#include "TemporaryTask.h"
#include <unordered_map>

namespace AirportStaffScheduler {

    class BaseSchedulingAlgorithm {
    public:
        BaseSchedulingAlgorithm(
            std::vector<Task> tasks,
            std::vector<Staff> staffList,
            std::vector<Shift> shiftList,
            std::vector<TemporaryTask> temporaryTasks = {}
        )
            : tasks_(std::move(tasks))
            , staffList_(std::move(staffList))
            , shiftList_(std::move(shiftList))
            , temporaryTasks_(std::move(temporaryTasks))
        {
            // 初始化分配表：-1 表示未分配
            shiftToStaffIndex_.assign(shiftList_.size(), -1);
            staffToShiftIndices_.assign(staffList_.size(), {});
            taskToShiftIndex_.assign(tasks_.size(), -1);
            shiftToTaskIndices_.assign(shiftList_.size(), {});
        }

        virtual ~BaseSchedulingAlgorithm() = default;

        // 调度主函数
        virtual void assignTasksToShift() {
            buildShiftStaffIndexMaps();
            preprocessTasks();
            assignTasksToShiftImpl();
            validateAssignmentResult();
        }

        void buildShiftStaffIndexMaps() {
            const int numShift = static_cast<int>(shiftList_.size());
            const int numStaff = static_cast<int>(staffList_.size());

            // 步骤1: 构建 staffId → staffIndex 映射（仅用于初始化，一次性）
            std::unordered_map<std::string, int> staffIdToIndex;
            staffIdToIndex.reserve(numStaff);
            for (int i = 0; i < numStaff; ++i) {
                staffIdToIndex[staffList_[i].getStaffId()] = i;
            }

            // 步骤2: 初始化映射表
            shiftToStaffIndex_.assign(numShift, -1);           // 默认 -1：未关联或无效
            staffToShiftIndices_.assign(numStaff, std::vector<int>());

            // 步骤3: 遍历每个班次，建立索引关系
            for (int shiftIdx = 0; shiftIdx < numShift; ++shiftIdx) {
                const std::string& assignedStaffId = shiftList_[shiftIdx].getStaffId();

                if (assignedStaffId.empty()) {
                    // 班次未分配员工，保留 shiftToStaffIndex_[shiftIdx] = -1
                    continue;
                }

                auto it = staffIdToIndex.find(assignedStaffId);
                if (it != staffIdToIndex.end()) {
                    int staffIdx = it->second;
                    shiftToStaffIndex_[shiftIdx] = staffIdx;
                    staffToShiftIndices_[staffIdx].push_back(shiftIdx);
                }
                else {
                    // 警告：班次指向了不存在的员工ID（数据不一致）
                    // 可选：记录日志或抛异常
                    shiftToStaffIndex_[shiftIdx] = -1; // 标记为无效
                }
            }
        }

        // ===== 核心分配接口（子类使用）=====
        void assignTaskToShift(int taskIdx, int shiftIdx) {
            if (taskIdx < 0 || taskIdx >= static_cast<int>(tasks_.size())) return;
            if (shiftIdx < 0 || shiftIdx >= static_cast<int>(staffList_.size())) return;
            if (taskToShiftIndex_[taskIdx] != -1) return; // 已分配，避免重复

            // 记录双向关系
            taskToShiftIndex_[taskIdx] = shiftIdx;
            shiftToTaskIndices_[shiftIdx].push_back(taskIdx);

            // 同步更新 Task 对象（保持对外兼容）
            tasks_[taskIdx].setAssignedShiftId(shiftList_[shiftIdx].getShiftId());

            // 更新员工状态（如 latestEndTime）
            shiftList_[shiftIdx].updateLatestEndTime(tasks_[taskIdx].getTaskEndTime());
        }

        // ===== 查询接口 =====
        bool isTaskAssigned(int taskIdx) const {
            return taskIdx >= 0 && taskIdx < static_cast<int>(taskToShiftIndex_.size()) &&
                taskToShiftIndex_[taskIdx] != -1;
        }

        int getAssignedStaffIndexForTask(int taskIdx) const {
            return (isTaskAssigned(taskIdx)) ? taskToShiftIndex_[taskIdx] : -1;
        }

        const std::vector<int>& getAssignedTaskIndicesForStaff(int staffIdx) const {
            if (staffIdx < 0 || staffIdx >= static_cast<int>(shiftToTaskIndices_.size()))
                static const std::vector<int> empty;
            return shiftToTaskIndices_[staffIdx];
        }

        // ===== Getters =====
        const std::vector<Task>& getTasks() const { return tasks_; }
        const std::vector<Staff>& getStaffList() const { return staffList_; }
        const std::vector<TemporaryTask>& getTemporaryTasks() const { return temporaryTasks_; }

    protected:
        virtual void preprocessTasks() {}
        virtual void assignTasksToShiftImpl() = 0;
        virtual void validateAssignmentResult() {
            // 任务分配结果检查
        }

        virtual void handleValidationFailure(int taskIdx, int staffIdx, const std::string& reason) {
            // 子类可重写：记录日志、抛异常等
        }

        // === 核心数据成员 ===
        std::vector<Task> tasks_;
        std::vector<Staff> staffList_;
        std::vector<Shift> shiftList_;
        std::vector<TemporaryTask> temporaryTasks_;

        // === 分配关系（纯索引，无 ID 映射）===
        std::vector<int> shiftToStaffIndex_;             // 班次 → 员工索引
        std::vector<std::vector<int>> staffToShiftIndices_; // 员工 → 班次索引列表
        std::vector<int> taskToShiftIndex_;             // 任务 → 班次索引
        std::vector<std::vector<int>> shiftToTaskIndices_; // 班次 → 任务索引列表
    };

} // namespace AirportStaffScheduler