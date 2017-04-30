#pragma once
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

// Shaders
#include "shader.h"



class cOcean {
public:
    typedef struct {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 hTilde;
        glm::vec3 conj_hTilde;
        glm::vec3 origin;
    } sOceanVertex;

    typedef struct {
        std::complex<float> height;
        glm::vec3 displacement;
        glm::vec3 normal;
    } sVertexNormal;

    cOcean(const int size, const float phillipsA, const glm::vec2 wind, const float length, bool geometry, const float gconst=9.8);
    ~cOcean();
    void release();
    void bind(Shader shader);

    float dispersion(int n_prime, int m_prime);      // deep water
    float phillips(int n_prime, int m_prime);        // phillips spectrum
    std::complex<float> hTilde_0(int n_prime, int m_prime);
    std::complex<float> hTilde(float t, int n_prime, int m_prime);
    sVertexNormal h_D_and_n(glm::vec2 x, float t);
    void evaluateWaves(float t);
    void evaluateWavesFFT(float t);
    void render(float t, glm::vec3 light_pos, glm::mat4 Projection, glm::mat4 View, glm::mat4 Model, bool use_fft);


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


};
