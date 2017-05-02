//     _   __ _____    
//    / | / /\___  \   
//   /  |/ /    /  /   
//  / /|  /    /  /__  
// /_/ |_/     \_____\ 
//                     


#include <cmath>
#include <random>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ocean.h"
#include "fft.h"


using namespace std;


cOcean::cOcean(const int size, const float phillipsA, const glm::vec2 wind, const float length, bool geometry, const float gconst) {
    this->N = size;

    h_tilde = new complex<float>[N * N];
    h_tilde_slopex = new complex<float>[N * N];
    h_tilde_slopez = new complex<float>[N * N];
    h_tilde_dx = new complex<float>[N * N];
    h_tilde_dz = new complex<float>[N * N];
    fft = new cFFT(N);

    this->phillipsA = phillipsA;
    this->wind = wind;
    this->length = length;
    this->geometry = geometry;
    this->gconst = gconst;
    this->pieces = NULL;

    this->normal_d = normal_distribution<float>(0.0f, 1.0f);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    _generate();
}

cOcean::~cOcean() {
    if (pieces)
        delete[] pieces;

    if (h_tilde)
        delete[] h_tilde;

    if (h_tilde_slopex)
        delete[] h_tilde_slopex;

    if (h_tilde_slopez)
        delete[] h_tilde_slopez;

    if (h_tilde_dx)
        delete[] h_tilde_dx;

    if (h_tilde_dz)
        delete[] h_tilde_dz;

    if (fft)
        delete fft;
}

void cOcean::release() {

}


void cOcean::bind(Shader shader) {
    this->shader = shader;
}


float cOcean::dispersion(int n_prime, int m_prime) {
    float w_0 = 2.0f * M_PI / 200.0f;
    float kx = M_PI * (2 * n_prime - N) / length;
    float kz = M_PI * (2 * m_prime - N) / length;

    return floor(sqrt(gconst * sqrt(kx * kx + kz * kz)) / w_0) * w_0;
}


float cOcean::phillips(int n_prime, int m_prime) {
    glm::vec2 k(M_PI * (2 * n_prime - N) / length, M_PI * (2 * m_prime - N) / length);
    float k_length = glm::length(k);
    if (k_length < 0.00001) 
        return 0.0;

    float k_length2 = k_length  * k_length;
    float k_length4 = k_length2 * k_length2;

    float k_dot_w = glm::dot(glm::normalize(k), glm::normalize(wind));
    float k_dot_w2 = k_dot_w * k_dot_w;

    float w_length = wind.length();
    float L = w_length * w_length / gconst;
    float L2 = L * L;

    float damping = 0.001;
    float l2 = L2 * damping * damping;

    return phillipsA * exp(-1.0f / (k_length2 * L2)) / k_length4 * k_dot_w2;
}

complex<float> cOcean::hTilde_0(int n_prime, int m_prime) {
    float real, imag;

    real = normal_d(engine);
    imag = normal_d(engine);

    complex<float> r(real, imag);
    complex<float> phi_debug = phillips(n_prime, m_prime);
    complex<float> debug = r * sqrt(phillips(n_prime, m_prime) / 2.0f);
    return r * sqrt(phillips(n_prime, m_prime) / 2.0f);
}

complex<float> cOcean::hTilde(float t, int n_prime, int m_prime) {
    int index = m_prime * (N + 1) + n_prime;

    complex<float> ht = vertices[index].hTilde;
    complex<float> conj_ht = vertices[index].conj_hTilde;

    float omegat = dispersion(n_prime, m_prime) * t;

    float cos_ = cos(omegat);
    float sin_ = sin(omegat);

    complex<float> c0(cos_, sin_);
    complex<float> c1(cos_, -sin_);

    complex<float> res = ht * c0 + conj_ht * c1;

    return res;
}

cOcean::sVertexNormal cOcean::update(glm::vec2 x, float t) {
    complex<float> h(0.0f, 0.0f);
    glm::vec2 D(0.0f, 0.0f);
    glm::vec3 n(0.0f, 0.0f, 0.0f);

    complex<float> c, res, htilde_c;
    glm::vec2 k;
    float kx, kz, k_length, k_dot_x;

    for (int m_prime = 0; m_prime < N; m_prime++) {
        kz = 2.0f * M_PI * (m_prime - N / 2.0f) / length;
        for (int n_prime = 0; n_prime < N; n_prime++) {
            kx = 2.0f * M_PI * (n_prime - N / 2.0f) / length;
            k = glm::vec2(kx, kz);

            k_length = glm::length(k);
            k_dot_x = glm::dot(k, x);

            c = complex<float>(cos(k_dot_x), sin(k_dot_x));
            htilde_c = hTilde(t, n_prime, m_prime) * c;

            h = h + htilde_c;

            float h_imag = htilde_c.imag();
            n = n + glm::vec3(-kx * h_imag, 0.0f, -kz * h_imag);

            if (k_length < 0.000001) continue;
            D = D + glm::vec2(kx / k_length * h_imag, kz / k_length * h_imag);
        }
    }

    n = glm::normalize((glm::vec3(0.0f, 1.0f, 0.0f) - n));

    sVertexNormal cvn(h, D, n);

    return cvn;
}

void cOcean::evaluateWaves(float t) {
    float lambda = -1.0;
    int index;
    glm::vec2 x;
    glm::vec2 d;
    sVertexNormal h_d_and_n;

    for (int m_prime = 0; m_prime < N; m_prime++) {
        for (int n_prime = 0; n_prime < N; n_prime++) {
            index = m_prime * (N + 1) + n_prime;

            x = glm::vec2(vertices[index].pos.x, vertices[index].pos.z);

            h_d_and_n = update(x, t);

            vertices[index].pos.y = h_d_and_n.height.real();

            vertices[index].pos.x = vertices[index].origin.x + lambda * h_d_and_n.displacement.x;
            vertices[index].pos.z = vertices[index].origin.z + lambda * h_d_and_n.displacement.y;

            vertices[index].normal.x = h_d_and_n.normal.x;
            vertices[index].normal.y = h_d_and_n.normal.y;
            vertices[index].normal.z = h_d_and_n.normal.z;
        }
    }
}

void cOcean::evaluateWavesFFT(float t) {
    float kx, kz, len, lambda = -1.0f;
    int index, index1;

    for (int m_prime = 0; m_prime < N; m_prime++) {
        kz = M_PI * (2.0f * m_prime - N) / length;
        for (int n_prime = 0; n_prime < N; n_prime++) {
            kx = M_PI * (2 * n_prime - N) / length;
            len = sqrt(kx * kx + kz * kz);
            index = m_prime * N + n_prime;

            h_tilde[index] = hTilde(t, n_prime, m_prime);
            h_tilde_slopex[index] = h_tilde[index] * complex<float>(0, kx);
            h_tilde_slopez[index] = h_tilde[index] * complex<float>(0, kz);
            if (len < 0.000001f) {
                h_tilde_dx[index] = complex<float>(0.0f, 0.0f);
                h_tilde_dz[index] = complex<float>(0.0f, 0.0f);
            }
            else {
                h_tilde_dx[index] = h_tilde[index] * complex<float>(0, -kx / len);
                h_tilde_dz[index] = h_tilde[index] * complex<float>(0, -kz / len);
            }
        }
    }
    //for (int i = 0; i < N * N; i++)
    //    cout << h_tilde[i] << endl;
    //cout << "===========" << endl;

    for (int m_prime = 0; m_prime < N; m_prime++) {
        fft->fft(h_tilde, h_tilde, 1, m_prime * N);
        fft->fft(h_tilde_slopex, h_tilde_slopex, 1, m_prime * N);
        fft->fft(h_tilde_slopez, h_tilde_slopez, 1, m_prime * N);
        fft->fft(h_tilde_dx, h_tilde_dx, 1, m_prime * N);
        fft->fft(h_tilde_dz, h_tilde_dz, 1, m_prime * N);
    }

    //for (int i = 0; i < N * N; i++)
    //    cout << h_tilde[i] << endl;
    //cout << "===========" << endl;

    for (int n_prime = 0; n_prime < N; n_prime++) {
        fft->fft(h_tilde, h_tilde, N, n_prime);
        fft->fft(h_tilde_slopex, h_tilde_slopex, N, n_prime);
        fft->fft(h_tilde_slopez, h_tilde_slopez, N, n_prime);
        fft->fft(h_tilde_dx, h_tilde_dx, N, n_prime);
        fft->fft(h_tilde_dz, h_tilde_dz, N, n_prime);
    }

    //for (int i = 0; i < N * N; i++)
    //    cout << h_tilde[i] << endl;

    int Nplus1 = N + 1;
    complex<float> signs[] = {complex<float>(1.0f, 0.0f), complex<float>(-1.0f, 0.0f)}, sign;
    glm::vec3 n;

    for (int m_prime = 0; m_prime < N; m_prime++) {
        for (int n_prime = 0; n_prime < N; n_prime++) {
            index = m_prime * N + n_prime;        // index into h_tilde..
            index1 = m_prime * Nplus1 + n_prime;  // index into vertices

            sign = signs[(n_prime + m_prime) & 1];

            h_tilde[index] = h_tilde[index] * sign;

            // height
            vertices[index1].pos.y = h_tilde[index].real();

            // displacement
            h_tilde_dx[index] = h_tilde_dx[index] * sign;
            h_tilde_dz[index] = h_tilde_dz[index] * sign;
            vertices[index1].pos.x = vertices[index1].origin.x + h_tilde_dx[index].real() * lambda;
            vertices[index1].pos.z = vertices[index1].origin.z + h_tilde_dz[index].real() * lambda;

            // normal
            h_tilde_slopex[index] = h_tilde_slopex[index] * sign;
            h_tilde_slopez[index] = h_tilde_slopez[index] * sign;
            n = glm::normalize(glm::vec3(0.0f - h_tilde_slopex[index].real(), 1.0f, 0.0f - h_tilde_slopez[index].real()));
            vertices[index1].normal.x = n.x;
            vertices[index1].normal.y = n.y;
            vertices[index1].normal.z = n.z;

            // for tiling
            if (n_prime == 0 && m_prime == 0) {
                vertices[index1 + N + Nplus1 * N].pos.y = h_tilde[index].real();

                vertices[index1 + N + Nplus1 * N].pos.x = vertices[index1 + N + Nplus1 * N].origin.x + h_tilde_dx[index].real() * lambda;
                vertices[index1 + N + Nplus1 * N].pos.z = vertices[index1 + N + Nplus1 * N].origin.z + h_tilde_dz[index].real() * lambda;

                vertices[index1 + N + Nplus1 * N].normal.x = n.x;
                vertices[index1 + N + Nplus1 * N].normal.y = n.y;
                vertices[index1 + N + Nplus1 * N].normal.z = n.z;
            }
            if (n_prime == 0) {
                vertices[index1 + N].pos.y = h_tilde[index].real();

                vertices[index1 + N].pos.x = vertices[index1 + N].origin.x + h_tilde_dx[index].real() * lambda;
                vertices[index1 + N].pos.z = vertices[index1 + N].origin.z + h_tilde_dz[index].real() * lambda;

                vertices[index1 + N].normal.x = n.x;
                vertices[index1 + N].normal.y = n.y;
                vertices[index1 + N].normal.z = n.z;
            }
            if (m_prime == 0) {
                vertices[index1 + Nplus1 * N].pos.y = h_tilde[index].real();

                vertices[index1 + Nplus1 * N].pos.x = vertices[index1 + Nplus1 * N].origin.x + h_tilde_dx[index].real() * lambda;
                vertices[index1 + Nplus1 * N].pos.z = vertices[index1 + Nplus1 * N].origin.z + h_tilde_dz[index].real() * lambda;

                vertices[index1 + Nplus1 * N].normal.x = n.x;
                vertices[index1 + Nplus1 * N].normal.y = n.y;
                vertices[index1 + Nplus1 * N].normal.z = n.z;
            }
        }
    }
}


void cOcean::render(float t, glm::vec3 lightPos, glm::vec3 view_pos, glm::mat4 Projection, glm::mat4 View, glm::mat4 Model, bool use_fft) {

    evaluateWavesFFT(t);

    _load();

    shader.Use();   // Don't forget this one!
                    // Transformation matrices
                    // Set up lights
    GLint lightPosLoc = glGetUniformLocation(shader.Program, "light_position");
    glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);

    // Set up camera
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "Projection"), 1, GL_FALSE, glm::value_ptr(Projection));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "View"), 1, GL_FALSE, glm::value_ptr(View));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "Model"), 1, GL_FALSE, glm::value_ptr(Model));

    glBindVertexArray(VAO);
    if (!geometry)
        glDrawArrays(GL_TRIANGLES, 0, indices.size());
    else {
        glVertexPointer(3, GL_FLOAT, 0, pieces);
        for (int i = 0; i < indices.size(); i += 3)
            glDrawArrays(GL_LINE_LOOP, i, 3);
    }

    glBindVertexArray(0);
}


void cOcean::_generate() {
    int total_size = N * N;

    // Generate vertices
    for (int i = 0; i < N + 1; i++)
        for (int j = 0; j < N + 1; j++) {
            std::complex<float> ht = hTilde_0(j, i);
            std::complex<float> conj_ht = hTilde_0(-j, -i);
            conj_ht = conj(conj_ht);

            glm::vec3 origin((j - N / 2.0f) * length / N, 0.0f, (i - N / 2.0f) * length / N);
            sOceanVertex vertex(origin, glm::vec3(0.0f, 1.0f, 0.0f), ht, conj_ht, origin);
            vertices.push_back(vertex);
        }

    // Generate indices
    int M = N + 1;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            int four[4] = { M * i + j, M * i + j + 1, M * (i + 1) + j + 1, M * (i + 1) + j };
            indices.push_back(four[0]);  //    /|
            indices.push_back(four[1]);  //   / |
            indices.push_back(four[2]);  //  /__|

            indices.push_back(four[2]);  //  ___
            indices.push_back(four[3]);  //  | /
            indices.push_back(four[0]);  //  |/

            indices.push_back(four[3]);  //  |.
            indices.push_back(four[0]);  //  | .
            indices.push_back(four[1]);  //  |__.
        }
}

void cOcean::_load() {
    int size = indices.size() * 6 * sizeof(GLfloat);

    if (pieces)
        delete[] pieces;

    pieces = new GLfloat[size];

    for (int i = 0; i < indices.size(); i++) {
        int index = indices[i];
        pieces[i * 6 + 0] = vertices[index].pos.x;
        pieces[i * 6 + 1] = vertices[index].pos.y;
        pieces[i * 6 + 2] = vertices[index].pos.z;
        pieces[i * 6 + 3] = vertices[index].normal.x;
        pieces[i * 6 + 4] = vertices[index].normal.y;
        pieces[i * 6 + 5] = vertices[index].normal.z;
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glInvalidateBufferData(VBO);
    glBufferData(GL_ARRAY_BUFFER, size, pieces, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); // Unbind VAO
}
