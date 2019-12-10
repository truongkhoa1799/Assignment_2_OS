
#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

static struct {
	uint32_t proc;	// ID of process currently uses this page
	int index;	// Index of the page in the list of pages allocated
			// to the process.
	int next;	// The next page in the list. -1 if it is the last
			// page.
} _mem_stat [NUM_PAGES];

static pthread_mutex_t mem_lock;

void init_mem(void) {
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	for (int i =0 ;i<NUM_PAGES; i++)
	{
		_mem_stat[i].proc =0;
		_mem_stat[i].index =-1;
		_mem_stat[i].next =-1;
	}
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* get offset of the virtual address */
static addr_t get_offset(addr_t addr) {
	return addr & ~((~0U) << OFFSET_LEN);
}

/* get the first layer index */
static addr_t get_first_lv(addr_t addr) {
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* get the second layer index */
static addr_t get_second_lv(addr_t addr) {
	return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
}

/* Search for page table table from the a segment table */
static struct page_table_t * get_page_table(
		addr_t index, 	// Segment level index
		struct seg_table_t * seg_table) { // first level table

	for (int i = 0; i < seg_table->size; i++) {
		if (seg_table->table[i].v_index == index)
		return seg_table->table[i].pages;
	}
	return NULL;

}

/* Translate virtual address to physical address. If [virtual_addr] is valid,
 * return 1 and write its physical counterpart to [physical_addr].
 * Otherwise, return 0 */
static int translate(
		addr_t virtual_addr, 	// Given virtual address
		addr_t * physical_addr, // Physical address to be returned
		struct pcb_t * proc) {  // Process uses given virtual address
	addr_t offset = get_offset(virtual_addr);
	addr_t first_lv = get_first_lv(virtual_addr);
	addr_t second_lv = get_second_lv(virtual_addr);
	//printf("in trans\n");
	struct page_table_t * page_table = NULL;
	page_table = get_page_table(first_lv, proc->seg_table);
	if (page_table == NULL) {
		return 0;
	}
	int i;
	for (i = 0; i < page_table->size; i++) {
		if (page_table->table[i].v_index == second_lv) {
			*physical_addr  = (page_table->table[i].p_index << OFFSET_LEN) + offset;
			return 1;
		}
	}
	return 0;
}

addr_t alloc_mem(uint32_t size, struct pcb_t * proc) {
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;
	uint32_t num_pages = (size % PAGE_SIZE != 0) ? size / PAGE_SIZE +1 : size / PAGE_SIZE ; 
	int mem_avail = 0; 
	int free_frames = 0;
	for (int i =0; i<PAGE_SIZE ; i++) if (_mem_stat[i].proc == 0) free_frames++;	 
	
	if (proc->bp + size <= RAM_SIZE && free_frames >= num_pages) mem_avail=1;

	if (mem_avail) 
	{
		ret_mem = proc->bp;
		proc->bp += num_pages * PAGE_SIZE;
		int first_layer = get_first_lv(ret_mem);
		int count = 0;
		int loc_frame;
		int pre_frame = -1;
		int index_seg;
		int index_page;
	
		int check_exist_seg = 0;
		for (int i = 0; i< proc->seg_table->size ; i++) 
		if (proc->seg_table->table[i].v_index == first_layer)
		{
			index_seg = i;
			check_exist_seg = 1;
			break;
		}
		if (check_exist_seg == 0)
		{
			proc->seg_table->size ++;
			index_seg = proc->seg_table->size - 1;
			proc->seg_table->table[index_seg].pages = malloc(sizeof(struct page_table_t));
			proc->seg_table->table[index_seg].v_index = first_layer;
			proc->seg_table->table[index_seg].pages->size = 0;
		}

		while (count < num_pages)
		{
			for (int k = 0; k < NUM_PAGES; k++)
				if (_mem_stat[k].proc == 0)
				{
					loc_frame = k;
					break;
				}
			_mem_stat[loc_frame].proc = proc->pid;
			_mem_stat[loc_frame].index = count;
			_mem_stat[loc_frame].next = -1;
			if (pre_frame !=-1) _mem_stat[pre_frame].next = loc_frame;
			pre_frame = loc_frame;

			int temp_first_layer = get_first_lv(ret_mem + count * PAGE_SIZE);
			int check = 0;
			while (check == 0){
				if (temp_first_layer == first_layer)
				{
					proc->seg_table->table[index_seg].pages->size++;
					index_page = proc->seg_table->table[index_seg].pages->size - 1;

					addr_t second_layer = get_second_lv(ret_mem + count * PAGE_SIZE);
					addr_t frame = loc_frame ;
					
					proc->seg_table->table[index_seg].pages->table[index_page].v_index = second_layer;
					proc->seg_table->table[index_seg].pages->table[index_page].p_index = frame;
					check =1;
				}
				else if (temp_first_layer != first_layer)
				{
					proc->seg_table->size ++;
					index_seg = proc->seg_table->size - 1;
					proc->seg_table->table[index_seg].pages = malloc(sizeof(struct page_table_t));
					proc->seg_table->table[index_seg].pages->size = 0 ;
					first_layer = temp_first_layer;
					proc->seg_table->table[index_seg].v_index = first_layer;
				}
			}
			count ++;
		}
	}
	// printf("_________ALLOCATE___________ num pages: %d , pid: :%d\n" ,num_pages , proc->pid);
	// printf("   Break point: %d", proc->bp>>10);
	// printf("	Pages used in memory: \n");
	// printf("	");
	// for (int i =0 ; i<NUM_PAGES; i++)
	// if (_mem_stat[i].proc!=0) printf("%d  ",i);
	
	// printf("\n");
	// printf("\n\tSegments used in virtual: \n");
	// for (int i = 0; i<proc->seg_table->size; i++) 
	// {
	// 	printf("	+ Seg: %d\n", i);
	// 	printf("		Pages: ");
	// 	for (int j = 0; j<proc->seg_table->table[i].pages->size ; j++)
	// 	printf("%d  ", j);
	// 	printf("\n\n");
	// }
	
	pthread_mutex_unlock(&mem_lock);
	return ret_mem;
}

int free_mem(addr_t address, struct pcb_t * proc) 
{
	//printf("in free: size seg:%d \n", proc->seg_table->size);	
	pthread_mutex_lock(&mem_lock);
	addr_t first_layer = get_first_lv(address);
	addr_t second_layer = get_second_lv(address);
	addr_t index_seg;
	addr_t frame;
	for (int i = 0; i < proc->seg_table->size; i++)
	if (proc->seg_table->table[i].v_index == first_layer)
	{
		index_seg = i;
		break;
	}

	for (int i = 0; i < proc->seg_table->table[index_seg].pages->size; i++)
		if (proc->seg_table->table[index_seg].pages->table[i].v_index == second_layer)
		{
			frame = proc->seg_table->table[index_seg].pages->table[i].p_index;
			break;
		}

	uint32_t loc_frame = frame;
	uint32_t pre_frame;
	int num_pages =0;
	while (loc_frame!= -1)
	{
		int temp_first_layer = get_first_lv(num_pages * PAGE_SIZE + address);
		second_layer = get_second_lv(num_pages * PAGE_SIZE + address);
		
		_mem_stat[loc_frame].proc = 0;
		_mem_stat[loc_frame].index = -1;
		pre_frame = loc_frame;
		loc_frame = _mem_stat[loc_frame].next;
		_mem_stat[pre_frame].next = -1;
		num_pages ++;
		for (int i = pre_frame << OFFSET_LEN; i < ((pre_frame + 1) << OFFSET_LEN) - 1; i++) _ram[i] = 0;
		
		int check = 0 ;
		while (check == 0)
		{
			if (temp_first_layer == first_layer)
			{
				int size_page = proc->seg_table->table[index_seg].pages->size;
				for (int i =0; i < size_page ; i++)
				if (proc->seg_table->table[index_seg].pages->table[i].v_index == second_layer)
				{
					proc->seg_table->table[index_seg].pages->table[i].p_index = 
					proc->seg_table->table[index_seg].pages->table[size_page-1].p_index;
					proc->seg_table->table[index_seg].pages->table[i].v_index = 
					proc->seg_table->table[index_seg].pages->table[size_page-1].v_index;
					proc->seg_table->table[index_seg].pages->size --;
					if (proc->seg_table->table[index_seg].pages->size == 0)
					{
						int size_seg = proc->seg_table->size;
						struct page_table_t *t = proc->seg_table->table[index_seg].pages;
						proc->seg_table->table[index_seg].pages = proc->seg_table->table[size_seg - 1].pages;
						proc->seg_table->table[index_seg].v_index = proc->seg_table->table[size_seg - 1].v_index;
						proc->seg_table->size--;
						free(t);
					}
					break;
				}
				check = 1;
			}
			else
			{
				for (int i =0; i<proc->seg_table->size;i++)
				if (proc->seg_table->table[i].v_index == temp_first_layer)
				{
					index_seg = i;
					first_layer = temp_first_layer;
					break;
				}
			}
		
		}
	}
	if (proc->bp == address + num_pages * PAGE_SIZE)
	proc->bp = proc->bp - num_pages * PAGE_SIZE;

	// printf("_________FREE___________ num pages: %d , pid: %d\n" , num_pages , proc->pid);
	// printf("   Break point: %d", proc->bp>>10);
	// printf("	Pages used in memory: \n");
	// printf("	");
	// for (int i =0 ; i<NUM_PAGES; i++)
	// if (_mem_stat[i].proc!=0) printf("%d  ",i);
	
	// printf("\n");
	// printf("\n\tSegments used in virtual: \n");
	// for (int i = 0; i<proc->seg_table->size; i++) 
	// {
	// 	printf("	+ Seg: %d\n", i);
	// 	printf("		Pages: ");
	// 	for (int j = 0; j<proc->seg_table->table[i].pages->size ; j++)
	// 	printf("%d(%d:%d)  ", j,proc->seg_table->table[i].pages->table[j].v_index,proc->seg_table->table[i].pages->table[j].p_index);
	// 	printf("\n\n");
	// }

	pthread_mutex_unlock(&mem_lock);
	return 0;
}

int read_mem(addr_t address, struct pcb_t * proc, BYTE * data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		pthread_mutex_lock(&mem_lock);
		*data = _ram[physical_addr];
		pthread_mutex_unlock(&mem_lock);
		return 0;
	}else{
		return 1;
	}
}

int write_mem(addr_t address, struct pcb_t * proc, BYTE data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		pthread_mutex_lock(&mem_lock);
		_ram[physical_addr] = data;
		pthread_mutex_unlock(&mem_lock);
		return 0;
	}else{
		return 1;
	}
}

void dump(void) {
	int i;
	for (i = 0; i < NUM_PAGES; i++) {
		if (_mem_stat[i].proc != 0) {
			printf("%03d: ", i);
			printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
				i << OFFSET_LEN,
				((i + 1) << OFFSET_LEN) - 1,
				_mem_stat[i].proc,
				_mem_stat[i].index,
				_mem_stat[i].next
			);
			int j;
			for (	j = i << OFFSET_LEN;
				j < ((i+1) << OFFSET_LEN) - 1;
				j++) {
				
				if (_ram[j] != 0) {
					printf("\t%05x: %02x\n", j, _ram[j]);
				}
					
			}
		}
	}
}


