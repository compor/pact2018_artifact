#ifndef INTERPOLATE_H
#define INTERPOLATE_H 
#include <limits.h>
#include <vector>

using namespace std;

#define MY_TINY (-99999999) 
#define MY_HUGE  (999999999)
#define MY_SMALL (-88888888)
#define MY_LARGE (88888888)

#define MMSIZE (1024*1024*4)

#define Rectf_min(a,b) ((a)<(b)?(a):(b))
#define Rectf_max(a,b) ((a)>(b)?(a):(b))
#define Rectf_max2(y1, x2, y2, x) Rectf_max(y1, (x+y2-x2))
#define Rectf_min2(x1, y1, x2, y2, x) Rectf_min(y2, (x+y1-x1))
#define Linr_01(y0, y1, x) ((y1-y0)*x + y0)
#define Thrld_max(a, b, c, d) ((a)>(b)?(c):(d))


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

#endif /* INTERPOLATE_H */
