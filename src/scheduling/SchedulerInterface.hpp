#ifndef SCHEDULER_INTERFACE_HPP
#define SCHEDULER_INTERFACE_HPP


class HardwarePlace;
class Task;


//! \brief Interface that schedulers must implement
class SchedulerInterface {
public:
	virtual ~SchedulerInterface()
	{
	}
	
	
	//! \brief Add a (ready) task that has been created or freed (but not unblocked)
	//!
	//! \param[in] task the task to be added
	//! \param[in] hardwarePlace the hardware place of the creator or the liberator
	//!
	//! \returns an idle HardwarePlace that is to be resumed or nullptr
	virtual HardwarePlace *addReadyTask(Task *task, HardwarePlace *hardwarePlace) = 0;
	
	//! \brief Add back a task that was blocked but that is now unblocked
	//!
	//! \param[in] unblockedTask the task that has been unblocked
	//! \param[in] hardwarePlace the hardware place of the unblocker
	virtual void taskGetsUnblocked(Task *unblockedTask, HardwarePlace *hardwarePlace) = 0;
	
	//! \brief Check if a hardware place is idle and can be resumed
	//! This call first checks if the hardware place is idle. If so, it marks it as not idle
	//! and returns true. Otherwise it returns false.
	//!
	//! \param[in] hardwarePlace the hardware place to check
	//!
	//! \returns true if the hardware place must be resumed
	virtual bool checkIfIdleAndGrantReactivation(HardwarePlace *hardwarePlace) = 0;
	
	//! \brief Get a ready task for execution
	//!
	//! \param[in] hardwarePlace the hardware place asking for scheduling orders
	//! \param[in] currentTask a task within whose context the resulting task will run
	//!
	//! \returns a ready task or nullptr
	virtual Task *getReadyTask(HardwarePlace *hardwarePlace, Task *currentTask = nullptr) = 0;
	
	//! \brief Get an idle hardware place
	//!
	//! \param[in] force indicates that an idle hardware place must be returned (if any) even if the scheduler does not have any pending work to be assigned
	//!
	//! \returns a hardware place that becomes non idle or nullptr
	virtual HardwarePlace *getIdleHardwarePlace(bool force=false) = 0;
	
};


#endif // SCHEDULER_INTERFACE_HPP
