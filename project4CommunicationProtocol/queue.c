#define Q_MAX_SIZE (256)
#include <stdint.h>
#include "queue.h"

typedef struct Q_T {
    uint8_t Data[Q_MAX_SIZE];
    // Index of oldest data element
    unsigned int Head;
    // Index of next free space
    unsigned int Tail;
    // Number of Elements in use
    unsigned int Size;
} ;

void Q_Init(Q_T *q)
{
    unsigned int i;
    for (i = 0; i < Q_MAX_SIZE ; i++) {
        q->Data[i] = 0;
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


int Q_Enqueue( Q_T *q , uint8_t d ) {
    uint32_t masking_state;

    if (!Q_Full(q)) {
        q->Data[q->Tail] = d;

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

uint8_t Q_Dequeue(Q_T *q) {
    uint32_t masking_state;
    uint8_t t = 0;

    if (!Q_Empty(q)) {
        t = q->Data[q->Head];
        q->Data[q->Head] = '_';

        masking_state = __get_PRIMASK();

        __disable_irq();

        q->Head = (q->Head+1) % Q_MAX_SIZE;
        q->Size--;

        __set_PRIMASK(masking_state);
    }

    return t;
}
