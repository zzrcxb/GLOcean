//     _   __ _____    
//    / | / /\___  \   
//   /  |/ /    /  /   
//  / /|  /    /  /__  
// /_/ |_/     \_____\ 
//                     


#pragma once

#define M_PI 3.14159265358979323846f
// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>

// STD
#include <iostream>
#include <vector>
#include <complex>
#include <random>

// Shaders
#include "shader.h"
#include "fft.h"



class cOcean {
public:
    class sOceanVertex {
    public:
        glm::vec3 pos;
        glm::vec3 normal;
        std::complex<float> hTilde;
        std::complex<float> conj_hTilde;
        glm::vec3 origin;

        sOceanVertex(glm::vec3 pos, glm::vec3 normal, std::complex<float> hTilde, std::complex<float> conj_hTilde, glm::vec3 origin) {
            this->pos = pos;
            this->normal = normal;
            this->hTilde = hTilde;
            this->conj_hTilde = conj_hTilde;
            this->origin = origin;
        }
    };

    class sVertexNormal {
    public:
        std::complex<float> height;
        glm::vec2 displacement;
        glm::vec3 normal;

        sVertexNormal(){}

        sVertexNormal(std::complex<float> height, glm::vec2 displacement, glm::vec3 normal) {
            this->height = height;
            this->displacement = displacement;
            this->normal = normal;
        }
    };

    cOcean(const int size, const float phillipsA, const glm::vec2 wind, const float length, bool geometry, const float gconst=9.8);
    ~cOcean();
    void release();
    void bind(Shader shader);

    float dispersion(int n_prime, int m_prime);      // deep water
    float phillips(int n_prime, int m_prime);        // phillips spectrum
    std::complex<float> hTilde_0(int n_prime, int m_prime);
    std::complex<float> hTilde(float t, int n_prime, int m_prime);
    void evaluateWaves(float t);
    void evaluateWavesFFT(float t);
    void render(float t, glm::vec3 light_pos, glm::vec3 view_pos, glm::mat4 Projection, glm::mat4 View, glm::mat4 Model, bool use_fft);
    sVertexNormal update(glm::vec2 x, float t);

private:
    bool geometry;

    float gconst;
    int N, Np; // Power of 2
    float phillipsA;
    glm::vec2 wind;
    float length;

    // Used to store vertices of ocean
    std::vector<sOceanVertex> vertices;
    std::vector<int> indices;

    // GL
    GLuint VAO, VBO;
    Shader shader;
    GLfloat *pieces;

    // Random
    std::default_random_engine engine;
    std::normal_distribution<float> normal_d;

    // FFT
    std::complex<float> *h_tilde, *h_tilde_slopex, *h_tilde_slopez, *h_tilde_dx, *h_tilde_dz;
    cFFT *fft;

    void _generate();
    void _load();
};
