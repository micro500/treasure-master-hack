#pragma once
#include "tree_node.h"

class working_code
{
public:
	working_code(std::vector<tree_node*>* bitfield_init, tree_manager* manager_in);
	virtual	~working_code();

	std::vector<tree_node*>* bitfield;
	std::vector<tree_node*>* rng_vals_init;

	std::vector<std::vector<tree_node*>*>* rng_res_nodes;
	std::vector<tree_node*>* rng_fwd_1;
	std::vector<tree_node*>* rng_fwd_128;

	std::vector<std::vector<tree_node*>*>* rng_vals;

	tree_manager* manager;

	std::vector<tree_node*>* get_alg_flags(std::vector<tree_node*>* nibble_selector, int nibble_index);

	//void add_rng_to_result(std::vector<tree_node*>* result, uint8 rng_index);
	void rng_process();
	void rng_process_precalc();
	void rng_process_steps();
	std::vector<tree_node*>* get_rng_res(uint8 rng_index);
	std::vector<tree_node*>* get_rng_fwd(uint8 rng_index);

	void process_working(std::vector<tree_node*>* schedule_entry, int sched_start_at, int sched_end_at);
	void process_working(std::vector<tree_node*>* schedule_entry, int sched_start_at);
	void process_working(std::vector<tree_node*>* schedule_entry);

	void process_forced_step(std::vector<tree_node*>* schedule_entry, int forced_alg, int nibble_index);

	bool use_rng_steps; 

	tree_node* alg06_node;

	tree_node* forced_alg_node;

	std::vector<tree_node*>* alg(uint8 alg_num);
	std::vector<tree_node*>* alg0();
	std::vector<tree_node*>* alg1();
	std::vector<tree_node*>* alg2();
	std::vector<tree_node*>* alg3();
	std::vector<tree_node*>* alg4();
	std::vector<tree_node*>* alg5();
	std::vector<tree_node*>* alg6();
	std::vector<tree_node*>* alg7();
};

std::vector<tree_node*>* decrypt_memory2(std::vector<tree_node*>* bitfield, uint8* encrypted_memory, uint8 memory_length);

std::vector<tree_node*>* rng_calc2(std::vector<tree_node*>::iterator rng_val);
std::vector<tree_node*>* rng_calc3(std::vector<tree_node*>::iterator rng1, std::vector<tree_node*>::iterator rng2);