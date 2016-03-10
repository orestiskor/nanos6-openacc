#ifndef DATA_ACCESS_BASE_HPP
#define DATA_ACCESS_BASE_HPP

#include <cassert>

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>

#include <InstrumentDataAccessId.hpp>
#include <TaskDataAccesses.hpp>
#include <TaskDataAccessesLinkingArtifacts.hpp>


class Task;


#include "../DataAccessType.hpp"


struct DataAccessBase {
	#if NDEBUG
		typedef boost::intrusive::link_mode<boost::intrusive::normal_link> link_mode_t;
	#else
		typedef boost::intrusive::link_mode<boost::intrusive::safe_link> link_mode_t;
	#endif
	
	//! Links used by the list of accesses of a Task
	TaskDataAccessesHook _taskDataAccessesLinks;
	
	//! Type of access: read, write, ...
	DataAccessType _type;
	
	//! True iff the access is weak
	bool _weak;
	
	//! Tasks to which the access corresponds
	Task *_originator;
	
	//! An identifier for the instrumentation
	Instrument::data_access_id_t _instrumentationId;
	
	DataAccessBase(
		DataAccessType type,
		bool weak,
		Task *originator,
		Instrument::data_access_id_t instrumentationId
	)
		: _taskDataAccessesLinks(), 
		_type(type), _weak(weak), _originator(originator),
		_instrumentationId(instrumentationId)
	{
		assert(originator != 0);
	}
};


#endif // DATA_ACCESS_BASE_HPP
