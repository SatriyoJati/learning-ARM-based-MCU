#include <stdint.h>
#include "queue.h"
#include "stm32f100xb.h"
#include "stddef.h"
#include <string.h>

void Q_Init(Q_T *q)
{
    unsigned int i;
    // memset((q->Data)->data, 0, Q_MAX_SIZE);
    for (i = 0; i < Q_MAX_SIZE ; i++) {
        memset((q->Data[i]).data, 0, DATA_SIZE);
        (q->Data[i]).len = 0;
        q->Head = 0;
        q->Tail = 0;
        q->Size = 0;
    }
}

int Q_Empty(Q_T *q)
{
    return q->Size == 0;
}

int Q_Full(Q_T *q)
{
    return q->Size == Q_MAX_SIZE;
}

int Q_Size(Q_T *q)
{
    return q->Size;
}


int Q_Enqueue( Q_T *q , uint8_t* d, uint16_t len ) {
    uint32_t masking_state;

    if (!Q_Full(q)) {
        memcpy(q->Data[q->Tail].data , d, len);
        q->Data[q->Tail].len = len;

        masking_state = __get_PRIMASK();

        __disable_irq();

        q->Tail = (q->Tail+1) % Q_MAX_SIZE;
        q->Size++;

        __set_PRIMASK(masking_state);
        return 1;
    } else {
        return 0;
    }
}

int Q_Dequeue(Q_T *q, Data* temp) {
    uint32_t masking_state;

    if (!Q_Empty(q)) {
        *temp = q->Data[q->Head];
        // q->Data[q->Head] = ;

        masking_state = __get_PRIMASK();

        __disable_irq();

        q->Head = (q->Head+1) % Q_MAX_SIZE;
        q->Size--;

        __set_PRIMASK(masking_state);
        return 1;
    }

    return 0;
}
