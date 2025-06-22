#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "bounded_buffer.h"

#define MAX_PRODUCERS 10
#define MAX_STRING_SIZE 100

// Message types
typedef enum {
    SPORTS = 0,
    NEWS = 1,
    WEATHER = 2
} MessageType;

const char* typeNames[] = {"SPORTS", "NEWS", "WEATHER"};

// Global variables
int numProducers;
int producerCounts[MAX_PRODUCERS];
BoundedBuffer* producerQueues[MAX_PRODUCERS];
BoundedBuffer* dispatcherQueues[3]; // S, N, W queues
BoundedBuffer* coEditorQueue;
int doneCount = 0;
pthread_mutex_t doneCountMutex = PTHREAD_MUTEX_INITIALIZER;

// Producer data structure
typedef struct {
    int id;
    int numProducts;
    BoundedBuffer* queue;
} ProducerData;

// Co-Editor data structure
typedef struct {
    MessageType type;
    BoundedBuffer* inputQueue;
    BoundedBuffer* outputQueue;
} CoEditorData;

// Producer thread function
void* producer(void* arg) {
    ProducerData* data = (ProducerData*)arg;
    int typeCounts[3] = {0, 0, 0}; // SPORTS, NEWS, WEATHER counters
    
    srand(time(NULL) + data->id); // Seed random number generator
    
    for (int i = 0; i < data->numProducts; i++) {
        MessageType type = rand() % 3;
        char message[MAX_STRING_SIZE];
        snprintf(message, MAX_STRING_SIZE, "Producer %d %s %d", 
                data->id, typeNames[type], typeCounts[type]);
        typeCounts[type]++;
        
        insert(data->queue, message);
    }
    
    // Send DONE message
    insert(data->queue, "DONE");
    
    return NULL;
}

// Dispatcher thread function
void* dispatcher(void* arg) {
    (void)arg; // Suppress unused parameter warning
    int round = 0;
    
    while (1) {
        int foundMessage = 0;
        
        // Round-robin through producer queues
        for (int i = 0; i < numProducers; i++) {
            int producerIndex = (round + i) % numProducers;
            
            // Try to get message without blocking
            char* message = NULL;
            
            // Check if queue has items (non-blocking check)
            if (sem_trywait(&producerQueues[producerIndex]->full) == 0) {
                pthread_mutex_lock(&producerQueues[producerIndex]->mutex);
                
                if (producerQueues[producerIndex]->count > 0) {
                    message = producerQueues[producerIndex]->buffer[producerQueues[producerIndex]->out];
                    producerQueues[producerIndex]->buffer[producerQueues[producerIndex]->out] = NULL;
                    producerQueues[producerIndex]->out = (producerQueues[producerIndex]->out + 1) % producerQueues[producerIndex]->size;
                    producerQueues[producerIndex]->count--;
                }
                
                pthread_mutex_unlock(&producerQueues[producerIndex]->mutex);
                sem_post(&producerQueues[producerIndex]->empty);
                
                if (message) {
                    foundMessage = 1;
                    
                    if (strcmp(message, "DONE") == 0) {
                        pthread_mutex_lock(&doneCountMutex);
                        doneCount++;
                        pthread_mutex_unlock(&doneCountMutex);
                        free(message);
                    } else {
                        // Parse message and route to appropriate queue
                        if (strstr(message, "SPORTS")) {
                            insert(dispatcherQueues[SPORTS], message);
                        } else if (strstr(message, "NEWS")) {
                            insert(dispatcherQueues[NEWS], message);
                        } else if (strstr(message, "WEATHER")) {
                            insert(dispatcherQueues[WEATHER], message);
                        }
                        free(message);
                    }
                }
            }
        }
        
        round++;
        
        // Check if all producers are done
        pthread_mutex_lock(&doneCountMutex);
        if (doneCount >= numProducers) {
            pthread_mutex_unlock(&doneCountMutex);
            break;
        }
        pthread_mutex_unlock(&doneCountMutex);
        
        // Small delay to prevent busy waiting
        if (!foundMessage) {
            usleep(1000); // 1ms
        }
    }
    
    // Send DONE to all dispatcher queues
    for (int i = 0; i < 3; i++) {
        insert(dispatcherQueues[i], "DONE");
    }
    
    return NULL;
}

// Co-Editor thread function
void* coEditor(void* arg) {
    CoEditorData* data = (CoEditorData*)arg;
    
    while (1) {
        char* message = removeItem(data->inputQueue);
        
        if (strcmp(message, "DONE") == 0) {
            insert(data->outputQueue, message);
            break;
        }
        
        // Simulate editing process (0.1 second delay)
        usleep(100000); // 100ms = 0.1 second
        
        insert(data->outputQueue, message);
    }
    
    return NULL;
}

// Screen Manager thread function
void* screenManager(void* arg) {
    (void)arg; // Suppress unused parameter warning
    int doneReceived = 0;
    
    while (doneReceived < 3) {
        char* message = removeItem(coEditorQueue);
        
        if (strcmp(message, "DONE") == 0) {
            doneReceived++;
        } else {
            printf("%s\n", message);
        }
        
        free(message);
    }
    
    printf("DONE\n");
    return NULL;
}

// Function to parse configuration file
int parseConfig(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open configuration file %s\n", filename);
        return -1;
    }
    
    char line[256];
    int producerIndex = 0;
    int coEditorQueueSize = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "PRODUCER")) {
            sscanf(line, "PRODUCER %d", &producerIndex);
            producerIndex--; // Convert to 0-based indexing
            
            // Read number of products
            fgets(line, sizeof(line), file);
            producerCounts[producerIndex] = atoi(line);
            
            // Read queue size
            fgets(line, sizeof(line), file);
            int queueSize;
            sscanf(line, "queue size = %d", &queueSize);
            
            // Create producer queue
            producerQueues[producerIndex] = createBoundedBuffer(queueSize);
            
            numProducers = producerIndex + 1;
        } else if (strstr(line, "Co-Editor queue size")) {
            sscanf(line, "Co-Editor queue size = %d", &coEditorQueueSize);
            coEditorQueue = createBoundedBuffer(coEditorQueueSize);
        }
    }
    
    fclose(file);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <config_file>\n", argv[0]);
        return 1;
    }
    
    // Parse configuration file
    if (parseConfig(argv[1]) != 0) {
        return 1;
    }
    
    // Create dispatcher queues (fixed size for simplicity)
    for (int i = 0; i < 3; i++) {
        dispatcherQueues[i] = createBoundedBuffer(100);
    }
    
    // Create threads
    pthread_t producerThreads[MAX_PRODUCERS];
    pthread_t dispatcherThread;
    pthread_t coEditorThreads[3];
    pthread_t screenManagerThread;
    
    // Create producer data and threads
    ProducerData producerData[MAX_PRODUCERS];
    for (int i = 0; i < numProducers; i++) {
        producerData[i].id = i + 1;
        producerData[i].numProducts = producerCounts[i];
        producerData[i].queue = producerQueues[i];
        pthread_create(&producerThreads[i], NULL, producer, &producerData[i]);
    }
    
    // Create dispatcher thread
    pthread_create(&dispatcherThread, NULL, dispatcher, NULL);
    
    // Create co-editor data and threads
    CoEditorData coEditorData[3];
    for (int i = 0; i < 3; i++) {
        coEditorData[i].type = i;
        coEditorData[i].inputQueue = dispatcherQueues[i];
        coEditorData[i].outputQueue = coEditorQueue;
        pthread_create(&coEditorThreads[i], NULL, coEditor, &coEditorData[i]);
    }
    
    // Create screen manager thread
    pthread_create(&screenManagerThread, NULL, screenManager, NULL);
    
    // Wait for all threads to complete
    for (int i = 0; i < numProducers; i++) {
        pthread_join(producerThreads[i], NULL);
    }
    
    pthread_join(dispatcherThread, NULL);
    
    for (int i = 0; i < 3; i++) {
        pthread_join(coEditorThreads[i], NULL);
    }
    
    pthread_join(screenManagerThread, NULL);
    
    // Clean up
    for (int i = 0; i < numProducers; i++) {
        destroyBoundedBuffer(producerQueues[i]);
    }
    
    for (int i = 0; i < 3; i++) {
        destroyBoundedBuffer(dispatcherQueues[i]);
    }
    
    destroyBoundedBuffer(coEditorQueue);
    pthread_mutex_destroy(&doneCountMutex);
    
    return 0;
}