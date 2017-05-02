#define M_PI 3.14159265358979323846f
#include "fft.h"


using namespace std;


cFFT::cFFT(unsigned int N) {
    this->N = N;
    this->reversed = NULL;
    this->T = NULL;
    this->pi2 = 2 * M_PI;

    c[0] = c[1] = NULL;

    log_2_N = log(N) / log(2);

    reversed = new unsigned int[N];        // prep bit reversals
    for (int i = 0; i < N; i++) reversed[i] = reverse(i);

    int pow2 = 1;
    T = new complex<float>*[log_2_N];      // prep T
    for (int i = 0; i < log_2_N; i++) {
        T[i] = new complex<float>[pow2];
        for (int j = 0; j < pow2; j++) T[i][j] = t(j, pow2 * 2);
        pow2 *= 2;
    }

    c[0] = new complex<float>[N];
    c[1] = new complex<float>[N];

    which = 0;
}


cFFT::~cFFT() {
    if (c[0]) delete [] c[0];
    if (c[1]) delete [] c[1];
    if (T) {
        for (int i = 0; i < log_2_N; i++) if (T[i]) delete [] T[i];
        delete [] T;
    }
    if (reversed) delete [] reversed;
}


unsigned int cFFT::reverse(unsigned int i) {
    unsigned int res = 0;
    for (int j = 0; j < log_2_N; j++) {
        res = (res << 1) + (i & 1);
        i >>= 1;
    }
    return res;
}


complex<float> cFFT::t(unsigned int x, unsigned int N) {
    return complex<float>(cos(pi2 * x / N), sin(pi2 * x / N));
}


std::complex<float>* cFFT::fft(complex<float>* input, complex<float>* output, int stride, int offset) {
    for (int i = 0; i < N; i++) c[which][i] = input[reversed[i] * stride + offset];

    int loops       = N >> 1;
    int size        = 1 << 1;
    int size_over_2 = 1;
    int w_          = 0;
    for (int i = 1; i <= log_2_N; i++) {
        which ^= 1;
        for (int j = 0; j < loops; j++) {
            for (int k = 0; k < size_over_2; k++) {
                c[which][size * j + k] =  c[which ^ 1][size * j + k] + c[which ^ 1][size * j + size_over_2 + k] * T[w_][k];
            }

            for (int k = size_over_2; k < size; k++) {
                c[which][size * j + k] =  c[which ^ 1][size * j - size_over_2 + k] - c[which^1][size * j + k] * T[w_][k - size_over_2];
            }
        }
        loops       >>= 1;
        size        <<= 1;
        size_over_2 <<= 1;
        w_++;
    }

    for (int i = 0; i < N; i++) output[i * stride + offset] = c[which][i];
    return output;
}
