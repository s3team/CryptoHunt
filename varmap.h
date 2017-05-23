struct BitMatrix;

typedef pair< set<int>,set<int> > PartMap;

void varmapAndoutputCVC(SEEngine *se1, Value *v1, SEEngine *se2, Value *v2);

void printVar(map<Value*, uint32_t> *varm);
void printBV(vector<bool> *bv);
void printMapInt(map<int,int> *m);
vector<bool> var2bv(map<Value*, uint32_t> *varm, vector<Value*> *vv);
map<Value*, uint32_t> bv2var(vector<bool> bv, vector<Value*> *vv);
int setOutMatrix(BitMatrix *im, Value *f, vector<Value*> *iv, vector<Value*> *ov,
                    SEEngine *se, BitMatrix *om);
