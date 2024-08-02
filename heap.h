#ifndef HEAP_H_
#define HEAP_H_

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#define H_CAP 128000
#define HEAP_CAP_WORDS (H_CAP / sizeof(uintptr_t))

extern uintptr_t heap[HEAP_CAP_WORDS];

extern const uintptr_t* stack_base;

#define CHUNK_LIST_CAP 1024

typedef struct {
	uintptr_t* start;
	size_t size;
} Chunk;

typedef struct {
	size_t count;
	Chunk chunks[CHUNK_LIST_CAP];
} Chunk_List;

void* h_alloc(size_t size_bytes);

void h_free(void* ptr);

void h_collect();

extern Chunk_List halloced_chunks;

extern Chunk_List hfreed_chunks;

extern Chunk_List temp_chunks;

void cl_insert(Chunk_List* cl, void* start, size_t size);

void cl_merge(Chunk_List* dst, const Chunk_List* src);

void cl_print(const Chunk_List* cl, const char* name);

int cl_find(const Chunk_List* cl, const uintptr_t* ptr);

void cl_remove(Chunk_List* cl, size_t idx);

void swap_chunks(Chunk* chunk1, Chunk* chunk2);

#endif // HEAP_H_