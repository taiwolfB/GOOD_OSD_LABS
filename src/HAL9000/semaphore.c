#include "HAL9000.h"
#include "semaphore.h"
#include "thread_internal.h"



void
SemaphoreInit(
    OUT     PSEMAPHORE      Semaphore,
    IN      DWORD           InitialValue
    )
{
    ASSERT(NULL != Semaphore);

    memzero(Semaphore, sizeof(SEMAPHORE));

    LockInit(&Semaphore->SemaphoreLock);

    InitializeListHead(&Semaphore->WaitingList);

    Semaphore->Value = InitialValue;
}




// It is better to use the blocking technique rather than using the busy waiting
// such that we do not keep the processor busy for no reason by using the busy waiting technique.
void
SemaphoreUp(
    INOUT   PSEMAPHORE      Semaphore,
    IN      DWORD           Value
)
{
    ASSERT(NULL != Semaphore);
    ASSERT(Value != 0);

    INTR_STATE dummyState;

    LockAcquire(&Semaphore->SemaphoreLock, &dummyState);
    DWORD i = 0;
    while (!IsListEmpty(&Semaphore->WaitingList) && i < Value)
    {
        ++i;
        PLIST_ENTRY currentUnblockedThreadPlistEntry = RemoveHeadList(&Semaphore->WaitingList);
        PTHREAD currentUnblockedThread = CONTAINING_RECORD(currentUnblockedThreadPlistEntry, THREAD, ReadyList);
        ThreadUnblock(currentUnblockedThread);
    }

    Semaphore->Value += Value;
    LockRelease(&Semaphore->SemaphoreLock, dummyState);
}


/*
* Busy waiting technique for semaphores
void
SemaphoreDown(
    INOUT    PSEMAPHORE    Semaphore,
    IN        DWORD        Value
    ) 
{
    ASSERT(Value != 0);
    ASSERT(Semaphore != NULL);

    INTR_STATE dummyState;33333

    LockAcquire(&Semaphore->SemaphoreLock, &dummyState);

    if (Semaphore->Value < Value) {
        LockRelease(&Semaphore->SemaphoreLock, dummyState);
        _mm_pause();
        LockAcquire(&Semaphore->SemaphoreLock, &dummyState);
    }

    Semaphore->Value -= Value;

    LockRelease(&Semaphore->SemaphoreLock, dummyState);
}

*/

void
SemaphoreDown(
    INOUT   PSEMAPHORE      Semaphore,
    IN      DWORD           Value
    ) 
{
    INTR_STATE dummyState;
    INTR_STATE oldState;

    ASSERT(Value != 0);
    ASSERT(Semaphore != NULL);

    oldState = CpuIntrDisable();

    LockAcquire(&Semaphore->SemaphoreLock, &dummyState);

    if (Semaphore->Value < Value)
    {
        InsertTailList(&Semaphore->WaitingList, &GetCurrentThread()->ReadyList);
        ThreadTakeBlockLock();
        LockRelease(&Semaphore->SemaphoreLock, dummyState);
        ThreadBlock();
        LockAcquire(&Semaphore->SemaphoreLock, &dummyState);
    }

    Semaphore->Value -= Value;

    LockRelease(&Semaphore->SemaphoreLock, dummyState);

    CpuIntrSetState(oldState);
}

