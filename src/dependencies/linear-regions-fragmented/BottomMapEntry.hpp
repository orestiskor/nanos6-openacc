#ifndef BOTTOM_MAP_ENTRY_HPP
#define BOTTOM_MAP_ENTRY_HPP


#include <boost/intrusive/avl_set.hpp>
#include <boost/intrusive/avl_set_hook.hpp>

#include "DataAccessRange.hpp"


class Task;
struct BottomMapEntry;


struct BottomMapEntryLinkingArtifacts {
	#if NDEBUG
		typedef boost::intrusive::link_mode<boost::intrusive::normal_link> link_mode_t;
	#else
		typedef boost::intrusive::link_mode<boost::intrusive::safe_link> link_mode_t;
	#endif
	
	typedef boost::intrusive::avl_set_member_hook<link_mode_t> hook_type;
	typedef hook_type* hook_ptr;
	typedef const hook_type* const_hook_ptr;
	typedef BottomMapEntry value_type;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	
	static inline constexpr hook_ptr to_hook_ptr (value_type &value);
	static inline constexpr const_hook_ptr to_hook_ptr(const value_type &value);
	static inline pointer to_value_ptr(hook_ptr n);
	static inline const_pointer to_value_ptr(const_hook_ptr n);
};


struct BottomMapEntry {
	BottomMapEntryLinkingArtifacts::hook_type _links;
	
	DataAccessRange _range;
	Task *_task;
	bool _local;
	
	BottomMapEntry(DataAccessRange range, Task *task, bool local)
		: _links(), _range(range), _task(task), _local(local)
	{
	}
	
	DataAccessRange const &getAccessRange() const
	{
		return _range;
	}
	
	DataAccessRange &getAccessRange()
	{
		return _range;
	}
	
};


inline constexpr BottomMapEntryLinkingArtifacts::hook_ptr
BottomMapEntryLinkingArtifacts::to_hook_ptr (BottomMapEntryLinkingArtifacts::value_type &value)
{
	return &value._links;
}

inline constexpr BottomMapEntryLinkingArtifacts::const_hook_ptr
BottomMapEntryLinkingArtifacts::to_hook_ptr(const BottomMapEntryLinkingArtifacts::value_type &value)
{
	return &value._links;
}

inline BottomMapEntryLinkingArtifacts::pointer
BottomMapEntryLinkingArtifacts::to_value_ptr(BottomMapEntryLinkingArtifacts::hook_ptr n)
{
	return (BottomMapEntryLinkingArtifacts::pointer)
		boost::intrusive::get_parent_from_member<BottomMapEntry>(
			n,
			&BottomMapEntry::_links
		);
}

inline BottomMapEntryLinkingArtifacts::const_pointer
BottomMapEntryLinkingArtifacts::to_value_ptr(BottomMapEntryLinkingArtifacts::const_hook_ptr n)
{
	return (BottomMapEntryLinkingArtifacts::const_pointer)
		boost::intrusive::get_parent_from_member<BottomMapEntry>(
			n,
			&BottomMapEntry::_links
		);
}


#endif // BOTTOM_MAP_ENTRY_HPP