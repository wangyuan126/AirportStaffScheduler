#pragma once
#include <vector>
#include "Task.h"               // 假设已定义
#include "Staff.h"
#include "Shift.h"
#include "GateCounterInfo.h"
#include "TravelTime.h"
#include "VehicleInfo.h"
#include "TemporaryTask.h"
#include "FlightSchedule.h"

namespace AirportStaffScheduler{
    class SchedulingAlgorithm {
    public:
        // 构造函数：接收所有输入列表（可移动或拷贝）
        SchedulingAlgorithm(
            std::vector<Task> tasks,
            std::vector<Staff> staffList,
            std::vector<Shift> shifts,
            std::vector<GateCounterInfo> gateCounterInfos,
            std::vector<TravelTime> travelTimes,
            std::vector<VehicleInfo> vehicleInfos,          // 非必须，但保留
            std::vector<TemporaryTask> temporaryTasks,
            std::vector<FlightSchedule> flightSchedules
        )
            : tasks_(std::move(tasks))
            , staffList_(std::move(staffList))
            , shifts_(std::move(shifts))
            , gateCounterInfos_(std::move(gateCounterInfos))
            , travelTimes_(std::move(travelTimes))
            , vehicleInfos_(std::move(vehicleInfos))
            , temporaryTasks_(std::move(temporaryTasks))
            , flightSchedules_(std::move(flightSchedules))
        {

        }

        void populateShiftQualifications();

        // 算法1：任务分配 —— 将任务分给相应班次（修改任务的班次ID等）
        void assignTasksToShifts();

        // 算法2：根据航班时间表更新任务时间（如登机、撤桥等触发的任务时间调整）
        void updateTaskTimesFromFlightSchedules();

        // ===== Getters (用于获取结果) =====
        const std::vector<Task>& getTasks() const { return tasks_; }
        const std::vector<Staff>& getStaffList() const { return staffList_; }
        const std::vector<Shift>& getShifts() const { return shifts_; }
        const std::vector<GateCounterInfo>& getGateCounterInfos() const { return gateCounterInfos_; }
        const std::vector<TravelTime>& getTravelTimes() const { return travelTimes_; }
        const std::vector<VehicleInfo>& getVehicleInfos() const { return vehicleInfos_; }
        const std::vector<TemporaryTask>& getTemporaryTasks() const { return temporaryTasks_; }
        const std::vector<FlightSchedule>& getFlightSchedules() const { return flightSchedules_; }

    private:
        std::vector<Task> tasks_;
        std::vector<Staff> staffList_;
        std::vector<Shift> shifts_;
        std::vector<GateCounterInfo> gateCounterInfos_;
        std::vector<TravelTime> travelTimes_;
        std::vector<VehicleInfo> vehicleInfos_;          // 非必须，但保留
        std::vector<TemporaryTask> temporaryTasks_;
        std::vector<FlightSchedule> flightSchedules_;
        std::vector<Shift*> shiftPtrs_;
    };
}