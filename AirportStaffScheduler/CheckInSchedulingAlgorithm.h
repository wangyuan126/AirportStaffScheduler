// CheckInSchedulingAlgorithm.h
#pragma once

#include "BaseSchedulingAlgorithm.h"
#include <vector>

namespace AirportStaffScheduler {

    class CheckInSchedulingAlgorithm : public BaseSchedulingAlgorithm {
    public:
        CheckInSchedulingAlgorithm(
            std::vector<Task> tasks,
            std::vector<Staff> staffList,
            std::vector<Shift> shiftList,
            std::vector<TemporaryTask> temporaryTasks
        )
            : BaseSchedulingAlgorithm(std::move(tasks), std::move(staffList), std::move(shiftList), std::move(temporaryTasks))
        {}

    protected:
        void preprocessTasks() override;
        void assignTasksToShiftImpl() override;
        void validateAssignmentResult() override;

    private:

    };

} // namespace AirportStaffScheduler