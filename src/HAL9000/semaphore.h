#pragma once
#include "common_lib.h"
#include "list.h"
#include "synch.h"


typedef struct _SEMAPHORE
{
    DWORD           Value;

    LOCK            SemaphoreLock;

    _Guarded_by_(SemaphoreLock)
    LIST_ENTRY      WaitingList;
    //struct _THREAD* Holder;
                  
} SEMAPHORE, * PSEMAPHORE;

void
SemaphoreInit(
    OUT     PSEMAPHORE      Semaphore,
    IN      DWORD           InitialValue
    );

void
SemaphoreDown(
    INOUT   PSEMAPHORE      Semaphore,
    IN      DWORD           Value
    );

void
SemaphoreUp(
    INOUT   PSEMAPHORE      Semaphore,
    IN      DWORD           Value
    );