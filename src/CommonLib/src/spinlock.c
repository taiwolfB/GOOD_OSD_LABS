#include "common_lib.h"
#include "lock_common.h"

#ifndef _COMMONLIB_NO_LOCKS_

void
SpinlockInit(
    OUT         PSPINLOCK       Lock
    )
{
    ASSERT(NULL != Lock);

    memzero(Lock, sizeof(SPINLOCK));

    _InterlockedExchange8(&Lock->State, LOCK_FREE);
}


// 1.a) The second parameter is the state of the system returned by the function, which will be further  used by other function, 
// because the SpinlockAcquire will disable the interrupts.
void
SpinlockAcquire(
    INOUT       PSPINLOCK       Lock,
    OUT         INTR_STATE*     IntrState
    )
{
    PVOID pCurrentCpu;

    ASSERT(NULL != Lock);
    ASSERT(NULL != IntrState);

    *IntrState = CpuIntrDisable();

    pCurrentCpu = CpuGetCurrent();

    ASSERT_INFO(pCurrentCpu != Lock->Holder,
                "Lock initial taken by function 0x%X, now called by 0x%X\n",
                Lock->FunctionWhichTookLock,
                *((PVOID*)_AddressOfReturnAddress())
                );

    // 1.b) This line is where the lock is taken and an atomic operation is performed
    // such that the system is not affected by multiple CPUs trying to take the lock, thus 
    // it is necessary even if all the interrupts are disabled because another CPU might come an try to take the lock.
    while (LOCK_TAKEN == _InterlockedCompareExchange8(&Lock->State, LOCK_TAKEN, LOCK_FREE))
    {
        _mm_pause();
    }

    ASSERT(NULL == Lock->FunctionWhichTookLock);
    ASSERT(NULL == Lock->Holder);

    // 1.c) The Holder keeps a pointer to the current CPU which holds the lock, such that the system knows who owns it.
    Lock->Holder = pCurrentCpu;
    // 1.c) The FunctionWhichTookLock is used for debugging purposes, meaning that
    // if we have an error, we can check the function which took the lock and see where the errors happened
    Lock->FunctionWhichTookLock = *( (PVOID*) _AddressOfReturnAddress() );

    ASSERT(LOCK_TAKEN == Lock->State);
}

BOOL_SUCCESS
BOOLEAN
SpinlockTryAcquire(
    INOUT       PSPINLOCK       Lock,
    OUT         INTR_STATE*     IntrState
    )
{
    PVOID pCurrentCpu;

    BOOLEAN acquired;

    ASSERT(NULL != Lock);
    ASSERT(NULL != IntrState);

    *IntrState = CpuIntrDisable();

    pCurrentCpu = CpuGetCurrent();

    acquired = (LOCK_FREE == _InterlockedCompareExchange8(&Lock->State, LOCK_TAKEN, LOCK_FREE));
    if (!acquired)
    {
        CpuIntrSetState(*IntrState);
    }
    else
    {
        ASSERT(NULL == Lock->FunctionWhichTookLock);
        ASSERT(NULL == Lock->Holder);

        Lock->Holder = pCurrentCpu;
        Lock->FunctionWhichTookLock = *((PVOID*)_AddressOfReturnAddress());

        ASSERT(LOCK_TAKEN == Lock->State);
    }

    return acquired;
}

BOOLEAN
SpinlockIsOwner(
    IN          PSPINLOCK       Lock
    )
{
    return CpuGetCurrent() == Lock->Holder;
}

void
SpinlockRelease(
    INOUT       PSPINLOCK       Lock,
    IN          INTR_STATE      OldIntrState
    )
{
    PVOID pCurrentCpu = CpuGetCurrent();

    //2. a) This assert checks if we are trying to release a lock which is NULL, which is totally wrong.
    ASSERT(NULL != Lock);
    //2. a) Makes sure that the CPU which tries to release the lock is really the Holder(Owner) of that LOCK.
    ASSERT_INFO(pCurrentCpu == Lock->Holder,
                "LockTaken by CPU: 0x%X in function: 0x%X\nNow release by CPU: 0x%X in function: 0x%X\n",
                Lock->Holder, Lock->FunctionWhichTookLock,
                pCurrentCpu, *( (PVOID*) _AddressOfReturnAddress() ) );
    //2. a) This assert makes sure that the interrupts are disabled for the current CPU such that no other
    //      interrupt appears which tries to release the same lock.
    ASSERT(INTR_OFF == CpuIntrGetState());

    Lock->Holder = NULL;
    Lock->FunctionWhichTookLock = NULL;
    //2. b) This is the line where the lock is released and an atomic operation is performed and 
    //      it makes sure that other CPUs will not try to release the lock. An atomic operation
    //      makes sure that the code is fully executed.
    _InterlockedExchange8(&Lock->State, LOCK_FREE);

    //2. c) The OldIntrState is used in order to set the old state of the Interrupts and also 
    //      return it from the function such that the other functions know that state.
    CpuIntrSetState(OldIntrState);
}

#endif // _COMMONLIB_NO_LOCKS_
