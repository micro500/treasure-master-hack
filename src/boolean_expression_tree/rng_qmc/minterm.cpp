#include <cstdint>
#include "minterm.h"

minterm::minterm(uint32_t value, uint32_t mask) : minterm_val(value), minterm_mask(mask) {}
minterm::~minterm() {}