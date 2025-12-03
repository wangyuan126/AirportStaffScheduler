// BaseSchedulingAlgorithm.h (终极精简版)
#pragma once
#include <vector>
#include "Task.h"
#include "Staff.h"
#include "TemporaryTask.h"

namespace AirportStaffScheduler {

    class BaseSchedulingAlgorithm {
    public:
        BaseSchedulingAlgorithm(
            std::vector<Task> tasks,
            std::vector<Staff> staffList,
            std::vector<TemporaryTask> temporaryTasks = {}
        )
            : tasks_(std::move(tasks))
            , staffList_(std::move(staffList))
            , temporaryTasks_(std::move(temporaryTasks))
        {
            // 初始化分配表：-1 表示未分配
            taskToStaffIndex_.assign(tasks_.size(), -1);
            staffToTaskIndices_.assign(staffList_.size(), {});
        }

        virtual ~BaseSchedulingAlgorithm() = default;

        // 调度主函数
        virtual void assignTasksToStaff() {
            preprocessTasks();
            assignTasksToStaffImpl();
            validateAssignmentResult();
        }

        // ===== 核心分配接口（子类使用）=====
        void assignTaskToStaff(int taskIdx, int staffIdx) {
            if (taskIdx < 0 || taskIdx >= static_cast<int>(tasks_.size())) return;
            if (staffIdx < 0 || staffIdx >= static_cast<int>(staffList_.size())) return;
            if (taskToStaffIndex_[taskIdx] != -1) return; // 已分配，避免重复

            // 记录双向关系
            taskToStaffIndex_[taskIdx] = staffIdx;
            staffToTaskIndices_[staffIdx].push_back(taskIdx);

            // 同步更新 Task 对象（保持对外兼容）
            tasks_[taskIdx].setAssignedStaffId(staffList_[staffIdx].getStaffId());

            // 更新员工状态（如 latestEndTime）
            staffList_[staffIdx].assignTask(tasks_[taskIdx]);
        }

        // ===== 查询接口 =====
        bool isTaskAssigned(int taskIdx) const {
            return taskIdx >= 0 && taskIdx < static_cast<int>(taskToStaffIndex_.size()) &&
                taskToStaffIndex_[taskIdx] != -1;
        }

        int getAssignedStaffIndexForTask(int taskIdx) const {
            return (isTaskAssigned(taskIdx)) ? taskToStaffIndex_[taskIdx] : -1;
        }

        const std::vector<int>& getAssignedTaskIndicesForStaff(int staffIdx) const {
            if (staffIdx < 0 || staffIdx >= static_cast<int>(staffToTaskIndices_.size()))
                static const std::vector<int> empty;
            return staffToTaskIndices_[staffIdx];
        }

        // ===== Getters =====
        const std::vector<Task>& getTasks() const { return tasks_; }
        const std::vector<Staff>& getStaffList() const { return staffList_; }
        const std::vector<TemporaryTask>& getTemporaryTasks() const { return temporaryTasks_; }

    protected:
        virtual void preprocessTasks() {}
        virtual void assignTasksToStaffImpl() = 0;
        virtual void validateAssignmentResult() {
            // 任务分配结果检查
        }

        virtual void handleValidationFailure(int taskIdx, int staffIdx, const std::string& reason) {
            // 子类可重写：记录日志、抛异常等
        }

        // === 核心数据成员 ===
        std::vector<Task> tasks_;
        std::vector<Staff> staffList_;
        std::vector<TemporaryTask> temporaryTasks_;

        // === 分配关系（纯索引，无 ID 映射）===
        std::vector<int> taskToStaffIndex_;             // 任务 → 员工索引
        std::vector<std::vector<int>> staffToTaskIndices_; // 员工 → 任务索引列表
    };

} // namespace AirportStaffScheduler