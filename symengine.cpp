/*
 * LLSE: Low Level Symbolic Execution Engine
 *
 * 1. Run symbolic execution engine on a binary trace
 * 2. Given a concrete input, interprete a formula and output the result
 * 3. Output formulas as CVC format
 *
 */

#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <queue>

using namespace std;

#include "core.h"
#include "symengine.h"
#include "varmap.h"

enum ValueTy {SYMBOL, CONCRETE};
enum OperTy {ADD, MOV, SHL, XOR, SHR};


// A symbolic or concrete value in a formula
struct Value {
     ValueTy valty;             // value type: SYMBOL or CONCRETE
     Operation *opr;
     string conval;             // concrete value
     int id;                 // a unique id for each value
     static int idseed;

     Value(ValueTy vty);
     Value(ValueTy vty, string con); // constructor for concrete value
     Value(ValueTy vty, Operation *oper);
     bool isSymbol();
};

int Value::idseed = 0;

Value::Value(ValueTy vty) : opr(NULL)
{
     id = ++idseed;
     valty = vty;
}

Value::Value(ValueTy vty, string con) : opr(NULL)
{
     id = ++idseed;
     valty = vty;
     conval = con;
}

Value::Value(ValueTy vty, Operation *oper) {
     id = ++idseed;
     valty = vty;
     opr = oper;
}

bool Value::isSymbol() {
     if (this->valty == SYMBOL)
          return true;
     else
          return false;
}

string getValueName(Value *v)
{
     if (v->valty == SYMBOL)
          return "sym" + to_string(v->id);
     else
          return v->conval;
}

// An operation taking several values to calculate a result value
struct Operation {
     string opty;
     Value *val[3];

     Operation(string opt, Value *v1);
     Operation(string opt, Value *v1, Value *v2);
     Operation(string opt, Value *v1, Value *v2, Value *v3);
};

Operation::Operation(string opt, Value *v1)
{
     opty = opt;
     val[0] = v1;
     val[1] = NULL;
     val[2] = NULL;
}

Operation::Operation(string opt, Value *v1, Value *v2)
{
     opty = opt;
     val[0] = v1;
     val[1] = v2;
     val[2] = NULL;
}

Operation::Operation(string opt, Value *v1, Value *v2, Value *v3)
{
     opty = opt;
     val[0] = v1;
     val[1] = v2;
     val[2] = v3;
}

Value *buildop1(string opty, Value *v1)
{
     Operation *oper = new Operation(opty, v1);
     Value *result;

     if (v1->isSymbol())
          result = new Value(SYMBOL, oper);
     else
          result = new Value(CONCRETE, oper);

     return result;
}

Value *buildop2(string opty, Value *v1, Value *v2)
{
     Operation *oper = new Operation(opty, v1, v2);
     Value *result;
     if (v1->isSymbol() || v2->isSymbol())
          result = new Value(SYMBOL, oper);
     else
          result = new Value(CONCRETE, oper);

     return result;
}

Value *buildop3(string opty, Value *v1, Value *v2, Value *v3)
{
     Operation *oper = new Operation(opty, v1, v2, v3);
     Value *result;

     if (v1->isSymbol() || v2->isSymbol() || v3->isSymbol())
          result = new Value(SYMBOL, oper);
     else
          result = new Value(CONCRETE, oper);

     return result;
}


// class SEEngine Implementation
void SEEngine::init(Value *v1, Value *v2, Value *v3, Value *v4,
                    Value *v5, Value *v6, Value *v7, Value *v8,
                    list<Inst>::iterator it1,
                    list<Inst>::iterator it2)
{
     ctx["eax"] = v1;
     ctx["ebx"] = v2;
     ctx["ecx"] = v3;
     ctx["edx"] = v4;
     ctx["esi"] = v5;
     ctx["edi"] = v6;
     ctx["esp"] = v7;
     ctx["ebp"] = v8;

     this->start = it1;
     this->end = it2;
}

void SEEngine::init(list<Inst>::iterator it1,
                    list<Inst>::iterator it2)
{
     this->start = it1;
     this->end = it2;
}

void SEEngine::initAllRegSymol(list<Inst>::iterator it1,
                               list<Inst>::iterator it2)
{
     ctx["eax"] = new Value(SYMBOL);
     ctx["ebx"] = new Value(SYMBOL);
     ctx["ecx"] = new Value(SYMBOL);
     ctx["edx"] = new Value(SYMBOL);
     ctx["esi"] = new Value(SYMBOL);
     ctx["edi"] = new Value(SYMBOL);
     ctx["esp"] = new Value(SYMBOL);
     ctx["ebp"] = new Value(SYMBOL);

     this->start = it1;
     this->end = it2;
}

// instructions which have no effect in symbolic execution
set<string> noeffectinst = {"test","jmp","jz","jbe","jo","jno","js","jns","je","jne",
                            "jnz","jb","jnae","jc","jnb","jae",
                            "jnc","jna","ja","jnbe","jl",
                            "jnge","jge","jnl","jle","jng","jg",
                            "jnle","jp","jpe","jnp","jpo","jcxz",
                            "jecxz"};

int SEEngine::symexec()
{
     for (list<Inst>::iterator it = start; it != end; ++it) {
          // cout << hex << it->addrn << ": ";
          // cout << it->opcstr << '\n';

          // skip no effect instructions
          if (noeffectinst.find(it->opcstr) != noeffectinst.end()) continue;

          switch (it->oprnum) {
          case 0:
               break;
          case 1:
          {
               Operand *op0 = it->oprd[0];
               Value *v0, *res;
               if (it->opcstr == "push") {
                    if (op0->ty == Operand::ImmValue) {
                         v0 = new Value(CONCRETE, op0->field[0]);
                         mem[it->memaddr] = v0;
                    } else if (op0->ty == Operand::Reg) {
                         mem[it->memaddr] = ctx[op0->field[0]];
                    } else if (op0->ty == Operand::Mem) {
                         // The memaddr in the trace is the read address
                         // We need to compute the write address
                         uint32_t espval = it->ctxreg[6];
                         if (memfind(it->memaddr)) {
                              v0 = mem[it->memaddr];
                         } else {
                              v0 = new Value(SYMBOL);
                              mem[it->memaddr] = v0;
                         }
                         mem[espval-4] = v0;
                    } else {
                         cout << "push error: the operand is not Imm, Reg or Mem!" << endl;
                         return 1;
                    }
               } else if (it->opcstr == "pop") {
                    if (op0->ty == Operand::Reg) {
                          ctx[op0->field[0]] = mem[it->memaddr];
                    } else {
                         cout << "pop error: the operand is not Reg!" << endl;
                         return 1;
                    }
               } else if (it->opcstr == "neg") {
                    if (op0->ty == Operand::Reg) {
                         v0 = ctx[op0->field[0]];
                         res = buildop1(it->opcstr, v0);
                         ctx[op0->field[0]] = res;
                    } else if (op0->ty == Operand::Mem) {
                         cout << "neg error: the operand is not Reg!" << endl;
                         return 1;
                    }
               } else {
                    cout << "instruction " << it->opcstr << " is not handled!" << endl;
               }
               break;
          }
          case 2:
          {
               Operand *op0 = it->oprd[0];
               Operand *op1 = it->oprd[1];
               Value *v0, *v1, *res;

               if (it->opcstr == "mov") { // handle mov instruction
                    if (op0->ty == Operand::Reg) {
                         if (op1->ty == Operand::ImmValue) { // mov reg, 0x1111
                              v1 = new Value(CONCRETE, op1->field[0]);
                              ctx[op0->field[0]] = v1;
                         } else if (op1->ty == Operand::Reg) { // mov reg, reg
                              ctx[op0->field[0]] = ctx[op1->field[0]];
                         } else if (op1->ty == Operand::Mem) { // mov reg, dword ptr [ebp+0x1]
                              /* 1. Get mem address
                                 2. check whether the mem address has been accessed
                                 3. if not, create a new value
                                 4. else load the value in that memory
                               */
                              if (memfind(it->memaddr)) {
                                   v1 = mem[it->memaddr];
                              } else {
                                   v1 = new Value(SYMBOL);
                                   mem[it->memaddr] = v1;
                              }
                              ctx[op0->field[0]] = v1;
                         } else {
                              cout << "op1 is not ImmValue, Reg or Mem" << endl;
                              return 1;
                         }
                    } else if (op0->ty == Operand::Mem) {
                         if (op1->ty == Operand::ImmValue) { // mov dword ptr [ebp+0x1], 0x1111
                              mem[it->memaddr] = new Value(CONCRETE, op1->field[0]);
                         } else if (op1->ty == Operand::Reg) { // mov dword ptr [ebp+0x1], reg
                              mem[it->memaddr] = ctx[op1->field[0]];
                         }
                    } else {
                         cout << "Error: The first operand in MOV is not Reg or Mem!" << endl;
                    }
               } else if (it->opcstr == "lea") { // handle lea instruction
                    /* lea reg, ptr [edx+eax*1]
                       interpret lea instruction based on different address type
                       1. op0 must be reg
                       2. op1 must be addr
                     */
                    if (op0->ty != Operand::Reg || op1->ty != Operand::Mem) {
                         cout << "lea format error!" << endl;
                    }
                    switch (op1->tag) {
                    case 5:
                    {
                         Value *f0, *f1, *f2; // corresponding field[0-2] in operand
                         f0 = ctx[op1->field[0]];
                         f1 = ctx[op1->field[1]];
                         f2 = new Value(CONCRETE, op1->field[2]);
                         res = buildop2("imul", f1, f2);
                         res = buildop2("add",f0, res);
                         ctx[op0->field[0]] = res;
                         break;
                    }
                    default:
                         cout << "Other tags in addr is not ready for lea!" << endl;
                         break;
                    }
               } else if (it->opcstr == "xchg") {
                    if (op1->ty == Operand::Reg) {
                         v1 = ctx[op1->field[0]];
                         if (op0->ty == Operand::Reg) {
                              v0 = ctx[op0->field[0]];
                              ctx[op1->field[0]] = v0; // xchg reg, reg
                              ctx[op0->field[0]] = v1;
                         } else if (op0->ty == Operand::Mem) {
                              if (memfind(it->memaddr)) {
                                   v0 = mem[it->memaddr];
                              } else {
                                   v0 = new Value(SYMBOL);
                                   mem[it->memaddr] = v0;
                              }
                              ctx[op1->field[0]] = v0; // xchg mem, reg
                              mem[it->memaddr] = v1;
                         } else {
                              cout << "xchg error: 1" << endl;
                         }
                    } else if (op1->ty == Operand::Mem) {
                         if (memfind(it->memaddr)) {
                              v1 = mem[it->memaddr];
                         } else {
                              v1 = new Value(SYMBOL);
                              mem[it->memaddr] = v1;
                         }
                         if (op0->ty == Operand::Reg) {
                              v0 = ctx[op0->field[0]];
                              ctx[op0->field[0]] = v1; // xchg reg, mem
                              mem[it->memaddr] = v0;
                         } else {
                              cout << "xchg error 3" << endl;
                         }
                    } else {
                         cout << "xchg error: 2" << endl;
                    }
               } else { // handle other instructions
                    if (op1->ty == Operand::ImmValue) {
                         v1 = new Value(CONCRETE, op1->field[0]);
                    } else if (op1->ty == Operand::Reg) {
                         v1 = ctx[op1->field[0]];
                    } else if (op1->ty == Operand::Mem) {
                         if (memfind(it->memaddr)) {
                              v1 = mem[it->memaddr];
                         } else {
                              v1 = new Value(SYMBOL);
                              mem[it->memaddr] = v1;
                         }
                    } else {
                         cout << "other instructions: op1 is not ImmValue, Reg, or Mem!" << endl;
                         return 1;
                    }

                    if (op0->ty == Operand::Reg) { // dest op is reg
                         v0 = ctx[op0->field[0]];
                         res = buildop2(it->opcstr, v0, v1);
                         ctx[op0->field[0]] = res;
                    } else if (op0->ty == Operand::Mem) { // dest op is mem
                         if (memfind(it->memaddr)) {
                              v0 = mem[it->memaddr];
                         } else {
                              v0 = new Value(SYMBOL);
                              mem[it->memaddr] = v0;
                         }
                         res = buildop2(it->opcstr, v0, v1);
                         mem[it->memaddr] = res;
                    } else {
                         cout << "other instructions: op2 is not ImmValue, Reg, or Mem!" << endl;
                         return 1;
                    }

               }

               break;
          }
          case 3:
          {
               Operand *op0 = it->oprd[0];
               Operand *op1 = it->oprd[1];
               Operand *op2 = it->oprd[2];
               Value *v1, *v2, *res;

               // three operands instructions are reduced to two operands
               if (it->opcstr == "imul" && op0->ty == Operand::Reg &&
                   op1->ty == Operand::Reg && op2->ty == Operand::ImmValue) { // imul reg, reg, imm
                    v1 = ctx[op1->field[0]];
                    v2 = new Value(CONCRETE, op2->field[0]);
                    res = buildop2(it->opcstr, v1, v2);
                    ctx[op0->field[0]] = res;
               } else {
                    cout << "three operands instructions other than imul are not handled!" << endl;
               }
               break;
          }
          default:
               cout << "all instructions: number of operands is larger than 4!" << endl;
               break;
          }
     }
     return 0;
}

void traverse(Value *v)
{
     if (v == NULL) return;

     Operation *op = v->opr;
     if (op == NULL) {
          if (v->valty == CONCRETE)
               cout << v->conval;
          else
               cout << "sym" << v->id;
     } else {
          cout << "(" << op->opty << " ";
          traverse(op->val[0]);
          cout << " ";
          traverse(op->val[1]);
          cout << ")";
     }
}

void SEEngine::outputFormula(string reg)
{
     Value *v = ctx[reg];
     cout << "sym" << v->id << "=" << endl;
     traverse(v);
     cout << endl;
}

vector<Value*> SEEngine::getAllOutput()
{
     vector<Value*> outputs;
     Value *v;

     // symbols in registers
     v = ctx["eax"];
     if (v->opr != NULL)
          outputs.push_back(v);
     v = ctx["ebx"];
     if (v->opr != NULL)
          outputs.push_back(v);
     v = ctx["ecx"];
     if (v->opr != NULL)
          outputs.push_back(v);
     v = ctx["edx"];
     if (v->opr != NULL)
          outputs.push_back(v);

     // symbols in memory
     for (auto const& x : mem) {
          v = x.second;
          if (v->opr != NULL)
               outputs.push_back(v);
     }

     return outputs;
}

void SEEngine::printAllRegFormulas()
{
     cout << "eax: ";
     outputFormula("eax");
     printInputSymbols("eax");
     cout << endl;

     cout << "ebx: ";
     outputFormula("ebx");
     printInputSymbols("ebx");
     cout << endl;

     cout << "ecx: ";
     outputFormula("ecx");
     printInputSymbols("ecx");
     cout << endl;

     cout << "edx: ";
     outputFormula("edx");
     printInputSymbols("edx");
     cout << endl;

     cout << "esi: ";
     outputFormula("esi");
     printInputSymbols("esi");
     cout << endl;

     cout << "edi: ";
     outputFormula("edi");
     printInputSymbols("edi");
     cout << endl;
}

void SEEngine::printMemFormula()
{
     for (auto const& x : mem) {
          Value *v = x.second;
          printf("%x: ", x.first);
          cout << "sym" << v->id << "=" << endl;
          traverse(v);
          cout << endl << endl;
     }
}

set<Value*> *getInputs(Value *output)
{
     queue<Value*> que;
     que.push(output);

     set<Value*> *inputset = new set<Value*>;

     while (!que.empty()) {
          Value *v = que.front();
          Operation *op = v->opr;
          que.pop();

          if(op == NULL) {
               if (v->valty == SYMBOL)
                    inputset->insert(v);
          } else {
               for (int i = 0; i < 3; ++i) {
                    if (op->val[i] != NULL) que.push(op->val[i]);
               }
          }
     }

     return inputset;
}

void SEEngine::printInputSymbols(string output)
{
     Value *v = ctx[output];
     set<Value*> *insyms = getInputs(v);

     cout << insyms->size() << " input symbols: ";
     for (set<Value*>::iterator it = insyms->begin(); it != insyms->end(); ++it) {
          cout << "sym" << (*it)->id << " ";
     }
     cout << endl;
}

// Recursively evaluate a Value
uint32_t eval(Value *v, map<Value*, uint32_t> *inmap)
{
     Operation *op = v->opr;
     if (op == NULL) {
          if (v->valty == CONCRETE)
               return stoul(v->conval, 0, 16);
          else
               return (*inmap)[v];
     } else {
          uint32_t op0, op1;
          // uint32_t op2;

          if (op->val[0] != NULL) op0 = eval(op->val[0], inmap);
          if (op->val[1] != NULL) op1 = eval(op->val[1], inmap);
          // if (op->val[2] != NULL) op2 = eval(op->val[2], inmap);

          if (op->opty == "add") {
               return op0 + op1;
          } else if (op->opty == "sub") {
               return op0 - op1;
          } else if (op->opty == "imul") {
               return op0 * op1;
          } else if (op->opty == "xor") {
               return op0 ^ op1;
          } else if (op->opty == "and") {
               return op0 & op1;
          } else if (op->opty == "or") {
               return op0 | op1;
          } else if (op->opty == "shl") {
               return op0 << op1;
          } else if (op->opty == "shr") {
               return op0 >> op1;
          } else if (op->opty == "neg") {
               return ~op0 + 1;
          } else if (op->opty == "inc") {
               return op0 + 1;
          } else {
               cout << "Instruction: " << op->opty << "is not interpreted!" << endl;
               return 1;
          }
     }
}

// Given inputs, concrete compute the output value of a formula
uint32_t SEEngine::conexec(Value *f, map<Value*, uint32_t> *inmap)
{
     set<Value*> *inputsym = getInputs(f);
     set<Value*> inmapkeys;
     for (map<Value*, uint32_t>::iterator it = inmap->begin(); it != inmap->end(); ++it) {
          inmapkeys.insert(it->first);
     }

     if (inmapkeys != *inputsym) {
          cout << "Some inputs don't have parameters!" << endl;
          return 1;
     }

     return eval(f, inmap);
}

// build a map based on a var vector and a input vector
map<Value*, uint32_t> buildinmap(vector<Value*> *vv, vector<uint32_t> *input)
{
     map<Value*, uint32_t> inmap;
     if (vv->size() != input->size()) {
          cout << "number of input symbols is wrong!" << endl;
          return inmap;
     }

     for (int i = 0, max = vv->size(); i < max; ++i) {
          inmap.insert(pair<Value*,uint32_t>((*vv)[i], (*input)[i]));
     }

     return inmap;
}

// get all inputs of formula f and push them into a vector
vector<Value*> getInputVector(Value *f)
{
     set<Value*> *inset = getInputs(f);

     vector<Value*> vv;
     for (set<Value*>::iterator it = inset->begin(); it != inset->end(); ++it) {
          vv.push_back(*it);
     }

     return vv;
}


// a post fix in symbol names to identify they are coming from different formulas
string sympostfix;

// output the calculation of v as a formula
void outputCVC(Value *v, FILE *fp)
{
     if (v == NULL) return;

     Operation *op = v->opr;
     if (op == NULL) {
          if (v->valty == CONCRETE) {
               uint32_t i = stoul(v->conval, 0, 16);
               fprintf(fp, "0hex%08x", i);
          } else
               fprintf(fp, "sym%d%s", v->id, sympostfix.c_str());
     } else {
          if (op->opty == "add") {
               fprintf(fp, "BVPLUS(32, ");
               outputCVC(op->val[0], fp);
               fprintf(fp, ", ");
               outputCVC(op->val[1], fp);
               fprintf(fp, ")");
          } else if (op->opty == "sub") {
               fprintf(fp, "BVSUB(32, ");
               outputCVC(op->val[0], fp);
               fprintf(fp, ", ");
               outputCVC(op->val[1], fp);
               fprintf(fp, ")");
          } else if (op->opty == "imul") {
               fprintf(fp, "BVMULT(32, ");
               outputCVC(op->val[0], fp);
               fprintf(fp, ", ");
               outputCVC(op->val[1], fp);
               fprintf(fp, ")");
          } else if (op->opty == "xor") {
               fprintf(fp, "BVXOR(");
               outputCVC(op->val[0], fp);
               fprintf(fp, ", ");
               outputCVC(op->val[1], fp);
               fprintf(fp, ")");
          } else if (op->opty == "and") {
               outputCVC(op->val[0], fp);
               fprintf(fp, " & ");
               outputCVC(op->val[1], fp);
          } else if (op->opty == "or") {
               outputCVC(op->val[0], fp);
               fprintf(fp, " | ");
               outputCVC(op->val[1], fp);
          } else if (op->opty == "neg") {
               fprintf(fp, "~");
               outputCVC(op->val[0], fp);
          } else if (op->opty == "shl") {
               outputCVC(op->val[0], fp);
               fprintf(fp, " << ");
               outputCVC(op->val[1], fp);
          } else if (op->opty == "shr") {
               outputCVC(op->val[0], fp);
               fprintf(fp, " >> ");
               outputCVC(op->val[1], fp);
          } else {
               cout << "Instruction: " << op->opty << " is not interpreted in CVC!" << endl;
               return;
          }
     }
}

void outputCVCFormula(Value *f)
{
     const char *cvcfile = "formula.cvc";
     FILE *fp = fopen(cvcfile, "w");

     outputCVC(f, fp);

     fclose(fp);
}

// output a CVC formula to check the equivalence of f1 and f2 using the variable mapping in m
void outputChkEqCVC(Value *f1, Value *f2, map<int,int> *m)
{
     const char *cvcfile = "ChkEq.cvc";
     FILE *fp = fopen(cvcfile, "w");

     for (map<int,int>::iterator it = m->begin(); it != m->end(); ++it) {
          fprintf(fp, "sym%da: BV(32);\n", it->first);
          fprintf(fp, "sym%db: BV(32);\n", it->second);
     }
     fprintf(fp, "\n");

     for (map<int,int>::iterator it = m->begin(); it != m->end(); ++it) {
          fprintf(fp, "ASSERT(sym%da = sym%db);\n", it->first, it->second);
     }

     fprintf(fp, "\nQUERY(\n");
     sympostfix = "a";
     outputCVC(f1, fp);
     fprintf(fp, "\n=\n");
     sympostfix = "b";
     outputCVC(f2, fp);
     fprintf(fp, ");\n");
     fprintf(fp, "COUNTEREXAMPLE;\n");
}

// output all bit formulas based on the result of variable mapping
void outputBitCVC(Value *f1, Value *f2, vector<Value*> *inv1, vector<Value*> *inv2,
                            list<FullMap> *result)
{
     for (list<FullMap>::iterator it = result->begin(); it != result->end(); ++it) {
          int n = 1;
          string cvcfile = "formula" + to_string(n++) + ".cvc";
          FILE *fp = fopen(cvcfile.c_str(), "w");

          map<int,int> *inmap = &(it->first);
          map<int,int> *outmap = &(it->second);

          // output bit values of the inputs;
          for (int i = 0, max = 32 * inv1->size(); i < max; ++i) {
               fprintf(fp, "bit%da: BV(1);\n", i);
          }
          for (int i = 0, max = 32 * inv2->size(); i < max; ++i) {
               fprintf(fp, "bit%db: BV(1);\n", i);
          }

          // output inputs mapping
          for (map<int,int>::iterator it = inmap->begin(); it != inmap->end(); ++it) {
               fprintf(fp, "ASSERT(bit%da = bit%db);\n", it->first, it->second);
          }
          fprintf(fp, "\n");


          fprintf(fp, "\nQUERY(\n");

          // concatenate bits into a variable in the formula
          for (int i = 0, max = inv1->size(); i < max; ++i) {
               fprintf(fp, "LET %sa = ", getValueName((*inv1)[i]).c_str());
               for (int j = 0; j < 31; ++j) {
                    fprintf(fp, "bit%da@", i*32+j);
               }
               fprintf(fp, "bit%da IN (\n", i+31);
          }
          for (int i = 0, max = inv2->size(); i < max; ++i) {
               fprintf(fp, "LET %sb = ", getValueName((*inv2)[i]).c_str());
               for (int j = 0; j < 31; ++j) {
                    fprintf(fp, "bit%db@", i*32+j);
               }
               fprintf(fp, "bit%db IN (\n", i+31);
          }

          fprintf(fp, "LET out1 = ");
          sympostfix = "a";
          outputCVC(f1, fp);
          fprintf(fp, " IN (\n");
          fprintf(fp, "LET out2 = ");
          sympostfix = "b";
          outputCVC(f2, fp);
          fprintf(fp, " IN (\n");

          for (map<int,int>::iterator it = outmap->begin(); it != outmap->end(); ++it) {
               if (next(it,1) != outmap->end())
                    fprintf(fp, "out1[%d:%d] = out2[%d:%d] AND\n", it->first, it->first, it->second, it->second);
               else
                    fprintf(fp, "out1[%d:%d] = out2[%d:%d]\n", it->first, it->first, it->second, it->second);
          }

          for (int i = 0, n = inv1->size() + inv2->size(); i < n; ++i) {
               fprintf(fp, ")");
          }
          fprintf(fp, ")));\n");
          fprintf(fp, "COUNTEREXAMPLE;");

          fclose(fp);
     }
}
