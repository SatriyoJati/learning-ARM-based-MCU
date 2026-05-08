#ifndef QUEUE_H
#define QUEUE_H
#include <stdint.h>

#define Q_MAX_SIZE 4
#define DATA_SIZE 512

typedef struct {
    uint8_t data[DATA_SIZE];
    uint16_t len;
} Data;

struct Q_T {
    Data Data[Q_MAX_SIZE];
    // Index of oldest data element
    unsigned int Head;
    // Index of next free space
    unsigned int Tail;
    // Number of Elements in use
    unsigned int Size;
} ;

typedef struct Q_T Q_T;

int Q_Enqueue( Q_T *q , uint8_t* d, uint16_t len );
int Q_Dequeue(Q_T *q, Data* temp);

void Q_Init(Q_T *q);
int Q_Empty(Q_T *q);
int Q_Full(Q_T *q);
int Q_Size(Q_T *q);

#endif