#ifndef CACHESIM_HPP
#define CACHESIM_HPP

#include <stdint.h>
#include <unordered_map>
#include <map>
#include <math.h>
#include <iostream>

struct cache_stats_t {
    uint64_t reads;
    uint64_t writes;
    uint64_t L1_accesses;
    uint64_t L1_read_misses;
    uint64_t L1_write_misses;
    uint64_t L2_read_misses;
    uint64_t L2_write_misses;
    uint64_t write_backs;
    uint64_t prefetched_blocks;
    uint64_t successful_prefetches; // The number of cache misses reduced by prefetching
    double   avg_access_time;
};

void cache_access(char rw, uint64_t address, cache_stats_t* p_stats);
void l2_access(char rw, uint64_t address, cache_stats_t* p_stats);
void setup_cache(uint64_t c1, uint64_t b1, uint64_t s1, uint64_t c2, uint64_t b2, uint64_t s2, uint32_t k);
void complete_cache(cache_stats_t *p_stats);
void prefetch(uint64_t address, cache_stats_t* p_stats);

static const uint64_t DEFAULT_C1 = 12;   /* 4KB Cache */
static const uint64_t DEFAULT_B1 = 5;    /* 32-byte blocks */
static const uint64_t DEFAULT_S1 = 3;    /* 8 blocks per set */
static const uint64_t DEFAULT_C2 = 15;   /* 32KB Cache */
static const uint64_t DEFAULT_B2 = 6;    /* 64-byte blocks */
static const uint64_t DEFAULT_S2 = 5;    /* 32 blocks per set */
static const uint32_t DEFAULT_K = 2;    /* prefetch 2 subsequent blocks */

/** Argument to cache_access rw. Indicates a load */
static const char     READ = 'r';
/** Argument to cache_access rw. Indicates a store */
static const char     WRITE = 'w';

/** Timestamp variable **/
static int t_s=0;
static int t_s2=0;

/** class for each cache_entry/tag **/

class c_tag
{
public:
    int valid_bit;
    int dirt_bit;
    int prefetched_bit;
    uint64_t og_address;
    int time_stamp;
    c_tag(uint64_t add, int ts):og_address(add),time_stamp(ts)
    {
        dirt_bit=0;
        prefetched_bit=0;
    }
};

/** class for each index of cache**/
class c_index
{
public:
std::unordered_map<uint64_t,c_tag> i_contents;
std::map<int,uint64_t> lru;
int max_set_size;
int current_set_size;
c_index(int size):max_set_size(size)
    {
        current_set_size=0;
    }  
};

/**class for cache **/

class cache
{
public:
    std::unordered_map<uint64_t,c_index> indices;
    int max_no_indices;
    int max_no_sets;
    cache()
    {

    }

};


/** variables for cache characteristics **/
static uint64_t C1 ;   
static uint64_t B1 ;    
static uint64_t S1 ;    
static uint64_t C2 ;   
static uint64_t B2 ;    
static uint64_t S2 ;    
static uint32_t K ;    


/** cache objects for level 1 and level 2 caches **/

static cache l1; /*level 1 cache*/
static cache l2; /*level 2 cache*/


/** parameters for prefetching **/
static uint64_t Previous_Block_Address=0;
static uint64_t Pending_Stride=0;


#endif /* CACHESIM_HPP */
