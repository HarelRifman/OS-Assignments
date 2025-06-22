#ifndef BOUNDED_BUFFER_H
#define BOUNDED_BUFFER_H

#include <pthread.h>
#include <semaphore.h>

typedef struct {
    char **buffer;
    int size;
    int in;
    int out;
    int count;
    sem_t empty;  // counting semaphore for empty slots
    sem_t full;   // counting semaphore for full slots
    pthread_mutex_t mutex;  // binary semaphore (mutex) for critical section
} BoundedBuffer;

// Function declarations
BoundedBuffer* createBoundedBuffer(int size);
void insert(BoundedBuffer* bb, char* item);
char* removeItem(BoundedBuffer* bb);
void destroyBoundedBuffer(BoundedBuffer* bb);

#endif