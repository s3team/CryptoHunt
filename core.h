struct Operand {
     enum OprTy {ImmValue, Reg, Mem};
     OprTy ty;
     int tag;
     int bit;
     bool issegaddr;
     string segreg;             // for seg mem access like fs:[0x1]
     string field[5];

     Operand() : bit(0),issegaddr(false) {}
};

struct Inst {
     int id;
     string addr;
     unsigned int addrn;
     string assembly;
     int opc;
     string opcstr;
     vector<string> oprs;
     int oprnum;
     Operand *oprd[3];
     uint32_t ctxreg[8];
     uint32_t memaddr;

     uint32_t getRegVal(string reg);
};

typedef pair< map<int,int>, map<int,int> > FullMap;
