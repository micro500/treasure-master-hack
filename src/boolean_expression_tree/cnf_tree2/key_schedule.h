#pragma once
#include <vector>
#include "tree_node.h"

std::vector<tree_node*>* run_key_schedule(uint8 map, std::vector<tree_node*>::iterator key_schedule_data);
std::vector<tree_node*>* key_schedule_algorithm(uint8 map, std::vector<tree_node*>::iterator key_schedule_data, uint8 algorithm);
std::vector<tree_node*>* key_schedule_algorithm_0(uint8 map, std::vector<tree_node*>::iterator key_schedule_data);
std::vector<tree_node*>* key_schedule_algorithm_1(uint8 map, std::vector<tree_node*>::iterator key_schedule_data);
std::vector<tree_node*>* key_schedule_algorithm_2(uint8 map, std::vector<tree_node*>::iterator key_schedule_data);
std::vector<tree_node*>* key_schedule_algorithm_3(uint8 map, std::vector<tree_node*>::iterator key_schedule_data);
std::vector<tree_node*>* key_schedule_algorithm_4(uint8 map, std::vector<tree_node*>::iterator key_schedule_data);
std::vector<tree_node*>* key_schedule_algorithm_5(uint8 map, std::vector<tree_node*>::iterator key_schedule_data);
std::vector<tree_node*>* key_schedule_algorithm_6(uint8 map, std::vector<tree_node*>::iterator key_schedule_data);
std::vector<tree_node*>* key_schedule_algorithm_7(uint8 map, std::vector<tree_node*>::iterator key_schedule_data);