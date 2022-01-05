#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "armemu.h"
    
void cache_init(struct cache_st *csp) {

    if (csp->type == CT_DIRECT_MAPPED) {
        csp->ways = 1;
        csp->index_mask = (csp->size) - 1;
    } else if (csp->type == CT_SET_ASSOCIATIVE) {
        csp->ways = 4;
        csp->index_mask = (csp->size / csp->ways) - 1;
    }

    csp->index_bits = 0;
    while (csp->index_mask & (1 << csp->index_bits)) {
              csp->index_bits++;
    }

    for (int i = 0; i < CACHE_MAX_SLOTS; i++) {
        csp->slots[i].valid = 0;
        csp->slots[i].tag = 0;
        csp->slots[i].data = 0;
        // timestamp only used for SA cache
        csp->slots[i].timestamp = 0;
    }

    csp->refs = 0;
    csp->hits = 0;
    csp->misses = 0;
    csp->misses_cold = 0;
    csp->misses_hot = 0;
}

void cache_print(struct cache_st *csp) {
    int num_slots_used = 0;
    int i;

    for (i = 0; i < csp->size; i++) {
        if (csp->slots[i].valid == 1) {
            num_slots_used += 1;
        }
    }

    printf("=== Cache\n");
    printf("Type          = ");
    if (csp->type == CT_DIRECT_MAPPED) {
        printf("direct mapped\n");
    } else if (csp->type == CT_SET_ASSOCIATIVE) {
        printf("4-way set associative\n");
    }
    printf("Size          = %d slots\n", csp->size);
    printf("References    = %d\n", csp->refs);
    printf("Hits          = %d (%.2f%% hit ratio)\n", csp->hits, 
           ((double) csp->hits / (double) csp->refs) * 100.00);
    printf("Misses        = %d (%.2f%% miss ratio)\n", csp->misses,
           ((double) csp->misses / (double) csp->refs) * 100.00);    
    printf("Misses (cold) = %d\n", csp->misses_cold);
    printf("Misses (hot)  = %d\n", csp->misses_hot);
    printf("%% Used        = %.2f%%\n", ((double) num_slots_used / (double) csp->size) * 100.0);    
}


// Direct mapped lookup
uint32_t cache_lookup_dm(struct cache_st *csp, uint32_t addr) {
    uint32_t tag;
    uint32_t index;
    struct cache_slot_st *slot;
    uint32_t data;


    index = (addr >> 2) & csp->index_mask;
    tag = addr >> (csp->index_bits + 2);

    slot = &csp->slots[index];

    csp->refs += 1;
    if (slot->valid && (slot->tag == tag)) {
        // hit
        csp->hits += 1;
        data = slot->data;
        verbose("  cache tag hit for index %d tag %X addr %lX\n",
                index, tag, addr);

    } else {
        // miss
        csp->misses += 1;
        if (slot->valid == 0) {
            csp->misses_cold += 1;
            verbose("  cache tag (%X) miss for index %d tag %X addr %X (cold)\n",
                    slot->tag, index, tag, addr);
        } else {
            csp->misses_hot += 1;
            verbose("  cache tag (%X) miss for index %d tag %X addr %X (hot)\n",
                    slot->tag, index, tag, addr);
            
        }
        slot->valid = 1;
        slot->tag = tag;
        data = *((uint32_t *) addr);
        slot->data = data;
    }
    
    return data;
}

uint32_t cache_lookup_sa(struct cache_st *csp, uint32_t addr) {
    uint32_t tag, index, converted_index, data, min_timestamp;
    int num_sets, update_index, hot_miss;
    struct cache_slot_st *slot;

    csp->refs += 1;
    
    index = (addr >> 2) & csp->index_mask;
    tag = addr >> (csp->index_bits + 2);
    num_sets = csp->size / csp->ways;
    converted_index = (index % num_sets) * csp->ways;

    // update_index : keeps track of the index for the slot we need to update (for a miss)
    update_index = 0; 
    hot_miss = 1;
    min_timestamp = csp->refs;

    // Iterate through each slot/way in set, keeping track of the oldest OR first invalid slot in case of a miss
    // hit : get data directly from the slot, update timestamp, set update_index to -1, then BREAK
    // cold miss : if the slot is not valid, BREAK out of the loop and update this slot
    // hot miss:  compare timestamps, and keep track of the slot with the oldest/lowest timestamp
    
    for(int i = 0 ; i < csp->ways ; i++) {
        slot = &csp->slots[converted_index + i];
        if (slot->valid && (slot->tag == tag)) {
            // hit
            csp->hits += 1;
            data = slot->data;
            slot->timestamp = csp->refs;
            update_index = -1;
            break;
            
        } else if (!slot->valid) {
            // cold miss
            csp->misses_cold += 1;
            update_index = i;
            hot_miss = 0;
            break;
            
        } else {
            // hot miss
            if (slot->timestamp < min_timestamp) {
                min_timestamp = slot->timestamp;
                update_index = i;
            }
        }
    }

    // Handle miss : get data directly from address, then store it in the slot (desctibed by the update_index)
    if (update_index >= 0) {
        slot = &csp->slots[converted_index + update_index];
        slot->valid = 1;
        slot->tag = tag;
        data = *((uint32_t *) addr);
        slot->data = data;
        slot->timestamp = csp->refs;
        csp->misses += 1;
        
        if (hot_miss) {
            csp->misses_hot += 1;
        }
    }

    return data;
    
}

// Cache lookup
uint32_t cache_lookup(struct cache_st *csp, uint32_t addr) {
    uint32_t data;

    if (csp->type == CT_DIRECT_MAPPED) {
        data = cache_lookup_dm(csp, addr);
    } else if (csp->type == CT_SET_ASSOCIATIVE) {
        data = cache_lookup_sa(csp, addr);
    } else {
        data = *((uint32_t *) addr);        
    }
    return data;
}
