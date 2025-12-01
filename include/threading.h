#ifndef H_THREADING
#define H_THREADING

#include <stdint.h>
#include <pthread.h>

#define MAX_CHUNK_QUEUE 256

// Chunk request for generation
typedef struct {
  int cx, cy, cz;           // Chunk coordinates (section coords, not block)
  float priority;           // Distance from player (lower = higher priority)
  uint64_t request_id;      // Unique ID for tracking
} ChunkRequest;

// Generated chunk data
typedef struct {
  int cx, cy, cz;
  uint8_t biome;
  uint8_t data[4096];       // 16x16x16 blocks
  uint64_t request_id;
  int ready;                // 1 when complete
} GeneratedChunk;

// Thread pool for chunk generation
typedef struct {
  pthread_t *threads;
  int thread_count;
  int shutdown;

  // Request queue (producer = main thread, consumer = workers)
  ChunkRequest request_queue[MAX_CHUNK_QUEUE];
  int request_head;
  int request_tail;
  int request_count;
  pthread_mutex_t request_mutex;
  pthread_cond_t request_cond;

  // Completion queue (producer = workers, consumer = main thread)
  GeneratedChunk *completed_chunks;
  int completed_head;
  int completed_tail;
  int completed_count;
  pthread_mutex_t completed_mutex;

  // Stats
  uint64_t total_generated;
  uint64_t next_request_id;
} ChunkThreadPool;

// Global thread pool instance
extern ChunkThreadPool chunk_pool;

// Initialize thread pool (thread_count = -1 for auto-detect CPU cores)
int init_chunk_pool(int thread_count);

// Shutdown and cleanup thread pool
void shutdown_chunk_pool(void);

// Submit chunk generation request (non-blocking)
// Returns request_id, or 0 if queue is full
uint64_t request_chunk_generation(int cx, int cy, int cz, float priority);

// Poll for completed chunks (non-blocking)
// Returns number of chunks retrieved, populates output array
// max_chunks = size of output array
int poll_completed_chunks(GeneratedChunk *output, int max_chunks);

// Get stats
void get_pool_stats(int *pending, int *completed, uint64_t *total);

#endif
