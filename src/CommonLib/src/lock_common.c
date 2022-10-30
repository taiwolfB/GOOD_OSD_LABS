#include "common_lib.h"
#include "lock_common.h"

#ifndef _COMMONLIB_NO_LOCKS_

PFUNC_LockInit           LockInit = NULL;

PFUNC_LockAcquire        LockAcquire = NULL;

PFUNC_LockTryAcquire     LockTryAcquire = NULL;

PFUNC_LockRelease        LockRelease = NULL;

PFUNC_LockIsOwner        LockIsOwner = NULL;

#pragma warning(push)
// warning C4028: formal parameter 1 different from declaration
#pragma warning(disable:4028)
// 5. A spinlock uses the busy waiting technique, thus it must not be used for large portions of code
//    because it will waste too much CPU power, unlike the Mutex which uses the block waiting technique
//    by alternatively blocking and unblocking threads for a better efficiency 
//    which can be used for synchronizing large portions of code.
//    The spinlock is a primitive synchronization mechanism which are provided by the OS, 
//    while the mutex is an executive one.
void
LockSystemInit(
    IN      BOOLEAN             MonitorSupport
    )
{

    if (MonitorSupport)
    {
        // we have monitor support
        LockInit = MonitorLockInit;
        LockAcquire = MonitorLockAcquire;
        LockTryAcquire = MonitorLockTryAcquire;
        LockIsOwner = MonitorLockIsOwner;
        LockRelease = MonitorLockRelease;
    }
    else
    {
        // use classic spinlock
        LockInit = SpinlockInit;
        LockAcquire = SpinlockAcquire;
        LockTryAcquire = SpinlockTryAcquire;
        LockIsOwner = SpinlockIsOwner;
        LockRelease = SpinlockRelease;
    }
}
#pragma warning(pop)

#endif // _COMMONLIB_NO_LOCKS_
