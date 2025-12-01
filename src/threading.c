#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "threading.h"
#include "worldgen.h"

ChunkThreadPool chunk_pool = {0};

// Forward declaration of worker thread function
static void* chunk_worker_thread(void* arg);

// Get number of CPU cores
static int get_cpu_count(void) {
  #ifdef _SC_NPROCESSORS_ONLN
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    if (nprocs > 0) return (int)nprocs;
  #endif
  return 4; // Default fallback
}

int init_chunk_pool(int thread_count) {
  memset(&chunk_pool, 0, sizeof(ChunkThreadPool));

  // Determine thread count
  if (thread_count <= 0) {
    thread_count = get_cpu_count();
  }

  chunk_pool.thread_count = thread_count;
  chunk_pool.shutdown = 0;
  chunk_pool.next_request_id = 1;

  // Initialize mutexes and condition variables
  pthread_mutex_init(&chunk_pool.request_mutex, NULL);
  pthread_cond_init(&chunk_pool.request_cond, NULL);
  pthread_mutex_init(&chunk_pool.completed_mutex, NULL);

  // Allocate completion queue
  chunk_pool.completed_chunks = calloc(MAX_CHUNK_QUEUE, sizeof(GeneratedChunk));
  if (!chunk_pool.completed_chunks) {
    fprintf(stderr, "Failed to allocate completion queue\n");
    return 0;
  }

  // Create worker threads
  chunk_pool.threads = malloc(sizeof(pthread_t) * thread_count);
  if (!chunk_pool.threads) {
    fprintf(stderr, "Failed to allocate thread array\n");
    free(chunk_pool.completed_chunks);
    return 0;
  }

  for (int i = 0; i < thread_count; i++) {
    if (pthread_create(&chunk_pool.threads[i], NULL, chunk_worker_thread, NULL) != 0) {
      fprintf(stderr, "Failed to create worker thread %d\n", i);
      chunk_pool.shutdown = 1;
      // Clean up already created threads
      for (int j = 0; j < i; j++) {
        pthread_join(chunk_pool.threads[j], NULL);
      }
      free(chunk_pool.threads);
      free(chunk_pool.completed_chunks);
      return 0;
    }
  }

  printf("Chunk thread pool initialized with %d threads\n", thread_count);
  return 1;
}

void shutdown_chunk_pool(void) {
  chunk_pool.shutdown = 1;

  // Wake up all worker threads
  pthread_mutex_lock(&chunk_pool.request_mutex);
  pthread_cond_broadcast(&chunk_pool.request_cond);
  pthread_mutex_unlock(&chunk_pool.request_mutex);

  // Wait for all threads to finish
  for (int i = 0; i < chunk_pool.thread_count; i++) {
    pthread_join(chunk_pool.threads[i], NULL);
  }

  // Cleanup
  pthread_mutex_destroy(&chunk_pool.request_mutex);
  pthread_cond_destroy(&chunk_pool.request_cond);
  pthread_mutex_destroy(&chunk_pool.completed_mutex);

  free(chunk_pool.threads);
  free(chunk_pool.completed_chunks);

  printf("Chunk thread pool shutdown. Total chunks generated: %lu\n",
         chunk_pool.total_generated);
}

uint64_t request_chunk_generation(int cx, int cy, int cz, float priority) {
  pthread_mutex_lock(&chunk_pool.request_mutex);

  // Check if queue is full
  if (chunk_pool.request_count >= MAX_CHUNK_QUEUE) {
    pthread_mutex_unlock(&chunk_pool.request_mutex);
    return 0;
  }

  // Add request to queue
  ChunkRequest *req = &chunk_pool.request_queue[chunk_pool.request_tail];
  req->cx = cx;
  req->cy = cy;
  req->cz = cz;
  req->priority = priority;
  req->request_id = chunk_pool.next_request_id++;

  chunk_pool.request_tail = (chunk_pool.request_tail + 1) % MAX_CHUNK_QUEUE;
  chunk_pool.request_count++;

  uint64_t request_id = req->request_id;

  // Signal waiting worker threads
  pthread_cond_signal(&chunk_pool.request_cond);
  pthread_mutex_unlock(&chunk_pool.request_mutex);

  return request_id;
}

int poll_completed_chunks(GeneratedChunk *output, int max_chunks) {
  pthread_mutex_lock(&chunk_pool.completed_mutex);

  int retrieved = 0;
  while (chunk_pool.completed_count > 0 && retrieved < max_chunks) {
    memcpy(&output[retrieved],
           &chunk_pool.completed_chunks[chunk_pool.completed_head],
           sizeof(GeneratedChunk));

    chunk_pool.completed_head = (chunk_pool.completed_head + 1) % MAX_CHUNK_QUEUE;
    chunk_pool.completed_count--;
    retrieved++;
  }

  pthread_mutex_unlock(&chunk_pool.completed_mutex);
  return retrieved;
}

void get_pool_stats(int *pending, int *completed, uint64_t *total) {
  pthread_mutex_lock(&chunk_pool.request_mutex);
  *pending = chunk_pool.request_count;
  pthread_mutex_unlock(&chunk_pool.request_mutex);

  pthread_mutex_lock(&chunk_pool.completed_mutex);
  *completed = chunk_pool.completed_count;
  pthread_mutex_unlock(&chunk_pool.completed_mutex);

  *total = chunk_pool.total_generated;
}

// Worker thread function
static void* chunk_worker_thread(void* arg) {
  (void)arg; // Unused

  while (1) {
    ChunkRequest request;
    int has_request = 0;

    // Get request from queue
    pthread_mutex_lock(&chunk_pool.request_mutex);

    while (chunk_pool.request_count == 0 && !chunk_pool.shutdown) {
      pthread_cond_wait(&chunk_pool.request_cond, &chunk_pool.request_mutex);
    }

    if (chunk_pool.shutdown) {
      pthread_mutex_unlock(&chunk_pool.request_mutex);
      break;
    }

    if (chunk_pool.request_count > 0) {
      // Find highest priority request (lowest priority value)
      int best_idx = -1;
      float best_priority = 1e9f;

      for (int i = 0; i < chunk_pool.request_count; i++) {
        int idx = (chunk_pool.request_head + i) % MAX_CHUNK_QUEUE;
        if (chunk_pool.request_queue[idx].priority < best_priority) {
          best_priority = chunk_pool.request_queue[idx].priority;
          best_idx = idx;
        }
      }

      if (best_idx >= 0) {
        request = chunk_pool.request_queue[best_idx];

        // Remove from queue by shifting
        if (best_idx != chunk_pool.request_head) {
          // Shift elements
          while (best_idx != chunk_pool.request_head) {
            int prev = (best_idx - 1 + MAX_CHUNK_QUEUE) % MAX_CHUNK_QUEUE;
            chunk_pool.request_queue[best_idx] = chunk_pool.request_queue[prev];
            best_idx = prev;
          }
        }

        chunk_pool.request_head = (chunk_pool.request_head + 1) % MAX_CHUNK_QUEUE;
        chunk_pool.request_count--;
        has_request = 1;
      }
    }

    pthread_mutex_unlock(&chunk_pool.request_mutex);

    if (!has_request) continue;

    // Generate the chunk (this is the expensive part)
    GeneratedChunk result;
    result.cx = request.cx;
    result.cy = request.cy;
    result.cz = request.cz;
    result.request_id = request.request_id;

    // Call the worldgen function to build the chunk
    result.biome = buildChunkSection(request.cx, request.cy, request.cz);
    memcpy(result.data, chunk_section, 4096);
    result.ready = 1;

    // Add to completion queue
    pthread_mutex_lock(&chunk_pool.completed_mutex);

    if (chunk_pool.completed_count < MAX_CHUNK_QUEUE) {
      memcpy(&chunk_pool.completed_chunks[chunk_pool.completed_tail],
             &result,
             sizeof(GeneratedChunk));

      chunk_pool.completed_tail = (chunk_pool.completed_tail + 1) % MAX_CHUNK_QUEUE;
      chunk_pool.completed_count++;
      chunk_pool.total_generated++;
    } else {
      fprintf(stderr, "Warning: Completion queue full, dropping chunk\n");
    }

    pthread_mutex_unlock(&chunk_pool.completed_mutex);
  }

  return NULL;
}
