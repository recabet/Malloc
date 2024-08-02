#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "heap.h"


uintptr_t heap[HEAP_CAP_WORDS] = {0};

const uintptr_t* stack_base = NULL;

bool reachable_chunks[CHUNK_LIST_CAP] = {0};

void* to_free[CHUNK_LIST_CAP] = {NULL};

size_t to_free_count = 0;

Chunk_List halloced_chunks = {0};

Chunk_List hfreed_chunks = {.count = 1, .chunks = {[0] = {.start = heap, .size = sizeof(heap)}},};

Chunk_List temp_chunks = {0};

void swap_chunks(Chunk* chunk1, Chunk* chunk2)
{
	const Chunk temp = *chunk1;
	*chunk1 = *chunk2;
	*chunk2 = temp;
}

void cl_insert(Chunk_List* cl, void* start, size_t size)
{
	if(cl->count >= CHUNK_LIST_CAP)
	{
		perror("Chunk list capacity exceeded");
		exit(EXIT_FAILURE);
	}

	cl->chunks[cl->count].start = start;
	cl->chunks[cl->count].size = size;

	for(size_t i = cl->count; i > 0 && cl->chunks[i].start < cl->chunks[i - 1].start; --i)
	{
		swap_chunks(&cl->chunks[i], &cl->chunks[i - 1]);
	}

	cl->count += 1;
}

void cl_merge(Chunk_List* dst, const Chunk_List* src)
{
	dst->count = 0;

	for(size_t i = 0; i < src->count; ++i)
	{
		const Chunk chunk = src->chunks[i];

		if(dst->count > 0)
		{
			Chunk* top_chunk = &dst->chunks[dst->count - 1];

			if(top_chunk->start + top_chunk->size == chunk.start)
			{
				top_chunk->size += chunk.size;
			}
			else
			{
				cl_insert(dst, chunk.start, chunk.size);
			}
		}
		else
		{
			cl_insert(dst, chunk.start, chunk.size);
		}
	}
}

void cl_print(const Chunk_List* cl, const char* name)
{
	printf("%s Chunks (%zu):\n", name, cl->count);
	for(size_t i = 0; i < cl->count; ++i)
	{
		printf("  start: %p, size: %zu\n", (void*) cl->chunks[i].start, cl->chunks[i].size);
	}
}

int cl_find(const Chunk_List* cl, const uintptr_t* ptr)
{
	for(size_t i = 0; i < cl->count; ++i)
	{
		if(cl->chunks[i].start == ptr)
		{
			return (int) i;
		}
	}

	return -1;
}

void cl_remove(Chunk_List* cl, size_t idx)
{
	if(idx >= cl->count)
	{
		perror("Invalid chunk list index");
		exit(EXIT_FAILURE);
	}

	for(size_t i = idx; i < cl->count - 1; ++i)
	{
		cl->chunks[i] = cl->chunks[i + 1];
	}
	cl->count -= 1;
}

void* h_alloc(size_t size)
{
	const size_t size_words = (size + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);

	if(size_words > 0)
	{
		cl_merge(&temp_chunks, &hfreed_chunks);
		hfreed_chunks = temp_chunks;

		for(size_t i = 0; i < hfreed_chunks.count; ++i)
		{
			const Chunk chunk = hfreed_chunks.chunks[i];
			if(chunk.size >= size_words)
			{
				cl_remove(&hfreed_chunks, i);

				const size_t tail_size_words = chunk.size - size_words;
				cl_insert(&halloced_chunks, chunk.start, size_words);

				if(tail_size_words > 0)
				{
					cl_insert(&hfreed_chunks, chunk.start + size_words, tail_size_words);
				}

				return chunk.start;
			}
		}
	}

	perror("Allocation failed: not enough memory");
	return NULL;
}

void h_free(void* ptr)
{
	if(ptr)
	{
		const int idx = cl_find(&halloced_chunks, ptr);
		if(idx < 0)
		{
			perror("Pointer not found in allocated chunks");
			exit(EXIT_FAILURE);
		}

		assert(ptr == halloced_chunks.chunks[idx].start);
		cl_insert(&hfreed_chunks, halloced_chunks.chunks[idx].start, halloced_chunks.chunks[idx].size);
		cl_remove(&halloced_chunks, (size_t) idx);
	}
}

static void m_region(const uintptr_t* start, const uintptr_t* end)
{
	for(; start < end; start += 1)
	{
		const uintptr_t* p = (const uintptr_t*) *start;
		for(size_t i = 0; i < halloced_chunks.count; ++i)
		{
			Chunk chunk = halloced_chunks.chunks[i];
			if(chunk.start <= p && p < chunk.start + chunk.size)
			{
				if(!reachable_chunks[i])
				{
					reachable_chunks[i] = true;
					m_region(chunk.start, chunk.start + chunk.size);
				}
			}
		}
	}
}

void h_collect()
{
	const uintptr_t* stack_start = (const uintptr_t*) __builtin_frame_address(0);
	memset(reachable_chunks, 0, sizeof(reachable_chunks));
	m_region(stack_start, stack_base + 1);

	to_free_count = 0;
	for(size_t i = 0; i < halloced_chunks.count; ++i)
	{
		if(!reachable_chunks[i])
		{
			if(to_free_count >= CHUNK_LIST_CAP)
			{
				perror("Too many chunks to free");
				exit(EXIT_FAILURE);
			}
			to_free[to_free_count++] = halloced_chunks.chunks[i].start;
		}
	}

	for(size_t i = 0; i < to_free_count; ++i)
	{
		h_free(to_free[i]);
	}
}
