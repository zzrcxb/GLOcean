//     _   __ _____    
//    / | / /\___  \   
//   /  |/ /    /  /   
//  / /|  /    /  /__  
// /_/ |_/     \_____\ 
//                     


#pragma once

#include <cmath>
#include <complex>


class cFFT {
  private:
    unsigned int N, which;
    unsigned int log_2_N;
    float pi2;
    unsigned int *reversed;
    std::complex<float> **T;
    std::complex<float>* c[2];
  protected:
  public:
    cFFT(unsigned int N);
    ~cFFT();
    unsigned int reverse(unsigned int i);
    std::complex<float> t(unsigned int x, unsigned int N);
    std::complex<float>* fft(std::complex<float>* input, std::complex<float>* output, int stride, int offset);
};
