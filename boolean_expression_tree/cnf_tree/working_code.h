#pragma once
#include "tree.h"
#include <vector>

class working_code
{
public:
	working_code(std::vector<node*>* bitfield_init, std::vector<node*>* rng_vals_init);
	virtual	~working_code();

	std::vector<node*>* bitfield;
	std::vector<std::vector<node*>*>* rng_vals;

	void set_rng_init(std::vector<node*>* rng_vals_init);
	void add_rng_to_result(std::vector<node*>* result, uint8 rng_index);
	void rng_process();
	std::vector<node*>* get_rng(uint8 rng_index);

	void process_working(std::vector<node*>* schedule_entry, int sched_start_at);
	void process_working(std::vector<node*>* schedule_entry);

	node* alg06_node;

	std::vector<node*>* alg(uint8 alg_num);
	std::vector<node*>* alg0();
	std::vector<node*>* alg1();
	std::vector<node*>* alg2();
	std::vector<node*>* alg3();
	std::vector<node*>* alg4();
	std::vector<node*>* alg5();
	std::vector<node*>* alg6();
	std::vector<node*>* alg7();
};
