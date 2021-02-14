//
// Created by pavol on 17.07.18.
//

#include "z3_base.h"

expr round_real2int(const expr &x) {
  Z3_ast r = Z3_mk_real2int(x.ctx(), x + x.ctx().real_val(1,2));
  return expr(x.ctx(), r);
}