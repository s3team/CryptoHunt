#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <map>
#include <stack>
#include <vector>
#include <set>

using namespace std;

#include "core.h"

list<Inst> instlist;

// Data structures for identify functions
struct FuncBody {
     int start;
     int end;
     int length;                // end - start
     unsigned int startAddr;
     unsigned int endAddr;
     int loopn;
};

struct Func {
     unsigned int callAddr;              // same as the startAddr in FuncBody
     list<struct FuncBody *> body;
};

// Data structures for loop identification
struct LoopBody {
     bool good;
     list<Inst>::iterator begin;
     list<Inst>::iterator end;
};

struct Loop {
     unsigned int startaddr;
     list<LoopBody> loopbody;
     vector<LoopBody> instance;
};

struct LoopSeq {           // continuous loop body
     int id;
     list<LoopBody> loopbds;
};

// jmp instruction names in x86 assembly
string jmpInstrName[33] = {"jo","jno","js","jns","je","jz","jne",
                           "jnz","jb","jnae","jc","jnb","jae",
                           "jnc","jbe","jna","ja","jnbe","jl",
                           "jnge","jge","jnl","jle","jng","jg",
                           "jnle","jp","jpe","jnp","jpo","jcxz",
                           "jecxz", "jmp"};


set<int> *jmpset;        // jmp instructions
map<string, int> *instenum;     // instruction enumerations

string getOpcName(int opc, map<string, int> *m)
{
     for (map<string, int>::iterator it = m->begin(); it != m->end(); ++it) {
          if (it->second == opc)
               return it->first;
     }

     return "unknown";
}

void printInstlist(list<Inst> *L, map<string, int> *m)
{
     for (list<Inst>::iterator it = L->begin(); it != L->end(); ++it) {
          cout << it->id << ' ';
          cout << hex << it->addrn << ' ';
          cout << it->addr << ' ';
          cout << it->opcstr << ' ';
          cout << getOpcName(it->opc, m) << ' ';
          cout << it->oprnum << endl;
          for (vector<string>::iterator ii = it->oprs.begin(); ii != it->oprs.end(); ++ii) {
               cout << *ii << endl;
          }
     }
}

void printLoop(list<Inst>::iterator begin, list<Inst>::iterator end, map<string, int> *m)
{
     list<Inst>::iterator it, endnext = next(end,1);
     cout << "loop body begins:" << endl;
     for (it = begin; it != endnext; ++it) {
          cout << dec << it->id << ' ';
          cout << hex << it->addrn << ' ';
          cout << getOpcName(it->opc, m) << ' ';
          cout << it->oprnum << endl;
          for (vector<string>::iterator ii = it->oprs.begin(); ii != it->oprs.end(); ++ii) {
               cout << *ii << " ";
          }
          cout << endl;
     }
     cout << "loop body ends." << endl;
     cout << endl;
}


map<unsigned int, list<FuncBody *> *> *
buildFuncList(list<Inst> *L)
{
     map<unsigned int, list<FuncBody *> *> *funcmap =
          new map<unsigned int, list<FuncBody *> *>;
     // list<Func> *funclist = new list<Func>;
     stack<list<Inst>::iterator> stk;


     for (list<Inst>::iterator it = L->begin(); it != L->end(); ++it) {
          // parse the whole instlist to build funclist

          if (it->opcstr == "call") {
               stk.push(it);
               // search whether the function is in the function list
               // if yes, identify whether it is a new function instance
               // if not, create a new function
               map<unsigned int, list<FuncBody *> *>::iterator i = funcmap->find(it->addrn);
               if (i == funcmap->end()) {
                    unsigned int calladdr = stoul(it->oprs[0], nullptr, 16);
                    funcmap->insert(pair<unsigned int, list<FuncBody *> *>(calladdr, NULL));
               }
          } else if (it->opcstr == "ret") {
               if (!stk.empty()) stk.pop();
          } else {}
     }

     return funcmap;
}

void printFuncmap(map<unsigned int, list<FuncBody *> *> *funcmap)
{
     map<unsigned int, list<FuncBody *> *>::iterator it;
     for (it = funcmap->begin(); it != funcmap->end(); ++it) {
          cout << hex << it->first << endl;
     }
}

map<string, int> *buildOpcodeMap(list<Inst> *L)
{
     map<string, int> *instenum = new map<string, int>;
     for (list<Inst>::iterator it = L->begin(); it != L->end(); ++it) {
          if (instenum->find(it->opcstr) == instenum->end())
               instenum->insert(pair<string, int>(it->opcstr, instenum->size()+1));
     }

     return instenum;
}

int getOpc(string s, map<string, int> *m)
{
     map<string, int>::iterator it = m->find(s);
     if (it != m->end())
          return it->second;
     else
          return 0;
}

bool isjump(int i, set<int> *jumpset)
{
     set<int>::iterator it = jumpset->find(i);
     if (it == jumpset->end())
          return false;
     else
          return true;
}


void countindjumps(list<Inst> *L) {
     int indjumpnum = 0;
     for (list<Inst>::iterator it = L->begin(); it != L->end(); ++it) {
          if (isjump(it->opc, jmpset) && it->oprd[0]->ty != Operand::ImmValue) {
               ++indjumpnum;
               cout << it->addr << "\t" << it->opcstr << " " << it->oprs[0] << endl;
          }
     }
     cout << "number of indirect jumps: " << indjumpnum << endl;
}

void findLoopSeq(list<Loop> *loops)
{
     list<LoopSeq> loopins;
     int loopinsid = 0;
     for (list<Loop>::iterator it = loops->begin(); it != loops->end(); ++it) {
          list<LoopBody>::iterator ii, iin;
          if (!it->loopbody.empty()) {
               LoopSeq lpins;
               lpins.id = ++loopinsid;
               cout << "first loop instance: " << loopinsid << endl;
               lpins.loopbds.push_back(it->loopbody.front());
               loopins.push_back(lpins);
          }
          for (ii = it->loopbody.begin(); (iin = next(ii,1)) != it->loopbody.end(); ++ii) {
               if (ii->good == true && iin->good == true){
                    if (iin->begin->id != ii->end->id + 1) {
                         LoopSeq newlpins;
                         newlpins.id = ++loopinsid;
                         cout << "new loop instance: " << loopinsid << endl;
                         newlpins.loopbds.push_back(*ii);
                         loopins.push_back(newlpins);
                    }
               }
          }
     }

     for (list<LoopSeq>::iterator it = loopins.begin(); it != loopins.end(); ++it) {
          cout << it->id << ": " << endl;
          //cout << it->loopbds.front().begin->addr << endl;
     }
}

bool isLoopBodyEq(LoopBody lp1, LoopBody lp2)
{
     list<Inst>::iterator it1 = lp1.begin;
     list<Inst>::iterator it2 = lp2.begin;

     while (it1 != lp1.end && it2 != lp2.end && (it1++)->opc == (it2++)->opc) ;

     if ((it1 == lp1.end) && (it2 == lp2.end))
          return true;
     else
          return false;
}

void printLoopBody(LoopBody lpbd)
{
     for (list<Inst>::iterator it = lpbd.begin; it != lpbd.end; ++it) {
          cout << it->addr << " " << it->opcstr << endl;
     }
     cout << endl;
}

void outputLoopInstance(list<Loop> *loops)
{
     int n = 1;
     for (list<Loop>::iterator it = loops->begin(); it != loops->end(); ++it) {
          for (int i = 0, max = it->instance.size(); i < max; ++i) {
               string loopfile = "loop" + to_string(n++) + ".txt";
               FILE *fp = fopen(loopfile.c_str(), "w");

               for (list<Inst>::iterator ii = it->instance[i].begin; ii != it->instance[i].end; ++ii) {
                    fprintf(fp, "%s;%s;", ii->addr.c_str(), ii->assembly.c_str());
                    for (int j = 0; j < 8; ++j) {
                         fprintf(fp, "%x,", ii->ctxreg[j]);
                    }
                    fprintf(fp, "%x,\n", ii->memaddr);
               }

               fclose(fp);
          }
     }
}

void loopdetect1(list<Inst> *L)
{
     // Loop detection
     int nloop = 0;
     // set<pair<unsigned int, unsigned int> > loops;
     list<Loop> loops;
     for (list<Inst>::iterator it = L->begin(); it != L->end(); ++it) {
          if (isjump(it->opc, jmpset)) {
               unsigned int targetaddr = stoul(it->oprs[0], 0, 16);
               auto ni = next(it, 1);
               if (targetaddr < it->addrn && it->addrn-targetaddr < 0xffff && ni->addrn == targetaddr) {
                    // pair<unsigned int, unsigned int> p;
                    // p.first = targetaddr;
                    // p.second = it->addrn;
                    LoopBody bd;
                    bd.end = it;

                    list<Loop>::iterator ii;
                    for (ii = loops.begin(); ii != loops.end(); ++ii) {
                         if (ii->startaddr == targetaddr) break;
                    }
                    if (ii == loops.end()) { // A new loop
                         Loop lp;
                         lp.startaddr = targetaddr;
                         lp.loopbody.push_back(bd);
                         loops.push_back(lp);
                    } else {    // The loop aready exists. Add a new loop body.
                         ii->loopbody.push_back(bd);
                    }
                    ++nloop;
               }
          }
     }

     // a loop body is good, if its size is less than 0xffff
     for (list<Loop>::iterator it = loops.begin(); it != loops.end(); ++it) {
          for (list<LoopBody>::iterator ii = it->loopbody.begin(); ii != it->loopbody.end(); ++ii) {
               int n = 0;
               for (list<Inst>::iterator i = ii->end; n < 0xffff; --i, ++n) {
                    if (i->addrn == it->startaddr) {
                         ii->begin = i;
                         ii->good = true;
                         break;
                    }
               }
               if (n == 0xffff) {
                    ii -> good = false;
                    // cout << "no begin address found!" << endl;
                    // cout << "id: " << dec << ii->end->id;
                    // cout << ", end address: " << hex << ii->end->addrn << endl;
               }
          }
     }

     cout << "remove all bad loop bodies." << endl;
     // remove all bad loop bodies
     for (list<Loop>::iterator it = loops.begin(); it != loops.end(); ++it) {
          for (list<LoopBody>::iterator ii = it->loopbody.begin(); ii != it->loopbody.end();) {
               if (ii->good == false) {
                    ii = it->loopbody.erase(ii);
               } else {
                    ++ii;
               }
          }
     }

     cout << "remove loops that have no loop body. " << endl;
     // remove loops that have no loop body
     for (list<Loop>::iterator it = loops.begin(); it != loops.end();) {
          if (it->loopbody.size() == 0) {
               it = loops.erase(it);
          } else {
               ++it;
          }
     }

     int goodbodies = 0;
     for (list<Loop>::iterator it = loops.begin(); it != loops.end(); ++it) {
          for (list<LoopBody>::iterator ii = it->loopbody.begin(); ii != it->loopbody.end(); ++ii) {
               if (ii->good == true) {
                    ++goodbodies;
                    //printLoop(ii->begin, ii->end, instenum);
               }
          }
     }


     // cout << "nloop = " << nloop << endl;
     cout << "num of loops: " << loops.size() << endl;
     // cout << "num of goodbodies = " << goodbodies << endl;


     // remove repeated loop bodies. Create loop instance list
     for (list<Loop>::iterator it = loops.begin(); it != loops.end(); ++it) {
          LoopBody b = it->loopbody.front();
          it->instance.push_back(b);
          for (list<LoopBody>::iterator ii = next(it->loopbody.begin(), 1); ii != it->loopbody.end(); ++ii) {
               int i, max;
               for (i = 0, max = it->instance.size(); i < max; ++i) {
                    if (isLoopBodyEq(it->instance[i], *ii)) {
                         break;
                    }
               }
               if (i == max) {
                    it->instance.push_back(*ii);
               }
          }
     }

     // print loop information
     for (list<Loop>::iterator it = loops.begin(); it != loops.end(); ++it) {
          cout << " loop body nums: " << dec << it->loopbody.size() << endl;
          cout << " loop instance nums: " << it->instance.size() << endl;
          // for (int i = 0, max = it->instance.size(); i < max; ++i) {
          //      printLoopBody(it->instance[i]);
          // }
     }

     outputLoopInstance(&loops);
}


// A naive loop detection for unrolled loops
void loopdetect2(list<Inst> *L)
{
     int loopnum = 0;
     for (int step = 2; step < 66; ++step) {
          for (int start = 0; start < 7000; ++start) {
               list<Inst>::iterator it1 = L->begin();
               for (int i = 0; i < start && it1 != L->end(); ++i) ++it1;
               list<Inst>::iterator it0 = it1;
               list<Inst>::iterator it2 = it1;
               for (int i = 0; i < step && it2 != L->end(); ++i) ++it2;
               list<Inst>::iterator it3 = it2;
               while ((it1->opc == it3->opc) && (it1 != it2) && (it3 != L->end())) { ++it1; ++it3; }
               if (it1 == it2) {
                    cout << "step: " << step << endl;
                    cout << "line: " << it0->id << endl;
                    cout << "address: " << it0->addr << endl;
                    cout << endl;
                    ++loopnum;
               }
          }
     }
     cout << "loop num: " << loopnum << endl;
}

void preprocess(list<Inst> *L)
{
     // build global instruction enum based on the instlist
     instenum = buildOpcodeMap(L);

     // update opc field in L
     for (list<Inst>::iterator it = L->begin(); it != L->end(); ++it) {
          it->opc = getOpc(it->opcstr, instenum);
     }

     // create a set containing the opcodes of all jump instructions
     jmpset = new set<int>;
     for (string &s : jmpInstrName) {
          int n;
          if ((n = getOpc(s, instenum)) != 0) {
               jmpset->insert(n);
          }
     }
}

// parse the whole trace into a instruction list L
void parseTrace(ifstream *infile, list<Inst> *L)
{
     string line;
     int num = 1;

     while (infile->good()) {
          getline(*infile, line);
          if (line.empty()) { continue; }

          istringstream strbuf(line);
          string temp, disasstr;

          Inst *ins = new Inst();
          ins->id = num++;

          // get the instruction address
          getline(strbuf, ins->addr, ';');
          ins->addrn = stoul(ins->addr, 0, 16);

          // get the disassemble string
          getline(strbuf, disasstr, ';');
          ins->assembly = disasstr;

          istringstream disasbuf(disasstr);
          getline(disasbuf, ins->opcstr, ' ');
          // ins->opc = getOpcode(ins->opcstr);

          while (disasbuf.good()) {
               getline(disasbuf, temp, ',');
               if (temp.find_first_not_of(' ') != string::npos)
                    ins->oprs.push_back(temp);
          }
          ins->oprnum = ins->oprs.size();

          // parse 8 context reg values
          for (int i = 0; i < 8; ++i) {
               getline(strbuf, temp, ',');
               ins->ctxreg[i] = stoul(temp, 0, 16);
          }

          // parse memory access address
          getline(strbuf, temp, ',');
          ins->memaddr = stoul(temp, 0, 16);

          L->push_back(*ins);
     }
}

int main(int argc, char **argv) {
     if (argc != 2) {
          fprintf(stderr, "usage: %s <tracefile>\n", argv[0]);
          return 1;
     }

     ifstream infile(argv[1]);
     if (!infile.is_open()) {
          fprintf(stderr, "Open file error!\n");
          return 1;
     }

     parseTrace(&infile, &instlist);

     infile.close();

     preprocess(&instlist);

     loopdetect1(&instlist);

     return 0;
}
