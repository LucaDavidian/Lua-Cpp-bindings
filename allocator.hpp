#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <cstdlib>
#include <cassert>
#include <cstddef>
#include <cstring>

// Lua default allocator implementation 
inline void *LuaDefAlloc(void *ud, void *memBlock, size_t origSize, size_t newSize)
{
	(void)ud; (void)origSize; // unused

	if (newSize == 0)
	{
		free(memBlock);
		return NULL;
	}
	else
		return realloc(memBlock, newSize);
}

// arena storage pool allocator
class LuaArenaAllocator
{
private:
	alignas(alignof(std::max_align_t)) struct FreeBlock
	{
		size_t sizeBlocks;
		FreeBlock *next;
	} *freeList;

public:
	LuaArenaAllocator(void *begin, size_t size) : memStart(begin), memSize(size), currentPosition(0U), freeList(nullptr), manage(false) 
	{
		freeList = static_cast<FreeBlock*>(memStart);
		freeList->sizeBlocks = memSize / sizeof(FreeBlock);
		freeList->next = nullptr;
	}

	LuaArenaAllocator(size_t size) : LuaArenaAllocator(new unsigned char[size], size) 
	{
		manage = true;
	}

	~LuaArenaAllocator()
	{
		if (manage)
			delete[] memStart;
	}

	void *Allocate(size_t sizeBytes)
	{
		size_t numBlocksToAllocate = (sizeof(FreeBlock) + sizeBytes - 1U) / sizeof(FreeBlock) + 1U;

		FreeBlock *current = freeList, *previous = current;

		while (current) 
		{
			if (current->sizeBlocks >= numBlocksToAllocate)
			{
				if (current->sizeBlocks == numBlocksToAllocate)
					previous->next = current->next;               // remove block from free list
				else  // block is greater
				{
					current->sizeBlocks -= numBlocksToAllocate;   // shrink free block
					current += current->sizeBlocks;               // advance to allocated block (tail end of free block)
					current->sizeBlocks = numBlocksToAllocate;    // set size of allocated block
					current->next = nullptr;
				}

				return current + 1;    // return one block past start of allocated memory (skip header)
			}

			previous = current;
			current = current->next;
		}

		if (!current)  // reached end of free list
			assert((false, "out of memory"));

		//assert(currentPosition + sizeBytes < LuaArenaAllocator::memSize);

		//void *memBlockStart = (unsigned char*)memStart + currentPosition;

		//currentPosition += sizeBytes;

		//// check memory alignment (strictest/largest alignment required)
		////currentPosition += currentPosition % ALIGNMENT ? ALIGNMENT - currentPosition % ALIGNMENT : 0U;
		//currentPosition = currentPosition + ALIGNMENT - currentPosition % ALIGNMENT;

		//return memBlockStart;
	}

	void Deallocate(void *memBlock, size_t oldSize)
	{


	}

	void *Reallocate(void *memBlock, size_t oldSize, size_t newSize)
	{
		//// allocate new memory and copy old memory
		void *memBlockStart = Allocate(newSize);
		memcpy(memBlockStart, memBlock, oldSize);

		// deallocate old memory
		Deallocate(memBlock, oldSize);

		return memBlockStart;
	}

	static void *LuaAlloc(void *ud, void *ptr, size_t osize, size_t nsize)
	{
		auto *allocator = static_cast<LuaArenaAllocator*>(ud);

		if (nsize == 0)
		{
			allocator->Deallocate(ptr, osize);
			return nullptr;
		}
		else
		{
			if (!ptr)
				return allocator->Allocate(nsize);
			else
				return allocator->Reallocate(ptr, osize, nsize);
		}
	}

private:
	bool manage;
	void *memStart;
	size_t memSize;
	size_t currentPosition;
	constexpr static size_t ALIGNMENT = alignof(std::max_align_t);
	constexpr static size_t ALLOC_BLOCK_SIZE = ALIGNMENT * 8U;   
};

#endif // ALLOCATOR_H
