#ifndef INTERPOLATE_H
#define INTERPOLATE_H 
#include <limits.h>
#include <vector>
#include "immintrin.h"
#include "math.h"
#include "sse_api.h"

using namespace std;

#define FLT_TINY (float)(INT_MIN>>11)
#define FLT_HUGE  (float)(INT_MAX>>11)
#define FLT_SMALL (float)(INT_TINY>>1)
#define FLT_BIG (float)(INT_HUGE>>1)

#define INT_TINY (INT_MIN>>11)
#define INT_HUGE  (INT_MAX>>11)
#define INT_SMALL (INT_TINY>>1)
#define INT_BIG (INT_HUGE>>1)

#define MMSIZE (1024*1024*4)

#define Rectf_min(a,b) ((a)<(b)?(a):(b))
#define Rectf_max(a,b) ((a)>(b)?(a):(b))
#define Rectf_max2(y1, x2, y2, x) Rectf_max(y1, (x+y2-x2))
#define Rectf_min2(x1, y1, x2, y2, x) Rectf_min(y2, (x+y1-x1))
#define Linr_01(y0, y1, x) ((y1-y0)*x + y0)

vint Rectf_vmax(const vint &a, const vint &b) {
    return max(a, b);
}

vfloat Rectf_vmax(const vfloat &a, const vfloat &b) {
    return max(a, b);
}

vfloat Rectf_vmin(const vfloat &a, const vfloat &b) {
    return min(a, b);
}

vint Rectf_vmin(const vint &a, const vint &b) {
    return min(a, b);
}

vint Rectf_vmax2(int y1, int x2, int y2, const vint &x) {
    return Rectf_vmax(y1, (x+(y2-x2)));
}

template<class T>
vector<double> Rectf_vmax(T* a, T b, int n) {
    vector<double> res;
    for(int i=0;i<n;i++)
        res.push_back(Rectf_max(a[i], b));
    return res;
}

template<class T>
void MeshGrid(const vector<T> &a, const vector<T> &b, T *c, T *d) {
    int i = 0;
    for(int j=0;j<b.size();j++) {
        for(int k=0;k<a.size();k++) {
            c[i] = a[k];
            d[i] = b[j];
            i++;
        }
    }
}

template<class T>
void MeshGridFull(const vector<T> &a, const vector<T> &b, T *c, T *d) {
    int i = 0;
    while(i<16) {
        for(int j=0;j<b.size();j++) {
            for(int k=0;k<a.size();k++) {
                c[i] = a[k];
                d[i] = b[j];
                i++;
            }
        }
    }
}



template<class T>
void MeshGrid(const vector<T> &a, const vector<T> &b, const vector<T> &c, T *d, T *e, T *f) {
    int i = 0;
    for(int p=0;p<c.size();p++) {
        for(int j=0;j<b.size();j++) {
            for(int k=0;k<a.size();k++) {
                d[i] = a[k];
                e[i] = b[j];
                f[i] = c[p];
                i++;
            }
        }
    }
}


template <class T>
vector<T> Linspace(T low, T high, int num) {
    vector<T> res;
    T bl = 1.0 * (high - low) / (num - 1);
    for(int i=0;i<num-1;i++) {
        res.push_back(low+i*bl);
    }
    res.push_back(high);
    return res;
}

template <class T>
void Linspace(T *buf, T low, T high, int num) {
    T bl = 1.0 * (high - low) / (num - 1);
    for(int i=0;i<num-1;i++) {
        buf[i] = low+i*bl;
    }
    buf[num-1] = high;
}

template <class T>
T Linr(T out0, T out1, T in0, T in1, T in_real) {
    return (out1-out0) / (in1-in0) * (in_real-in0) + out0;
}

template <class T>
T Rectf(T in0, T out0, T in1, T out1, T in2, T out2, T in3, T out3, T in_real) {

    double a = (double)out2 - ((double)out3 - (double)out2) / ((double)in3 - (double)in2) * (double)in2 + ((double)out3 - (double)out2) / ((double)in3 - (double)in2) * (double)in_real;
    double b = (double)out0 - ((double)out1 - (double)out0) / ((double)in1 - (double)in0) * (double)in0 + ((double)out1 - (double)out0) / ((double)in1 - (double)in0) * (double)in_real;

    if(out0 > out1) {
        return a > b? (T)a : (T)b;
    } else if(out0 < out1) {
        return a < b? (T)a : (T)b;
    } else if(out2 < out3) {
        return a > b? (T)a : (T)b;
    } else {
        return a < b? (T)a : (T)b;
    }
}


template <class T>
class SymList {
    T *arr;
    int tsize;
    using TT = typename conditional<is_same<T, int>::value, __m512i, __m512>::type;
    public:
    SymList() {
        tsize = 0;
        arr = (T *)_mm_malloc(sizeof(T)*MMSIZE*16, 64);
    }
    ~SymList() {
        _mm_free(arr);
    }
    int size() {
        return tsize;
    }
    void clear() {
        tsize = 0;
    }
    void push_back(TT elem, __mmask16 mas) {
        _mm512_mask_store_epi32(arr+tsize*16, mas, elem);
        tsize++;
    }
    void push_back(TT elem) {
        _mm512_store_epi32(arr+tsize*16, elem);
        tsize++;
    }

    T* operator [] (int idx) {
        return arr + idx*16;
    }
};

#endif /* INTERPOLATE_H */
