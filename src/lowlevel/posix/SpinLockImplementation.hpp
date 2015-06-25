#ifndef APPLE_SPIN_LOCK_IMPLEMENTATION_HPP
#define APPLE_SPIN_LOCK_IMPLEMENTATION_HPP


#ifndef SPIN_LOCK_HPP
	#error Include SpinLock.h instead
#endif


inline SpinLock::SpinLock()
{
	pthread_spin_init(&_lock, PTHREAD_PROCESS_PRIVATE);
#ifndef NDEBUG
	_owner = nullptr;
#endif
}

inline SpinLock::~SpinLock()
{
	assertUnowned();
	pthread_spin_destroy(&_lock);
}

inline void SpinLock::lock()
{
	pthread_spin_lock(&_lock);
	assertUnowned();
	setOwner();
}

inline bool SpinLock::tryLock()
{
	bool success = (pthread_spin_trylock(&_lock) == 0);
	
	if (success) {
		assertUnowned();
		setOwner();
	}
	
	return success;
}

inline void SpinLock::unlock()
{
	assertCurrentOwner();
	unsetOwner();
	pthread_spin_unlock(&_lock);
}

#ifndef NDEBUG
inline bool SpinLock::isLockedByThisThread()
{
	return (_owner == ompss_debug::getCurrentWorkerThread());
}
#endif


#endif // APPLE_SPIN_LOCK_IMPLEMENTATION_HPP
