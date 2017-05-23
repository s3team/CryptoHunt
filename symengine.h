struct Operation;
struct Value;

// Symbolic execution engine
class SEEngine {
private:
     // Value *eax, *ebx, *ecx, *edx, *esi, *edi, *esp, *ebp;
     map<string, Value*> ctx;
     list<Inst>::iterator start;
     list<Inst>::iterator end;
     map<uint32_t, Value*> mem;

     bool memfind(uint32_t addr) {
          map<uint32_t, Value*>::iterator ii = mem.find(addr);
          if (ii == mem.end())
               return false;
          else
               return true;
     }


public:
     SEEngine() {
          ctx = { {"eax", NULL}, {"ebx", NULL}, {"ecx", NULL}, {"edx", NULL},
                  {"esi", NULL}, {"edi", NULL}, {"esp", NULL}, {"ebp", NULL}
          };
     };
     void init(Value *v1, Value *v2, Value *v3, Value *v4,
               Value *v5, Value *v6, Value *v7, Value *v8,
               list<Inst>::iterator it1,
               list<Inst>::iterator it2);
     void init(list<Inst>::iterator it1,
               list<Inst>::iterator it2);
     void initAllRegSymol(list<Inst>::iterator it1,
                          list<Inst>::iterator it2);
     int symexec();
     uint32_t conexec(Value *f, map<Value*, uint32_t> *input);
     void outputFormula(string reg);
     void printAllRegFormulas();
     void printMemFormula();
     void printInputSymbols(string output);
     Value *getValue(string s) { return ctx[s]; }
     vector<Value*> getAllOutput();
};

void outputCVCFormula(Value *f);
void outputChkEqCVC(Value *f1, Value *f2, map<int,int> *m);
void outputBitCVC(Value *f1, Value *f2, vector<Value*> *inv1, vector<Value*> *inv2,
                  list<FullMap> *result);
map<Value*, uint32_t> buildinmap(vector<Value*> *vv, vector<uint32_t> *input);
vector<Value*> getInputVector(Value *f); // get formula f's inputs as a vector
string getValueName(Value *v);
