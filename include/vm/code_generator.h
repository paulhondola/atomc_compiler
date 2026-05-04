#pragma once

// code generation

#include "../analyzer/type_analyzer.h"
#include "vm.h"

// inserts after the specified instruction a conversion instruction
// only if necessary
void insert_conversion_if_needed(Instruction *before, Type *src_type, Type *dst_type);

// if is_left_value is true, generates an rval from the current value from stack
void add_rval(Instruction **code, bool is_left_value, Type *type);
