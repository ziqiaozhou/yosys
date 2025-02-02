/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Clifford Wolf <clifford@clifford.at>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef SymCellTypes_H
#define SymCellTypes_H
#include "kernel/calc_sym.h"
#include "kernel/celltypes.h"
#include "kernel/yosys.h"
YOSYS_NAMESPACE_BEGIN

struct SymCellTypes {
  dict<RTLIL::IdString, CellType> cell_types;

  SymCellTypes() {}

  SymCellTypes(RTLIL::Design *design) { setup(design); }

  void setup(RTLIL::Design *design = NULL) {
    if (design)
      setup_design(design);

    setup_internals();
    setup_internals_mem();
    setup_stdcells();
    setup_stdcells_mem();
  }

  void setup_type(RTLIL::IdString type, const pool<RTLIL::IdString> &inputs,
                  const pool<RTLIL::IdString> &outputs,
                  bool is_evaluable = false) {
    CellType ct = {type, inputs, outputs, is_evaluable};
    cell_types[ct.type] = ct;
  }

  void setup_module(RTLIL::Module *module) {
    pool<RTLIL::IdString> inputs, outputs;
    for (RTLIL::IdString wire_name : module->ports) {
      RTLIL::Wire *wire = module->wire(wire_name);
      if (wire->port_input)
        inputs.insert(wire->name);
      if (wire->port_output)
        outputs.insert(wire->name);
    }
    setup_type(module->name, inputs, outputs);
  }

  void setup_design(RTLIL::Design *design) {
    for (auto module : design->modules())
      setup_module(module);
  }

  void setup_internals() {
    std::vector<RTLIL::IdString> unary_ops = {
        "$not",       "$pos",        "$neg",         "$reduce_and",
        "$reduce_or", "$reduce_xor", "$reduce_xnor", "$reduce_bool",
        "$logic_not", "$slice",      "$lut",         "$sop"};

    std::vector<RTLIL::IdString> binary_ops = {
        "$and",       "$or",       "$xor",    "$xnor",   "$shl", "$shr",
        "$sshl",      "$sshr",     "$shift",  "$shiftx", "$lt",  "$le",
        "$eq",        "$ne",       "$eqx",    "$nex",    "$ge",  "$gt",
        "$add",       "$sub",      "$mul",    "$div",    "$mod", "$pow",
        "$logic_and", "$logic_or", "$concat", "$macc"};
    IdString A = "\\A", B = "\\B", S = "\\S", Y = "\\Y";
    IdString P = "\\P", G = "\\G", C = "\\C", X = "\\X";
    IdString BI = "\\BI", CI = "\\CI", CO = "\\CO", EN = "\\EN";

    for (auto type : unary_ops)
      setup_type(type, {A}, {Y}, true);

    for (auto type : binary_ops)
      setup_type(type, {A, B}, {Y}, true);

    for (auto type : std::vector<RTLIL::IdString>({"$mux", "$pmux"}))
      setup_type(type, {A, B, S}, {Y}, true);

    setup_type("$lcu", {P, G, CI}, {CO}, true);
    setup_type("$alu", {A, B, CI, BI}, {X, Y, CO}, true);
    setup_type("$fa", {A, B, C}, {X, Y}, true);

    setup_type("$tribuf", {A, EN}, {Y}, true);

    setup_type("$assert", {A, EN}, pool<RTLIL::IdString>(), true);
    setup_type("$assume", {A, EN}, pool<RTLIL::IdString>(), true);
    setup_type("$live", {A, EN}, pool<RTLIL::IdString>(), true);
    setup_type("$fair", {A, EN}, pool<RTLIL::IdString>(), true);
    setup_type("$cover", {A, EN}, pool<RTLIL::IdString>(), true);
    setup_type("$initstate", pool<RTLIL::IdString>(), {Y}, true);
    setup_type("$anyconst", pool<RTLIL::IdString>(), {Y}, true);
    setup_type("$anyseq", pool<RTLIL::IdString>(), {Y}, true);
    setup_type("$allconst", pool<RTLIL::IdString>(), {Y}, true);
    setup_type("$allseq", pool<RTLIL::IdString>(), {Y}, true);
    setup_type("$equiv", {A, B}, {Y}, true);
  }

  void setup_internals_mem() {
    IdString SET = "\\SET", CLR = "\\CLR", CLK = "\\CLK", ARST = "\\ARST",
             EN = "\\EN";
    IdString Q = "\\Q", D = "\\D", ADDR = "\\ADDR", DATA = "\\DATA",
             RD_EN = "\\RD_EN";
    IdString RD_CLK = "\\RD_CLK", RD_ADDR = "\\RD_ADDR", WR_CLK = "\\WR_CLK",
             WR_EN = "\\WR_EN";
    IdString WR_ADDR = "\\WR_ADDR", WR_DATA = "\\WR_DATA",
             RD_DATA = "\\RD_DATA";
    IdString CTRL_IN = "\\CTRL_IN", CTRL_OUT = "\\CTRL_OUT";

    setup_type("$sr", {SET, CLR}, {Q});
    setup_type("$ff", {D}, {Q});
    setup_type("$dff", {CLK, D}, {Q});
    setup_type("$dffe", {CLK, EN, D}, {Q});
    setup_type("$dffsr", {CLK, SET, CLR, D}, {Q});
    setup_type("$adff", {CLK, ARST, D}, {Q});
    setup_type("$dlatch", {EN, D}, {Q});
    setup_type("$dlatchsr", {EN, SET, CLR, D}, {Q});

    setup_type("$memrd", {CLK, EN, ADDR}, {DATA});
    setup_type("$memwr", {CLK, EN, ADDR, DATA}, pool<RTLIL::IdString>());
    setup_type("$meminit", {ADDR, DATA}, pool<RTLIL::IdString>());
    setup_type("$mem",
               {RD_CLK, RD_EN, RD_ADDR, WR_CLK, WR_EN, WR_ADDR, WR_DATA},
               {RD_DATA});

    setup_type("$fsm", {CLK, ARST, CTRL_IN}, {CTRL_OUT});
  }

  void setup_stdcells() {
    IdString A = "\\A", B = "\\B", C = "\\C", D = "\\D";
    IdString E = "\\E", F = "\\F", G = "\\G", H = "\\H";
    IdString I = "\\I", J = "\\J", K = "\\K", L = "\\L";
    IdString M = "\\M", N = "\\N", O = "\\O", P = "\\P";
    IdString S = "\\S", T = "\\T", U = "\\U", V = "\\V";
    IdString Y = "\\Y";

    setup_type("$_BUF_", {A}, {Y}, true);
    setup_type("$_NOT_", {A}, {Y}, true);
    setup_type("$_AND_", {A, B}, {Y}, true);
    setup_type("$_NAND_", {A, B}, {Y}, true);
    setup_type("$_OR_", {A, B}, {Y}, true);
    setup_type("$_NOR_", {A, B}, {Y}, true);
    setup_type("$_XOR_", {A, B}, {Y}, true);
    setup_type("$_XNOR_", {A, B}, {Y}, true);
    setup_type("$_ANDNOT_", {A, B}, {Y}, true);
    setup_type("$_ORNOT_", {A, B}, {Y}, true);
    setup_type("$_MUX_", {A, B, S}, {Y}, true);
    setup_type("$_MUX4_", {A, B, C, D, S, T}, {Y}, true);
    setup_type("$_MUX8_", {A, B, C, D, E, F, G, H, S, T, U}, {Y}, true);
    setup_type("$_MUX16_",
               {A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, S, T, U, V},
               {Y}, true);
    setup_type("$_AOI3_", {A, B, C}, {Y}, true);
    setup_type("$_OAI3_", {A, B, C}, {Y}, true);
    setup_type("$_AOI4_", {A, B, C, D}, {Y}, true);
    setup_type("$_OAI4_", {A, B, C, D}, {Y}, true);
    setup_type("$_TBUF_", {A, E}, {Y}, true);
  }

  void setup_stdcells_mem() {
    IdString S = "\\S", R = "\\R", C = "\\C";
    IdString D = "\\D", Q = "\\Q", E = "\\E";

    std::vector<char> list_np = {'N', 'P'}, list_01 = {'0', '1'};

    for (auto c1 : list_np)
      for (auto c2 : list_np)
        setup_type(stringf("$_SR_%c%c_", c1, c2), {S, R}, {Q});

    setup_type("$_FF_", {D}, {Q});

    for (auto c1 : list_np)
      setup_type(stringf("$_DFF_%c_", c1), {C, D}, {Q});

    for (auto c1 : list_np)
      for (auto c2 : list_np)
        setup_type(stringf("$_DFFE_%c%c_", c1, c2), {C, D, E}, {Q});

    for (auto c1 : list_np)
      for (auto c2 : list_np)
        for (auto c3 : list_01)
          setup_type(stringf("$_DFF_%c%c%c_", c1, c2, c3), {C, R, D}, {Q});

    for (auto c1 : list_np)
      for (auto c2 : list_np)
        for (auto c3 : list_np)
          setup_type(stringf("$_DFFSR_%c%c%c_", c1, c2, c3), {C, S, R, D}, {Q});

    for (auto c1 : list_np)
      setup_type(stringf("$_DLATCH_%c_", c1), {E, D}, {Q});

    for (auto c1 : list_np)
      for (auto c2 : list_np)
        for (auto c3 : list_np)
          setup_type(stringf("$_DLATCHSR_%c%c%c_", c1, c2, c3), {E, S, R, D},
                     {Q});
  }

  void clear() { cell_types.clear(); }

  bool cell_known(RTLIL::IdString type) { return cell_types.count(type) != 0; }

  bool cell_output(RTLIL::IdString type, RTLIL::IdString port) {
    auto it = cell_types.find(type);
    return it != cell_types.end() && it->second.outputs.count(port) != 0;
  }

  bool cell_input(RTLIL::IdString type, RTLIL::IdString port) {
    auto it = cell_types.find(type);
    return it != cell_types.end() && it->second.inputs.count(port) != 0;
  }

  bool cell_evaluable(RTLIL::IdString type) {
    auto it = cell_types.find(type);
    return it != cell_types.end() && it->second.is_evaluable;
  }

  static RTLIL::SymConst eval_not(RTLIL::SymConst v) {

    return (~v.to_expr()).simplify();
  }

  static RTLIL::SymConst eval(RTLIL::IdString type, const RTLIL::SymConst &arg1,
                              const RTLIL::SymConst &arg2, bool signed1,
                              bool signed2, int result_len,
                              bool *errp = nullptr) {
    if (type == "$sshr" && !signed1)
      type = "$shr";
    if (type == "$sshl" && !signed1)
      type = "$shl";

    if (type != "$sshr" && type != "$sshl" && type != "$shr" &&
        type != "$shl" && type != "$shift" && type != "$shiftx" &&
        type != "$pos" && type != "$neg" && type != "$not") {
      if (!signed1 || !signed2)
        signed1 = false, signed2 = false;
    }

#define HANDLE_CELL_TYPE(_t)                                                   \
  if (type == "$" #_t)                                                         \
    return SymConst_##_t(arg1, arg2, signed1, signed2, result_len);
    HANDLE_CELL_TYPE(not)
    HANDLE_CELL_TYPE(and)
    HANDLE_CELL_TYPE(or)
    HANDLE_CELL_TYPE(xor)
    HANDLE_CELL_TYPE(xnor)
    HANDLE_CELL_TYPE(reduce_and)
    HANDLE_CELL_TYPE(reduce_or)
    HANDLE_CELL_TYPE(reduce_xor)
    HANDLE_CELL_TYPE(reduce_xnor)
    HANDLE_CELL_TYPE(reduce_bool)
    HANDLE_CELL_TYPE(logic_not)
    HANDLE_CELL_TYPE(logic_and)
    HANDLE_CELL_TYPE(logic_or)
    HANDLE_CELL_TYPE(shl)
    HANDLE_CELL_TYPE(shr)
    HANDLE_CELL_TYPE(sshl)
    HANDLE_CELL_TYPE(sshr)
    HANDLE_CELL_TYPE(shift)
    HANDLE_CELL_TYPE(shiftx)
    HANDLE_CELL_TYPE(lt)
    HANDLE_CELL_TYPE(le)
    HANDLE_CELL_TYPE(eq)
    HANDLE_CELL_TYPE(ne)
    HANDLE_CELL_TYPE(eqx)
    HANDLE_CELL_TYPE(nex)
    HANDLE_CELL_TYPE(ge)
    HANDLE_CELL_TYPE(gt)
    HANDLE_CELL_TYPE(add)
    HANDLE_CELL_TYPE(sub)
    HANDLE_CELL_TYPE(mul)
    HANDLE_CELL_TYPE(div)
    HANDLE_CELL_TYPE(mod)
    HANDLE_CELL_TYPE(pow)
    HANDLE_CELL_TYPE(pos)
    HANDLE_CELL_TYPE(neg)
#undef HANDLE_CELL_TYPE

    if (type == "$_BUF_")
      return arg1;
    if (type == "$_NOT_")
      return eval_not(arg1);
    if (type == "$_AND_")
      return SymConst_and(arg1, arg2, false, false, 1);
    if (type == "$_NAND_")
      return eval_not(SymConst_and(arg1, arg2, false, false, 1));
    if (type == "$_OR_")
      return SymConst_or(arg1, arg2, false, false, 1);
    if (type == "$_NOR_")
      return eval_not(SymConst_or(arg1, arg2, false, false, 1));
    if (type == "$_XOR_")
      return SymConst_xor(arg1, arg2, false, false, 1);
    if (type == "$_XNOR_")
      return SymConst_xnor(arg1, arg2, false, false, 1);
    if (type == "$_ANDNOT_")
      return SymConst_and(arg1, eval_not(arg2), false, false, 1);
    if (type == "$_ORNOT_")
      return SymConst_or(arg1, eval_not(arg2), false, false, 1);

    if (errp != nullptr) {
      *errp = true;
      return State::Sm;
    }

    log_abort();
  }

  static RTLIL::SymConst eval(RTLIL::Cell *cell, const RTLIL::SymConst &arg1,
                              const RTLIL::SymConst &arg2,
                              bool *errp = nullptr) {
    if (cell->type == "$slice") {
      RTLIL::SymConst ret;
      int width = cell->parameters.at("\\Y_WIDTH").as_int();
      int offset = cell->parameters.at("\\OFFSET").as_int();
      for (size_t i = 0; i < width; ++i) {
        ret.push_back(arg1[i + offset]);
      }
      return ret;
    }

    if (cell->type == "$concat") {
      return z3::concat(arg1.to_expr(), arg2.to_expr());
    }

    if (cell->type == "$lut") {
      int width = cell->parameters.at("\\WIDTH").as_int();

      std::vector<RTLIL::StateSym> t(cell->parameters.at("\\LUT").bits.begin(),
                                     cell->parameters.at("\\LUT").bits.end());
      while (GetSize(t) < (1 << width))
        t.push_back(RTLIL::S0);
      t.resize(1 << width);

      for (int i = width - 1; i >= 0; i--) {
        RTLIL::StateSym sel = arg1[i];
        std::vector<RTLIL::StateSym> new_t;
        if (sel == RTLIL::S0)
          new_t = std::vector<RTLIL::StateSym>(t.begin(),
                                               t.begin() + GetSize(t) / 2);
        else if (sel == RTLIL::S1)
          new_t =
              std::vector<RTLIL::StateSym>(t.begin() + GetSize(t) / 2, t.end());
        else
          for (int j = 0; j < GetSize(t) / 2; j++)
            new_t.push_back(t[j] == t[j + GetSize(t) / 2] ? t[j] : RTLIL::Sx);
        t.swap(new_t);
      }

      log_assert(GetSize(t) == 1);
      return t;
    }

    if (cell->type == "$sop") {
      int width = cell->parameters.at("\\WIDTH").as_int();
      int depth = cell->parameters.at("\\DEPTH").as_int();
      auto ct = cell->parameters.at("\\TABLE").bits;
      std::vector<RTLIL::StateSym> t(ct.begin(), ct.end());
      while (GetSize(t) < width * depth * 2)
        t.push_back(RTLIL::S0);

      RTLIL::StateSym default_ret = State::S0;

      for (int i = 0; i < depth; i++) {
        bool match = true;
        bool match_x = true;

        for (int j = 0; j < width; j++) {
          RTLIL::StateSym a = arg1[j];
          if (t.at(2 * width * i + 2 * j + 0) == State::S1) {
            if (a == State::S1)
              match_x = false;
            if (a != State::S0)
              match = false;
          }
          if (t.at(2 * width * i + 2 * j + 1) == State::S1) {
            if (a == State::S0)
              match_x = false;
            if (a != State::S1)
              match = false;
          }
        }

        if (match)
          return State::S1;

        if (match_x)
          default_ret = State::Sx;
      }

      return default_ret;
    }

    bool signed_a = cell->parameters.count("\\A_SIGNED") > 0 &&
                    cell->parameters["\\A_SIGNED"].as_bool();
    bool signed_b = cell->parameters.count("\\B_SIGNED") > 0 &&
                    cell->parameters["\\B_SIGNED"].as_bool();
    int result_len = cell->parameters.count("\\Y_WIDTH") > 0
                         ? cell->parameters["\\Y_WIDTH"].as_int()
                         : -1;
    return eval(cell->type, arg1, arg2, signed_a, signed_b, result_len, errp);
  }

  static RTLIL::SymConst eval(RTLIL::Cell *cell, const RTLIL::SymConst &arg1,
                              const RTLIL::SymConst &arg2,
                              const RTLIL::SymConst &arg3,
                              bool *errp = nullptr) {
    if (cell->type.in("$mux", "$pmux", "$_MUX_")) {
      z3::expr_vector ret = arg1.bits;
      size_t result_len = arg1.size();
      assert(result_len > 0);
      assert(result_len * arg3.size() == arg2.size());
      for (int i = 0; i < arg3.size(); ++i) {
        if (arg3[i].to_state() == State::S1) {
          return arg2.extract(i * result_len, result_len);
        }
        if (arg3[i].to_state() == State::S0)
          continue;
        for (int j = 0; j < result_len; ++j) {
          ret[j] = (arg3[i].to_expr() & arg2[j + i * result_len].to_expr()) |
                   ((~arg3[i].to_expr()) & arg1[j].to_expr());
        }
      }
    //  std::cerr << "mux end, ret.to_string=\n";
      //std::cerr << "=" << ret << "\n";
      return ret;
    }

    if (cell->type == "$_AOI3_")
      return eval_not(SymConst_or(SymConst_and(arg1, arg2, false, false, 1),
                                  arg3, false, false, 1));
    if (cell->type == "$_OAI3_")
      return eval_not(SymConst_and(SymConst_or(arg1, arg2, false, false, 1),
                                   arg3, false, false, 1));

    log_assert(arg3.bits.size() == 0);
    return eval(cell, arg1, arg2, errp);
  }

  static RTLIL::SymConst eval(RTLIL::Cell *cell, const RTLIL::SymConst &arg1,
                              const RTLIL::SymConst &arg2,
                              const RTLIL::SymConst &arg3,
                              const RTLIL::SymConst &arg4,
                              bool *errp = nullptr) {
    if (cell->type == "$_AOI4_")
      return eval_not(SymConst_or(SymConst_and(arg1, arg2, false, false, 1),
                                  SymConst_and(arg3, arg4, false, false, 1),
                                  false, false, 1));
    if (cell->type == "$_OAI4_")
      return eval_not(SymConst_and(SymConst_or(arg1, arg2, false, false, 1),
                                   SymConst_and(arg3, arg4, false, false, 1),
                                   false, false, 1));

    log_assert(arg4.bits.size() == 0);
    return eval(cell, arg1, arg2, arg3, errp);
  }
};

YOSYS_NAMESPACE_END

#endif
