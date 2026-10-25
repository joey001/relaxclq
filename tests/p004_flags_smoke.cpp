// Legacy packed-bool overflow/aliasing regression, validation only.
#include "maximal_kplex/sequence.h"
#include <cassert>
#include <memory>
int main() {
  for (int n : {0,1,127,128,511,512,513,1024,2048,4096,4100}) {
    std::unique_ptr<bool[]> storage(new bool[n+4]);
    for (int offset : {0,1,2,3}) for (int pattern=0;pattern<4;++pattern) {
      bool* flags=storage.get()+offset;
      int expected=0;
      for (int i=0;i<n;++i) {
        flags[i]=pattern==0 ? false : pattern==1 ? true : pattern==2 ? (i%2!=0) : ((unsigned(i)*1664525u+1013904223u)%7<3);
        expected+=flags[i];
      }
      assert(sequence::sumFlagsSerial(flags,n)==expected);
    }
  }
}
