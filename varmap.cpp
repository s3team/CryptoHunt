#include <iostream>
#include <list>
#include <set>
#include <map>
#include <bitset>
#include <vector>
#include <algorithm>
#include <time.h>

using namespace std;

#include "core.h"
#include "symengine.h"
#include "varmap.h"

// the index's bit of a symbol
struct BitValue {
     Value *sym;
     int idx;                     // index of the bit vector: 1-32

     BitValue() { sym = NULL; idx = 0; }
     BitValue(Value *v, int n);
};

BitValue::BitValue(Value *v, int n)
{
     sym = v;
     idx = n;
}


struct BitMatrix {
     vector< vector<bool> > m;

     BitMatrix() {}
     BitMatrix(vector< vector<bool> > mx);
     BitMatrix(int row, int col);
     void initIdentityMatrix(int n);
     void show();
     vector<int> getRowVector();
     vector<int> getColVector();
     void randomizeAll();
     void setCol(int col, bool b);
};

BitMatrix::BitMatrix(vector< vector<bool> > mx)
{
     m = mx;
}

BitMatrix::BitMatrix(int row, int col)
{
     m.resize(row);
     for (int i = 0, rown = m.size(); i < rown; ++i) {
          m[i].resize(col);
     }
}

// Initialize a n dimension identity matrix
void BitMatrix::initIdentityMatrix(int n)
{
     vector<bool> temp(n);

     for (int i = 0; i < n; ++i) {
          for (int j = 0; j < n; ++j) {
               if (j == i)
                    temp[j] = 1;
               else
                    temp[j] = 0;
          }
          m.push_back(temp);
     }
}

void BitMatrix::show()
{
     for (int i = 0, nrow = m.size(); i < nrow; ++i) {
          for (int j = 0, ncol = m[i].size(); j < ncol; ++j) {
               cout << m[i][j] << " ";
          }
          cout << endl;
     }
}

vector<int> BitMatrix::getRowVector()
{
     vector<int> rv;

     for (int i = 0, nrow = m.size(); i < nrow; ++i) {
          int rn = 0;
          for (int j = 0, ncol = m[i].size(); j < ncol; ++j) {
               if (m[i][j] == 1) ++rn;
          }
          rv.push_back(rn);
     }

     return rv;
}

vector<int> BitMatrix::getColVector()
{
     vector<int> cv;

     for (int j = 0, ncol = m[0].size(); j < ncol; ++j) {
          int cn = 0;
          for (int i = 0, nrow = m.size(); i < nrow; ++i) {
               if (m[i][j] == 1) ++cn;
          }
          cv.push_back(cn);
     }

     return cv;
}

void BitMatrix::randomizeAll()
{
     srand(time(NULL));
     for (int i = 0, nrow = m.size(); i < nrow; ++i) {
          for (int j = 0, ncol = m[i].size(); j < ncol; ++j) {
               m[i][j] = rand() % 2;
          }
     }

}


// set all elements in the column col to boolean value b
void BitMatrix::setCol(int col, bool b)
{
     for (int i = 0, nrow = m.size(); i < nrow; ++i) {
          m[i][col] = b;
     }
}

// set mapped variables in input matrix im1 and im2 to the same random boolean value
void randomizeMappedVar(BitMatrix *im1, BitMatrix *im2, map<int,int> *mappedvar)
{
     srand(time(NULL));

     for (map<int,int>::iterator it = mappedvar->begin(); it != mappedvar->end(); ++it) {
          int c1 = it->first;
          int c2 = it->second;
          bool r = rand() % 2;

          // set column c1 in im1 and c2 in im2 to random number r
          im1->setCol(c1, r);
          im2->setCol(c2, r);
     }
}

// set mapped variables in input matrix im1 and im2 as identity matrix
void setIdentityMatrix(BitMatrix *im1, BitMatrix *im2, map<int,int> *mappedvar)
{
     set<int> mvset1;           // mapped variables in im1
     set<int> mvset2;           // mapped variables in im2

     for (map<int,int>::iterator it = mappedvar->begin(); it != mappedvar->end(); ++it) {
          mvset1.insert(it->first);
          mvset2.insert(it->second);
     }

     // im1 and im2 should have the same row and col size, do the checking
     // before call this function!
     int vnum = im1->m[0].size();
     vector<int> umv1, umv2;    // unmapped variables in im1 and im2

     for (int i = 0; i < vnum; ++i) {
          if (mvset1.find(i) == mvset1.end()) {
               umv1.push_back(i);
          }
          if (mvset2.find(i) == mvset2.end()) {
               umv2.push_back(i);
          }
     }

     // set unmapped variables to identity matrix
     for (int i = 0, nrow = umv1.size(); i < nrow; ++i) {
          for (int j = 0, ncol = umv1.size(); j < ncol; ++j) {
               if (i == j)
                    im1->m[i][umv1[j]] = 1;
               else
                    im1->m[i][umv1[j]] = 0;

               if (i == j)
                    im2->m[i][umv2[j]] = 1;
               else
                    im2->m[i][umv2[j]] = 0;
          }
     }
}

// set the output matrix om based on the input matrix im
// each row in om is the output of f
int setOutMatrix(BitMatrix *im, Value *f, vector<Value*> *iv, vector<Value*> *ov,
                    SEEngine *se, BitMatrix *om)
{
     for (int i = 0, nrow = im->m.size(); i < nrow; ++i) {
          map<Value*, uint32_t> input = bv2var(im->m[i], iv);
          uint32_t output = se->conexec(f, &input);
          // cout << i << ": ";
          // cout << hex << output << endl;
          map<Value*, uint32_t> outmap = {{(*ov)[0], output}};
          vector<bool> outbv = var2bv(&outmap, ov);
          om->m[i] = outbv;
     }
     // cout << dec << endl;

     return 1;
}

// convert a bit vector to a map from symbol to its concrete value
map<Value*, uint32_t> bv2var(vector<bool> bv, vector<Value*> *vv)
{
     map<Value*, uint32_t> varm;
     if (bv.size() != 32 * vv->size()) {
          cout << "bv2var: bv and vv are not consistent" << endl;
          return varm;
     }

     bitset<32> bs;
     for (int i = 0, max = vv->size(); i < max; ++i) {
          for (int j = 0; j < 32; ++j) {
               bs[j] = bv[i*32+j];
          }
          varm.insert( pair<Value*, uint32_t>((*vv)[i], bs.to_ulong()) );
     }

     return varm;
}

// convert a symbol map to a bit vector
vector<bool> var2bv(map<Value*, uint32_t> *varm, vector<Value*> *vv)
{
     vector<bool> bv;

     if (varm->size() != vv->size()) {
          cout << "var2bv: varm and vv are not consistent!" << endl;
          return bv;
     }

     for (int i = 0, max = vv->size(); i < max; ++i) {
          bitset<32> bs((*varm)[(*vv)[i]]);
          for (int j = 0; j < 32; ++j) {
               bv.push_back(bs[j]);
          }
     }

     return bv;
}

// check whether a partial mapping is consistent
bool checkConsist(vector<PartMap> *m)
{
     int i, mlen = m->size();

     if (mlen == 0) {
          return true;
     } else if (mlen == 1) {
          if ((*m)[0].first.size() != (*m)[0].second.size()) {
               return false;
          } else {
               return true;
          }
     } else {
          for (i = 0; i < mlen-1; ++i) {
               if ((*m)[i].first.size() != (*m)[i].second.size()) {
                    return false;
               }
               for (int j = i + 1; j < mlen; ++j) {
                    vector<int> interset1, interset2;

                    set_intersection((*m)[i].first.begin(), (*m)[i].first.end(),
                                     (*m)[j].first.begin(), (*m)[j].first.end(),
                                     back_inserter(interset1));
                    set_intersection((*m)[i].second.begin(), (*m)[i].second.end(),
                                     (*m)[j].second.begin(), (*m)[j].second.end(),
                                     back_inserter(interset2));

                    if (interset1.size() != interset2.size()) {
                         return false;
                    }
               }
          }

          // check the last element
          if ((*m)[mlen-1].first.size() != (*m)[mlen-1].second.size()) {
               return false;
          }
     }

     return true;
}

// reduce a partial mapping m by set intersection, return a map containing all mapped variables
// after reduce, no empty PartMap or one to one PartMap in m
map<int,int> reduce(vector<PartMap> *m)
{
     map<int,int> mvar;         // mapped variables identified in this reduce

     // erase empty PartMap in m
     for (int i = 0, max = m->size(); i < max;) {
          if ((*m)[i].first.empty()) { // second is also empty when first is empty
               m->erase(m->begin() + i);
               max = m->size();
          } else {
               ++i;
          }
     }

     for (int i = 0, mlen = m->size(); i < mlen-1;) {
          for (int j = i + 1; j < mlen;) {
               set<int> interset1, interset2;

               set_intersection((*m)[i].first.begin(), (*m)[i].first.end(),
                                (*m)[j].first.begin(), (*m)[j].first.end(),
                                inserter(interset1, interset1.begin()));
               set_intersection((*m)[i].second.begin(), (*m)[i].second.end(),
                                (*m)[j].second.begin(), (*m)[j].second.end(),
                                inserter(interset2, interset2.begin()));

               if (interset1.size() != 0 && interset2.size() != 0) {

                    // remove the common elements in m[i] and m[j]
                    for (set<int>::iterator iter = interset1.begin(); iter != interset1.end(); ++iter) {
                         (*m)[i].first.erase(*iter);
                         (*m)[j].first.erase(*iter);
                    }
                    for (set<int>::iterator iter = interset2.begin(); iter != interset2.end(); ++iter) {
                         (*m)[i].second.erase(*iter);
                         (*m)[j].second.erase(*iter);
                    }

                    // push interset1 and interset2 m
                    PartMap pm(interset1, interset2);
                    m->push_back(pm);

                    // erase empty PartMap in m
                    for (int i = 0, max = m->size(); i < max;) {
                         if ((*m)[i].first.empty()) { // second is also empty when first is empty
                              m->erase(m->begin() + i);
                              max = m->size();
                         } else {
                              ++i;
                         }
                    }

                    // remove mapped variable from m and push it to mvar
                    for (int i = 0, max = m->size(); i < max;) {
                         if ((*m)[i].first.size() == 1) { // second size is also 1 when first is 1

                              set<int>::iterator it1 = (*m)[i].first.begin();
                              set<int>::iterator it2 = (*m)[i].second.begin();

                              int mv1 = *it1;
                              int mv2 = *it2;

                              mvar.insert(pair<int,int>(*it1, *it2));
                              m->erase(m->begin() + i);
                              max = m->size();

                              // remove the mapped variable from m
                              for (int ii = 0; ii < max; ++ii) {
                                   set<int>::iterator it;
                                   if ((it = (*m)[ii].first.find(mv1)) != (*m)[ii].first.end())
                                        (*m)[ii].first.erase(it);
                                   if ((it = (*m)[ii].second.find(mv2)) != (*m)[ii].second.end())
                                        (*m)[ii].second.erase(it);
                              }

                         } else {
                              ++i;
                         }

                    }

                    // reset the iterator
                    i = 0;
                    j = 1;
                    mlen = m->size();
                    continue;
               } else {
                    ++j;
               }
          }
          ++i;                  // put ++i ++j here so that continue can go around them
     }

     // erase empty PartMap in m
     for (int i = 0, max = m->size(); i < max;) {
          if ((*m)[i].first.empty()) { // second is also empty when first is empty
               m->erase(m->begin() + i);
               max = m->size();
          } else {
               ++i;
          }
     }

     return mvar;
}


// print all variables and their concrete value
void printVar(map<Value*, uint32_t> *varm)
{
     for (map<Value*, uint32_t>::iterator it = varm->begin(); it != varm->end(); ++it) {
          cout << getValueName(it->first) << ": ";
          cout << hex << it->second;
          cout << dec << endl;
     }
}


// print a bit vector
void printBV(vector<bool> *bv)
{
     for (int i = 0, max = bv->size(); i < max; ++i) {
          cout << (*bv)[i] << " ";
          if (i == 31) cout << endl;
     }
}


// print a int vector. It is usually used to print a col vector or row vector
void printVecInt(vector<int> *v)
{
     for (vector<int>::iterator it = v->begin(); it != v->end(); ++it) {
          cout << *it << " ";
     }
     cout << endl;
}


// print a partial maping from a set of int to a set of int
void printPartialMapping(vector<PartMap> *unmap)
{
     for (int i = 0, max = unmap->size(); i < max; ++i) {
          cout << "{";
          for (set<int>::iterator it1 = (*unmap)[i].first.begin(); it1 != (*unmap)[i].first.end(); ++it1)
               cout << *it1 << ",";
          cout << "} -> {";
          for (set<int>::iterator it2 = (*unmap)[i].second.begin(); it2 != (*unmap)[i].second.end(); ++it2)
               cout << *it2 << ",";
          cout << "}" << endl;
     }
}

void printMapInt(map<int,int> *m)
{
     for (map<int,int>::iterator it = m->begin(); it != m->end(); ++it) {
          cout << it->first << " -> " << it->second << endl;
     }
}

// update the partial mapping list by compare two cv or rv
void updateUnmap(vector<PartMap> *unmap, vector<int> *vec1, vector<int> *vec2, map<int,int> *m)
{
     set<int> mvset1;
     set<int> mvset2;

     for (map<int,int>::iterator it = m->begin(); it != m->end(); ++it) {
          mvset1.insert(it->first);
          mvset2.insert(it->second);
     }

     int vnum = vec1->size();
     list<int> umv1, umv2;    // unmapped variables in im1 and im2

     for (int i = 0; i < vnum; ++i) {
          if (mvset1.find(i) == mvset1.end()) {
               umv1.push_back(i);
          }
          if (mvset2.find(i) == mvset2.end()) {
               umv2.push_back(i);
          }
     }

     // in each iteration, size of umv1 and umv2 should reduce the same value
     while (umv1.size() != 0) {
          int head = umv1.front();
          set<int> s1 = {head}, s2;
          int n = (*vec1)[head]; // value in the row or col vector

          for (list<int>::iterator it = umv1.begin(); it != umv1.end(); ++it) {
               if ((*vec1)[*it] == n) {
                    s1.insert(*it);
               }
          }
          for (list<int>::iterator it = umv1.begin(); it != umv1.end();) { // erase all partial mapped vars in s1
               if (s1.find(*it) != s1.end()) {
                    it = umv1.erase(it);
               } else {
                    ++it;
               }
          }

          for (list<int>::iterator it = umv2.begin(); it != umv2.end(); ++it) {
               if ((*vec2)[*it] == n) {
                    s2.insert(*it);
               }
          }
          for (list<int>::iterator it = umv2.begin(); it != umv2.end();) { // erase all partial mapped vars in s2
               if (s2.find(*it) != s2.end()) {
                    it = umv2.erase(it);
               } else {
                    ++it;
               }
          }

          PartMap pm(s1, s2);
          unmap->push_back(pm);
     }
}

bool sortpmsize(PartMap pm1, PartMap pm2)
{
     return (pm1.first.size() < pm2.first.size());
}

// Generate a list containing all variable mappings
void varmap(Value *f1, Value *f2, SEEngine *se1, SEEngine *se2,
            vector<Value*> *inv1, vector<Value*> *inv2,
            vector<PartMap> unmapin, vector<PartMap> unmapout,
            map<int,int> inmap, map<int,int> outmap,
            list<FullMap> *result)
{

     if (unmapin.size() == 0 && unmapout.size() == 0) {      // all variables are mapped
          FullMap fm(inmap, outmap);
          result->push_back(fm);

          return;
     }

     // number of variables in f1 and f2
     int ninv1 = inv1->size();
     int ninv2 = inv2->size();

     int ninmap = inmap.size();

     BitMatrix im1(32*ninv1 - ninmap, 32*ninv1), im2(32*ninv2 - ninmap, 32*ninv2);
     randomizeMappedVar(&im1, &im2, &inmap);
     setIdentityMatrix(&im1, &im2, &inmap);


     BitMatrix om1(32*ninv1 - ninmap,32), om2(32*ninv2 - ninmap,32);
     vector<Value*> outv1 = {f1};
     vector<Value*> outv2 = {f2};

     setOutMatrix(&im1, f1, inv1, &outv1, se1, &om1);

     setOutMatrix(&im2, f2, inv2, &outv2, se2, &om2);

     vector<int> rv1 = om1.getRowVector();
     vector<int> cv1 = om1.getColVector();
     vector<int> rv2 = om2.getRowVector();
     vector<int> cv2 = om2.getColVector();

     vector<int> rv1sort = rv1;
     sort(rv1sort.begin(), rv1sort.end());
     vector<int> cv1sort = cv1;
     sort(cv1sort.begin(), cv1sort.end());

     vector<int> rv2sort = rv2;
     sort(rv2sort.begin(), rv2sort.end());
     vector<int> cv2sort = cv2;
     sort(cv2sort.begin(), cv2sort.end());

     if (rv1sort != rv2sort || cv1sort != cv2sort)
          return;


     updateUnmap(&unmapin, &rv1, &rv2, &inmap);
     updateUnmap(&unmapout, &cv1, &cv2, &outmap);


     if (!checkConsist(&unmapin))
          return;
     if (!checkConsist(&unmapout))
          return;

     map<int,int> newmapin = reduce(&unmapin);
     map<int,int> newmapout = reduce(&unmapout);


     sort(unmapin.begin(), unmapin.end(), sortpmsize);
     sort(unmapout.begin(), unmapout.end(), sortpmsize);


     // update inmap and outmap using newmapin and newmapout
     for (map<int,int>::iterator it = newmapin.begin(); it != newmapin.end(); ++it) {
          inmap[it->first] = it->second;
     }
     for (map<int,int>::iterator it = newmapout.begin(); it != newmapout.end(); ++it) {
          outmap[it->first] = it->second;
     }


     if (unmapin.size() == 0 && unmapout.size() == 0) {
          FullMap fm(inmap, outmap);
          result->push_back(fm);

          return;
     } else if (unmapin.size() != 0) {
          PartMap p = unmapin[0];
          set<int>::iterator i1 = p.first.begin();
          for (set<int>::iterator i2 = p.second.begin(); i2 != p.second.end(); ++i2) {
               set<int> s1 = {*i1};
               set<int> s2 = {*i2};
               PartMap pm(s1, s2);
               unmapin.push_back(pm);
               varmap(f1, f2, se1, se2, inv1, inv2, unmapin, unmapout, inmap, outmap, result);
          }
     } else {
          PartMap p = unmapout[0];
          set<int>::iterator i1 = p.first.begin();
          for (set<int>::iterator i2 = p.second.begin(); i2 != p.second.end(); ++i2) {
               set<int> s1 = {*i1};
               set<int> s2 = {*i2};
               PartMap pm(s1, s2);
               unmapout.push_back(pm);
               varmap(f1, f2, se1, se2, inv1, inv2, unmapin, unmapout, inmap, outmap, result);
          }
     }
}

void varmapAndoutputCVC(SEEngine *se1, Value *v1, SEEngine *se2, Value *v2)
{
     vector<Value*> inv1 = getInputVector(v1);
     vector<Value*> inv2 = getInputVector(v2);

     // skip variable mapping when the inputs have different number of bits
     if (inv1.size() != inv2.size()) {
          cout << "no mapping found" << endl;
          return;
     }

     // initialize the paritial mapping in/out set
     set<int> inset1, inset2;
     for (int i = 0, max = inv1.size(); i < max; ++i) {
          for (int j = 0; j < 32; ++j) {
               inset1.insert(32*i + j);
          }
     }
     inset2 = inset1;

     set<int> outset1 = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
                         16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
     set<int> outset2 = outset1;

     PartMap pm1(inset1, inset2), pm2(outset1, outset2);
     vector<PartMap> vpm1 = {pm1};
     vector<PartMap> vpm2 = {pm2};

     // the mapped input, output and the result
     map<int,int> inmap, outmap;
     list<FullMap> result;

     varmap(v1, v2, se1, se2, &inv1, &inv2, vpm1, vpm2, inmap, outmap, &result);
     if (result.size() != 0) {
          cout << "variable mapping result: " << result.size() << " possible mapping found." << endl;
          outputBitCVC(v1, v2, &inv1, &inv2, &result);
     } else {
          cout << "no mapping found" << endl;
     }

}
