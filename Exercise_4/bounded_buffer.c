#define _GNU_SOURCE
#include "bounded_buffer.h"
#include <stdlib.h>
#include <string.h>

BoundedBuffer* createBoundedBuffer(int size) {
    BoundedBuffer* bb = (BoundedBuffer*)malloc(sizeof(BoundedBuffer));
    bb->buffer = (char**)malloc(size * sizeof(char*));
    bb->size = size;
    bb->in = 0;
    bb->out = 0;
    bb->count = 0;
    
    // Initialize semaphores
    sem_init(&bb->empty, 0, size);  // Initially all slots are empty
    sem_init(&bb->full, 0, 0);      // Initially no slots are full
    pthread_mutex_init(&bb->mutex, NULL);
    
    return bb;
}

void insert(BoundedBuffer* bb, char* item) {
    // Wait for empty slot
    sem_wait(&bb->empty);
    
    // Enter critical section
    pthread_mutex_lock(&bb->mutex);
    
    // Insert item
    bb->buffer[bb->in] = strdup(item);
    bb->in = (bb->in + 1) % bb->size;
    bb->count++;
    
    // Exit critical section
    pthread_mutex_unlock(&bb->mutex);
    
    // Signal that buffer has one more item
    sem_post(&bb->full);
}

char* removeItem(BoundedBuffer* bb) {
    // Wait for full slot
    sem_wait(&bb->full);
    
    // Enter critical section
    pthread_mutex_lock(&bb->mutex);
    
    // Remove item
    char* item = bb->buffer[bb->out];
    bb->out = (bb->out + 1) % bb->size;
    bb->count--;
    
    // Exit critical section
    pthread_mutex_unlock(&bb->mutex);
    
    // Signal that buffer has one more empty slot
    sem_post(&bb->empty);
    
    return item;
}

void destroyBoundedBuffer(BoundedBuffer* bb) {
    if (bb) {
        // Clean up any remaining items
        for (int i = 0; i < bb->count; i++) {
            int index = (bb->out + i) % bb->size;
            if (bb->buffer[index]) {
                free(bb->buffer[index]);
            }
        }
        
        free(bb->buffer);
        sem_destroy(&bb->empty);
        sem_destroy(&bb->full);
        pthread_mutex_destroy(&bb->mutex);
        free(bb);
    }
}