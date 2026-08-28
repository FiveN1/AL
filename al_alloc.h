#ifndef AL_ALLOC_INCLUDED
#define AL_ALLOC_INCLUDED

/*
	al_alloc - AL allocators
	
	QUICK JUMP:
	>>arena_def
	>>bit_pool_def
	>>arena_impl
	>>bit_pool_impl

*/

#define AL_MEM_ALIGN (sizeof(void*)) // on 32bit machines = 4bytes, on 64bit machines = 8bytes;
#define AL_MEM_ALIGN_UP_POW2(n, p) (((uint64_t)(n) + (uint64_t)((p) - 1)) & (~((uint64_t)(p) - 1))) // align podle (p)
#define AL_KiB 1024
#define AL_MiB 1048576
#define AL_GiB 1073741824

#ifdef __GNUC__ 
#define al_tzcnt32(n) __builtin_ctz(n) // POZOR: možná není u32...???
#elif defined(_MSC_VER)
#define al_tzcnt32(n) _tzcnt_u32(n)
#define al_tzcnt64(n) _tzcnt_u64(n)
#else
unsigned int al_tzcnt32(unsigned int v);
unsigned long long al_tzcnt64(unsigned long long v);
#endif

enum {
	AL_MAX_BIT_POOL_LAYERS = 4
};

/*
	al_arena
	arena allocator. linearly allocates blocks of memory.
	memory is always aligned to AL_MEM_ALIGN field.

	>>arena_def
*/
typedef struct al_arena {
	uint64_t size;		 // arena capacity
	uint64_t free_index; // offset to free space in arena
	char _tag[16];		 // debug tag for debugging in memory. by default tagged as: "al_arena"
} al_arena;

al_arena* al_arena_create(uint64_t size);
void al_arena_delete(al_arena* arena);
void* al_arena_push(al_arena* arena, uint64_t size);
void al_arena_reset(al_arena* arena);
uint64_t al_arena_get_max_size(al_arena* arena);
uint64_t al_arena_get_allocated_size(al_arena* arena);

/*
	al_bit_pool

	64 bit pool allocator.

	O(1) allocation/deallocation with basically no fragmentation.
	free slots are found using the 'tzcnt64' instruction, thus always allocating the lowest free slots.
	for a 2 layer bit pool the instruction is called 2 times, effectivelly searching 4096 slots.

	note that the null'th index is reserved for invalid values and will allways allocated.

	1 bit memory overhead. for example a 3 layer bit pool (262'144 capacity) wil, have an overhead of: 1 + 1/64 + 1/4096 bits;
	each slot will be marked by one bit as allocated (1) or free (0).

	essentially all x64 processors support the 'tzcnt64' instruction. 
	on older processors the instruction will be translated to the BSF instruction.
	this library can implement this instruction as a bit-hack, which will count the trailing zeros in parallel.

	bti pool capacity grows exponentially with the number of layers.
	- 64^n; where n represents the number of layers.
	- for 1 layer: 64^1 = 64 capacity;
	- for 2 layers: 64^2 = 4096 capacity;
	- for 3 layers: 64^3 = 262'144 capacity;
	- for 4 layers: 64^4 = 16'777'216 capacity;
	max layer count is defined by AL_MAX_BIT_POOL_LAYERS. (so that someone doesnt accidentally acllocate 64TiB of data)

	reference:
	- https://jakubtomsu.github.io/posts/bit_pools/

	>>bit_pool_def
*/
typedef struct al_bit_pool {
	size_t _element_size; // size of one data element in pool
	uint8_t _layer_count; // number of layers in the pool
	size_t _l0_offset;	  // pre-calculated offset to the lowest layer (data layer)
	char _tag[16];		  // debug tag for debugging in memory. by default tagged as: "al_bit_pool"
} al_bit_pool;

al_bit_pool* al_bit_pool_create(uint64_t element_size, uint8_t layer_count);
void al_bit_pool_delete(al_bit_pool* bit_pool);
uint64_t al_bit_pool_get_capacity(al_bit_pool* bit_pool);
uint64_t al_bit_pool_add(al_bit_pool* bit_pool);
void al_bit_pool_remove(al_bit_pool* bit_pool, uint64_t index);
void* al_bit_pool_get(al_bit_pool* bit_pool, uint64_t index);
bool al_bit_pool_check_if_allocated(al_bit_pool* bit_pool, uint64_t index);
uint64_t* _al_bit_pool_get_top_layer_ptr(al_bit_pool* bit_pool);

#endif AL_ALLOC_INCLUDED
#ifdef AL_IMPL 

//
// INTRINSICS
//

#if !defined(al_tzcnt32)
unsigned int al_tzcnt32(unsigned int v) {
	// https://graphics.stanford.edu/%7Eseander/bithacks.html#ZerosOnRightParallel
	// (11.07.2026) lze dosáhnout bez branching!
	unsigned int c = 32;
	v &= -(signed int)v;
	if (v) c--;
	if (v & 0x0000FFFF) c -= 16;
	if (v & 0x00FF00FF) c -= 8;
	if (v & 0x0F0F0F0F) c -= 4;
	if (v & 0x33333333) c -= 2;
	if (v & 0x55555555) c -= 1;
	return c;
}
#endif
#if !defined(al_tzcnt64)
unsigned long long al_tzcnt64(unsigned long long v) {
	// https://graphics.stanford.edu/%7Eseander/bithacks.html#ZerosOnRightParallel (updated for 64 bit)
	// (11.07.2026) lze dosáhnout bez branching!
	unsigned long long c = 64;
	v &= -(signed long long)v;
	if (v) c--;
	if (v & 0x00000000FFFFFFFF) c -= 32;
	if (v & 0x0000FFFF0000FFFF) c -= 16;
	if (v & 0x00FF00FF00FF00FF) c -= 8;
	if (v & 0x0F0F0F0F0F0F0F0F) c -= 4;
	if (v & 0x3333333333333333) c -= 2;
	if (v & 0x5555555555555555) c -= 1;
	return c;
}
#endif

//
// ARENA
// >>arena_impl

al_arena* al_arena_create(uint64_t size) {
	al_arena* arena = (al_arena*)malloc(size + sizeof(al_arena));
	if (arena == NULL) return NULL;
	memset(arena, 0, size + sizeof(al_arena));
	arena->size = size;
	arena->free_index = sizeof(al_arena);
	snprintf(arena->_tag, 16, "al_arena");
	return arena;
}

void al_arena_delete(al_arena* arena) {
	free(arena);
}

void* al_arena_push(al_arena* arena, uint64_t size) {
	uint64_t free_index_aligned = AL_MEM_ALIGN_UP_POW2(arena->free_index, AL_MEM_ALIGN); // align memory location
	uint64_t new_free_index = free_index_aligned + size; // can sometimes not fit due to alignment!
	if (new_free_index > arena->size + sizeof(al_arena)) {
		printf("ERROR: al_arena overflow at: %p\n", arena); 
		return (void*)&arena[1];
	}
	arena->free_index = new_free_index;
	return (void*)((uint8_t*)arena + free_index_aligned);
}

void al_arena_reset(al_arena* arena) {
	arena->free_index = sizeof(al_arena);
}

uint64_t al_arena_get_max_size(al_arena* arena) {
	return arena->size;
}

uint64_t al_arena_get_allocated_size(al_arena* arena) {
	return arena->free_index;
}

//
// BIT POOL
// >>bit_pool_impl

uint64_t* _al_bit_pool_get_top_layer_ptr(al_bit_pool* bit_pool) {
	return (uint64_t*)((uint8_t*)bit_pool + sizeof(al_bit_pool) + bit_pool->_element_size * al_bit_pool_get_capacity(bit_pool));
}

al_bit_pool* al_bit_pool_create(uint64_t element_size, uint8_t layer_count) { // ps: this might be considered as whitney style
	if (layer_count == 0) layer_count == 1;
	if (layer_count > AL_MAX_BIT_POOL_LAYERS) layer_count = AL_MAX_BIT_POOL_LAYERS;
	uint64_t real_element_count = 1ULL << (layer_count * 6); // bit pool capacity is in the powers of 64
	uint64_t layer_sum = ((1ULL << (layer_count * 6)) - 1) / (64 - 1); // sum formula for the geometric progression
	uint64_t aligned_element_size = AL_MEM_ALIGN_UP_POW2(element_size, AL_MEM_ALIGN); // get aligned element size, for optimized packing
	uint64_t alloc_size = sizeof(al_bit_pool) + aligned_element_size * real_element_count + sizeof(uint64_t) * layer_sum;
	al_bit_pool* bit_pool = (al_bit_pool*)malloc(alloc_size);
	if (bit_pool == NULL) return NULL;
	memset(bit_pool, 0, alloc_size);
	bit_pool->_layer_count = layer_count;
	bit_pool->_element_size = aligned_element_size;
	bit_pool->_l0_offset = ((1ULL << ((layer_count - 1) * 6)) - 1) / (64 - 1);
	snprintf(bit_pool->_tag, 16, "al_bit_pool");
	(_al_bit_pool_get_top_layer_ptr(bit_pool) + bit_pool->_l0_offset)[0] |= 1ULL; // set lowest bit as allocated, since this slot is used for invalid values
	return bit_pool;
}

void al_bit_pool_delete(al_bit_pool* bit_pool) {
	free(bit_pool);
}

uint64_t al_bit_pool_add(al_bit_pool* bit_pool) {
	uint64_t* layer = _al_bit_pool_get_top_layer_ptr(bit_pool); // pointer to the highest layer
	if (*layer == ~0ULL) {
		printf("ERROR: al_bit_pool overflow at: %p\n", bit_pool);
		return 0; // if bit pool is full, return invalid slot index
	}
	uint64_t slot_index = 0; // index of a slot in layer
	for (uint8_t i = 0; i < bit_pool->_layer_count - 1; i++) { // descent layers
		slot_index = al_tzcnt64(~layer[slot_index]) + slot_index * 64; // get index of slot in lower layer
		layer += 1ULL << (i * 6); // geometric progression in layer memory (each layer has 64^n elements, where n is the layer depth)
	}
	uint64_t bit_index = al_tzcnt64(~layer[slot_index]); // get bit index in lowest layer slot
	layer[slot_index] |= (1ULL << bit_index); // set bit as "taken" = 1
	uint64_t index = bit_index + slot_index * 64; // get actual index of element
	for (uint8_t i = bit_pool->_layer_count - 1; i > 0; i--) { // back propagate the bit update
		if (layer[slot_index] != ~0ULL) break; // only continue updating if the slot is full
		layer -= 1ULL << ((i - 1) * 6); // go up a layer
		bit_index = 0x3F & slot_index; // get bit index in the slot. (the first 6 bits) (equivalent to modulus of 64)
		slot_index = slot_index >> 6; // get slot index in upper layer (equivalent to division by 64)
		layer[slot_index] |= (1ULL << bit_index); // set as "taken"
	}
	memset(al_bit_pool_get(bit_pool, index), 0, bit_pool->_element_size); // return cleared memory by default
	return index;
}

void al_bit_pool_remove(al_bit_pool* bit_pool, uint64_t index) {
	if (index == 0) { // cant deallocate invalid slot
		printf("WARN: al_bit_pool cant deallocate invalid id! %p\n", bit_pool);
		return;
	}
	uint64_t* layer = _al_bit_pool_get_top_layer_ptr(bit_pool); // pointer to the highest layer
	layer += bit_pool->_l0_offset; // offset to the lowest layer
	uint64_t bit_index = 0;
	uint64_t slot_index = index;
	for (uint8_t i = bit_pool->_layer_count - 1;; i--) { // back propagate the bit update (every slot above is available)
		bit_index = 0x3F & slot_index; // get bit index in the slot. (the first 6 bits) (equivalent to modulus of 64)
		slot_index = slot_index >> 6; // get slot index in upper layer (equivalent to division by 64)
		if (((layer[slot_index] >> bit_index) & 0x01) == 0) break; // check if bit was already set to "available", if yes then there is no need to keep updating the hiearchy
		layer[slot_index] ^= (1ULL << bit_index); // set bit as "available" = 0
		if (i == 0) break; // before moving on to next layer check for i
		layer -= 1ULL << ((i - 1) * 6); // go up a layer
	}
}

void* al_bit_pool_get(al_bit_pool* bit_pool, uint64_t index) {
	return (void*)((uint8_t*)bit_pool + sizeof(al_bit_pool) + bit_pool->_element_size * index);
}

uint64_t al_bit_pool_get_capacity(al_bit_pool* bit_pool) {
	return 1ULL << (bit_pool->_layer_count * 6);
}

bool al_bit_pool_check_if_allocated(al_bit_pool* bit_pool, uint64_t index) {
	uint64_t* layer = _al_bit_pool_get_top_layer_ptr(bit_pool);
	layer += bit_pool->_l0_offset;
	uint64_t bit_index = 0x3F & index;
	uint64_t slot_index = index >> 6;
	if (((layer[slot_index] >> bit_index) & 0x01) == 1) {
		return true;
	}
	return false;
}

#endif AL_IMPL