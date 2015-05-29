#include "cachesim.hpp"


using namespace std;
/**
 * Subroutine for initializing the cache. You many add and initialize any global or heap
 * variables as needed.
 *
 * @c1 Total size of L1 in bytes is 2^C1
 * @b1 Size of each block in L1 in bytes is 2^B1
 * @s1 Number of blocks per set in L1 is 2^S1
 * @c2 Total size of L2 in bytes is 2^C2
 * @b2 Size of each block in L2 in bytes is 2^B2
 * @s2 Number of blocks per set in L2 is 2^S2
 * @k Prefetch K subsequent blocks
 */
void setup_cache(uint64_t c1, uint64_t b1, uint64_t s1, uint64_t c2, uint64_t b2, uint64_t s2, uint32_t k) {
	C1=c1; B1=b1; S1=s1; C2=c2; B2=b2; S2=s2; K=k;

	//setting up cache sizes
	l1.max_no_indices=pow(2,C1-B1-S1);
	l1.max_no_sets=pow(2,S1);
	l2.max_no_indices=pow(2,C2-B2-S2);
	l2.max_no_sets=pow(2,S2);


}

/**
 * Subroutine that simulates the cache one trace event at a time.
 *
 * @rw The type of event. Either READ or WRITE
 * @address  The target memory address
 * @p_stats Pointer to the statistics structure
 */
void cache_access(char rw, uint64_t address, cache_stats_t* p_stats) {
	//increase time stanp variable for every cache access
	t_s++;
	
	//Increase number of l1 accesses and also increase read or write access count depending on value of rw
	p_stats->L1_accesses++;
	if(rw=='w')
		{
			p_stats->writes++;
		}
	else
		p_stats->reads++;


	//caclculate address
	uint64_t tag;  
	uint64_t ind;
	uint64_t temp1,temp2;
	temp1= address>>B1;
	tag= temp1>>(C1-B1-S1);
	temp2=tag<<(C1-B1-S1);
	ind=temp1-temp2;
	
	//for a fully associative cache
	if(C1-B1==S1)
		ind=0;

	//check if tag is present in cache l1

	if(l1.indices.find(ind)!= l1.indices.end() && l1.indices.find(ind)->second.i_contents.find(tag)!=	l1.indices.find(ind)->second.i_contents.end())
	{
		//cout<<"hit for "<<address<<endl;
		//we have a hit
		//change status of lru
		//1) erase content from lru  and cache contents
		int t_s_tag=l1.indices.find(ind)->second.i_contents.find(tag)->second.time_stamp;		
		l1.indices.find(ind)->second.lru.erase(t_s_tag);

		//2)update timestanp for accessed block in tagstore and lru

		l1.indices.find(ind)->second.lru.insert(pair<int,uint64_t>(t_s,tag));
		l1.indices.find(ind)->second.i_contents.find(tag)->second.time_stamp=t_s;

		//change dirty bit for that block if access is a write
		if(rw=='w')
			{
				l1.indices.find(ind)->second.i_contents.find(tag)->second.dirt_bit=1;
			}

		//nothing else to be done
		return;
	}
	
	//tag is not present in cache

	//1)read it from l2

	l2_access('r',address,p_stats);

	//count it as l1 miss
	if(rw=='w')
		p_stats->L1_write_misses++;
	else
		p_stats->L1_read_misses++;


	//2) check if index exists
	if(l1.indices.find(ind)==l1.indices.end())
		l1.indices.insert(pair<uint64_t,c_index>(ind,c_index(l1.max_no_sets)));

	//3) check if there is space in that particular index 
	if(l1.indices.find(ind)->second.current_set_size>=l1.max_no_sets)
	{


		//4) if not then we have a problem, check if lru block is dirty
		uint64_t tag_to_be_replced=l1.indices.find(ind)->second.lru.begin()->second;

			//1) delete the block from the lru
			l1.indices.find(ind)->second.lru.erase(l1.indices.find(ind)->second.lru.begin()->first);

			//2) check if it is dirty
			if(l1.indices.find(ind)->second.i_contents.find(tag_to_be_replced)->second.dirt_bit==1)
			{
				//write block to l2
				l2_access('w',l1.indices.find(ind)->second.i_contents.find(tag_to_be_replced)->second.og_address,p_stats);
				//cout<<"tag to be replaced is "<<tag_to_be_replced<<endl;
			}

			//3) remove it from cache and index and decrease current size of index for that set
			l1.indices.find(ind)->second.i_contents.erase(tag_to_be_replced);
			l1.indices.find(ind)->second.current_set_size--;

	}


	//just insert the tag in lru and index and increase index set size
	l1.indices.find(ind)->second.lru.insert(pair<int,uint64_t>(t_s,tag));
	l1.indices.find(ind)->second.i_contents.insert(pair<uint64_t,c_tag>(tag,c_tag(address,t_s)));
	//if it is a write access then mark dirty bit as 1
	if(rw=='w')
	{
		l1.indices.find(ind)->second.i_contents.find(tag)->second.dirt_bit=1;
	}

	l1.indices.find(ind)->second.current_set_size++;
	return;


}

void l2_access(char rw, uint64_t address, cache_stats_t* p_stats)
{

	t_s2++;
	//caclculate address
	uint64_t tag;  
	uint64_t ind;
	uint64_t temp1,temp2;
	temp1= address>>B2;
	tag= temp1>>(C2-B2-S2);
	temp2=tag<<(C2-B2-S2);
	ind=temp1-temp2;

	//for a fully associative cache
	if(C2-B2==S2)
		ind=0;

	//check if tag is present in cache l1

	if(l2.indices.find(ind)!= l2.indices.end() && l2.indices.find(ind)->second.i_contents.find(tag)!=	l2.indices.find(ind)->second.i_contents.end())
	{
		
		//we have a hit
		//change status of lru
		//1) erase content from lru  and cache contents
		int t_s_tag=l2.indices.find(ind)->second.i_contents.find(tag)->second.time_stamp;
		l2.indices.find(ind)->second.lru.erase(t_s_tag);

		//2)update timestanp for accessed block in tagstore and lru

		l2.indices.find(ind)->second.lru.insert(pair<int,uint64_t>(t_s2,tag));
		l2.indices.find(ind)->second.i_contents.find(tag)->second.time_stamp=t_s2;

		//if it is a prefetched block, increase successful prefetch count and reset prefetch bit for that block to 0
		if(l2.indices.find(ind)->second.i_contents.find(tag)->second.prefetched_bit==1)
		{
			l2.indices.find(ind)->second.i_contents.find(tag)->second.prefetched_bit=0;
			p_stats->successful_prefetches++;
		}

		//change dirty bit for that block if access is a write
		if(rw=='w')
			{
				l2.indices.find(ind)->second.i_contents.find(tag)->second.dirt_bit=1;
				
			}

		//nothing else to be done
		return;
	}
	
	//tag is not present in cache

	//count it as l2 miss
	if(rw=='w')
		p_stats->L2_write_misses++;
	else
		p_stats->L2_read_misses++;


	//2) check if index exists
	if(l2.indices.find(ind)==l2.indices.end())
		l2.indices.insert(pair<uint64_t,c_index>(ind,c_index(l2.max_no_sets)));

	//3) check if there is space in that particular index 
	if(l2.indices.find(ind)->second.current_set_size>=l2.max_no_sets)
	{


		//4) if not then we have a problem, check if lru block is dirty
		uint64_t tag_to_be_replced=l2.indices.find(ind)->second.lru.begin()->second;

			//1) delete the block from the lru
			l2.indices.find(ind)->second.lru.erase(l2.indices.find(ind)->second.lru.begin()->first);

			//2) check if it is dirty
			if(l2.indices.find(ind)->second.i_contents.find(tag_to_be_replced)->second.dirt_bit==1)
			{
				//write back to memory
				p_stats->write_backs++;

			}

			//3) remove it from cache and index and decrease current size of index for that set
			l2.indices.find(ind)->second.i_contents.erase(tag_to_be_replced);
			l2.indices.find(ind)->second.current_set_size--;

	}


	//just insert the tag in lru and index and increase index set size
	l2.indices.find(ind)->second.lru.insert(pair<int,uint64_t>(t_s2,tag));
	l2.indices.find(ind)->second.i_contents.insert(pair<uint64_t,c_tag>(tag,c_tag(address,t_s2)));
	//if it is a write access then mark dirty bit as 1
	if(rw=='w')
	{
		l2.indices.find(ind)->second.i_contents.find(tag)->second.dirt_bit=1;
	}

	l2.indices.find(ind)->second.current_set_size++;


	//if it is a miss, let's prefetch and then complete
	prefetch(address,p_stats);
	return;
	
}


void prefetch(uint64_t address, cache_stats_t* p_stats) {
	uint64_t tag;  
	uint64_t ind;
	uint64_t temp1,temp2;
	address = (address>>B2)<<B2;
	uint64_t d=address-Previous_Block_Address;
	Previous_Block_Address=address;

	if(d==Pending_Stride) {
		for(int i=1;i<=K;i++)
			{
				//calculate address to be fetched
				address=d+address;

				//calculate tag and index
				temp1= address>>B2;
				tag= temp1>>(C2-B2-S2);
				temp2=tag<<(C2-B2-S2);
				ind=temp1-temp2;

				//for a fully associative cache
				if(C2-B2==S2)
					ind=0;

				//check if block in l2
				if(l2.indices.find(ind)!= l2.indices.end() && l2.indices.find(ind)->second.i_contents.find(tag)!=	l2.indices.find(ind)->second.i_contents.end())
				{
					//if yes do nothing
					continue;
				}
				else
				{
					p_stats->prefetched_blocks++;	
					if(l2.indices.find(ind)==l2.indices.end())
						l2.indices.insert(pair<uint64_t,c_index>(ind,c_index(l2.max_no_sets)));

					//3) check if there is space in that particular index and find ts for lru block
					int ts=l2.indices.find(ind)->second.lru.begin()->first;

					//if there is no space 
					if(l2.indices.find(ind)->second.current_set_size>=l2.max_no_sets)
						{


							//4) if not then we have a problem, check if lru block is dirty
							uint64_t tag_to_be_replced=l2.indices.find(ind)->second.lru.begin()->second;
							ts=l2.indices.find(ind)->second.lru.begin()->first;
							//1) delete the block from the lru
							l2.indices.find(ind)->second.lru.erase(ts);

							//2) check if it is dirty
							if(l2.indices.find(ind)->second.i_contents.find(tag_to_be_replced)->second.dirt_bit==1)
							{
							//write back to memory
							p_stats->write_backs++;

							}

							//3) remove it from cache and index and decrease current size of index for that set
							l2.indices.find(ind)->second.i_contents.erase(tag_to_be_replced);
							l2.indices.find(ind)->second.current_set_size--;

						}
						else
						{
							// if there is space
							ts--;
						}

						//just insert the tag in lru and index and increase index set size
						l2.indices.find(ind)->second.lru.insert(pair<int,uint64_t>(ts,tag));
						l2.indices.find(ind)->second.i_contents.insert(pair<uint64_t,c_tag>(tag,c_tag(address,ts)));
						l2.indices.find(ind)->second.i_contents.find(tag)->second.prefetched_bit=1;
						l2.indices.find(ind)->second.current_set_size++;
				}

			}
	}
	Pending_Stride=d;
}
/**
 * Subroutine for calculating overall statistics such as miss rate or average access time.
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_cache(cache_stats_t *p_stats) {
	float HT1= 2+(0.2*S1);
	float HT2= 4+(0.4*S2);
	float MR1=((float)p_stats->L1_read_misses+(float)p_stats->L1_write_misses)/(float)p_stats->L1_accesses;
	float MR2=((float)p_stats->L2_read_misses)/((float)p_stats->L1_read_misses+(float)p_stats->L1_write_misses);
	p_stats->avg_access_time=HT1+(MR1*(HT2+(MR2 * 500.0)));
}
