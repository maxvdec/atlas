//
// abstract.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Abstract vector and matrix definitions
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef BEZEL_ABSTRACT_H
#define BEZEL_ABSTRACT_H

#include <glm/glm.hpp>

namespace bezel {
class vecN {
  public:
    int number;
    float *data;

    vecN() : number(0), data(nullptr) {}
    vecN(int n);
    vecN(const vecN &other);
    vecN &operator=(const vecN &other);
    ~vecN() { delete[] data; };

    float operator[](const int i) const { return data[i]; }
    float &operator[](const int i) { return data[i]; }
    const vecN &operator*=(float rhs);
    vecN operator*(const float rhs) const;
    vecN operator+(const vecN &rhs) const;
    vecN operator-(const vecN &rhs) const;
    const vecN &operator+=(const vecN &rhs);
    const vecN &operator-=(const vecN &rhs);

    void fill(float value);

    inline static vecN from3(const glm::vec3 &v) {
        vecN result(3);
        result.data[0] = v.x;
        result.data[1] = v.y;
        result.data[2] = v.z;
        return result;
    }
};

inline vecN::vecN(int N) {
    number = N;
    data = new float[N];
    for (int i = 0; i < N; i++) {
        data[i] = 0.0f;
    }
}

inline vecN::vecN(const vecN &other) {
    number = other.number;
    data = new float[number];
    for (int i = 0; i < number; i++) {
        data[i] = other.data[i];
    }
}

inline vecN &vecN::operator=(const vecN &other) {
    if (this != &other) {
        delete[] data;
        number = other.number;
        data = new float[number];
        for (int i = 0; i < number; i++) {
            data[i] = other.data[i];
        }
    }
    return *this;
}

inline const vecN &vecN::operator*=(float rhs) {
    for (int i = 0; i < number; i++) {
        data[i] *= rhs;
    }
    return *this;
}

inline vecN vecN::operator*(const float rhs) const {
    vecN result(number);
    for (int i = 0; i < number; i++) {
        result.data[i] = data[i] * rhs;
    }
    return result;
}

inline vecN vecN::operator+(const vecN &rhs) const {
    vecN result(number);
    for (int i = 0; i < number; i++) {
        result.data[i] = data[i] + rhs.data[i];
    }
    return result;
}

inline vecN vecN::operator-(const vecN &rhs) const {
    vecN result(number);
    for (int i = 0; i < number; i++) {
        result.data[i] = data[i] - rhs.data[i];
    }
    return result;
}

inline const vecN &vecN::operator+=(const vecN &rhs) {
    for (int i = 0; i < number; i++) {
        data[i] += rhs.data[i];
    }
    return *this;
}

inline const vecN &vecN::operator-=(const vecN &rhs) {
    for (int i = 0; i < number; i++) {
        data[i] -= rhs.data[i];
    }
    return *this;
}

inline void vecN::fill(float value) {
    for (int i = 0; i < number; i++) {
        data[i] = value;
    }
}

} // namespace bezel

namespace bezel {
class matMN {
  public:
    int rows;
    int cols;
    vecN *data;

    matMN() : rows(0), cols(0), data(nullptr) {}
    matMN(int m, int n);
    inline matMN(const matMN &other) { *this = other; }
    ~matMN() { delete[] data; };

    const matMN &operator=(const matMN &other);
    const matMN &operator*=(float rhs);
    vecN operator*(const vecN &rhs) const;
    matMN operator*(const matMN &rhs) const;
    matMN operator*(const float rhs) const;

    void fill(float value);
    matMN transpose() const;
};

inline matMN::matMN(int M, int N) {
    rows = M;
    cols = N;
    data = new vecN[M];
    for (int i = 0; i < M; i++) {
        data[i] = vecN(N);
    }
}

inline const matMN &matMN::operator=(const matMN &other) {
    if (this != &other) {
        delete[] data;
        rows = other.rows;
        cols = other.cols;
        data = new vecN[rows];
        for (int i = 0; i < rows; i++) {
            data[i] = other.data[i];
        }
    }
    return *this;
}

inline const matMN &matMN::operator*=(float rhs) {
    for (int i = 0; i < rows; i++) {
        data[i] *= rhs;
    }
    return *this;
}

inline vecN matMN::operator*(const vecN &rhs) const {
    if (cols != rhs.number) {
        return vecN(0);
    }
    vecN result(rows);
    for (int i = 0; i < rows; i++) {
        result[i] = 0.0f;
        for (int j = 0; j < cols; j++) {
            result[i] += data[i][j] * rhs[j];
        }
    }
    return result;
}

inline matMN matMN::operator*(const matMN &rhs) const {
    if (cols != rhs.rows) {
        return matMN(0, 0);
    }
    matMN result(rows, rhs.cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < rhs.cols; j++) {
            result.data[i][j] = 0.0f;
            for (int k = 0; k < cols; k++) {
                result.data[i][j] += data[i][k] * rhs.data[k][j];
            }
        }
    }
    return result;
}

inline matMN matMN::operator*(const float rhs) const {
    matMN result(rows, cols);
    for (int i = 0; i < rows; i++) {
        result.data[i] = data[i] * rhs;
    }
    return result;
}

inline void matMN::fill(float value) {
    for (int i = 0; i < rows; i++) {
        data[i].fill(value);
    }
}

inline matMN matMN::transpose() const {
    matMN result(cols, rows);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            result.data[j][i] = data[i][j];
        }
    }
    return result;
}

class matN {
  public:
    int dimensionNum;
    vecN *data;

    matN() : dimensionNum(0), data(nullptr) {}
    matN(int n);
    inline matN(const matN &other) { *this = other; }
    inline matN(const matMN &other) { *this = other; }
    ~matN() { delete[] data; };

    const matN &operator=(const matN &other);
    const matN &operator=(const matMN &other);

    void identity();
    void fill(float value);
    void transpose();

    void operator*=(float rhs);
    vecN operator*(const vecN &rhs) const;
    matN operator*(const matN &rhs) const;
};

inline matN::matN(int N) {
    dimensionNum = N;
    data = new vecN[N];
    for (int i = 0; i < N; i++) {
        data[i] = vecN(N);
    }
}

inline const matN &matN::operator=(const matN &other) {
    if (this != &other) {
        delete[] data;
        dimensionNum = other.dimensionNum;
        data = new vecN[dimensionNum];
        for (int i = 0; i < dimensionNum; i++) {
            data[i] = other.data[i];
        }
    }
    return *this;
}

inline const matN &matN::operator=(const matMN &other) {
    if (other.rows != other.cols) {
        return *this;
    }

    dimensionNum = other.rows;
    delete[] data;
    data = new vecN[dimensionNum];
    for (int i = 0; i < dimensionNum; i++) {
        data[i] = other.data[i];
    }
    return *this;
}

inline void matN::identity() {
    for (int i = 0; i < dimensionNum; i++) {
        for (int j = 0; j < dimensionNum; j++) {
            data[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}

inline void matN::fill(float value) {
    for (int i = 0; i < dimensionNum; i++) {
        data[i].fill(value);
    }
}

inline void matN::transpose() {
    for (int i = 0; i < dimensionNum; i++) {
        for (int j = i + 1; j < dimensionNum; j++) {
            float temp = data[i][j];
            data[i][j] = data[j][i];
            data[j][i] = temp;
        }
    }
}

inline void matN::operator*=(float rhs) {
    for (int i = 0; i < dimensionNum; i++) {
        data[i] *= rhs;
    }
}

inline vecN matN::operator*(const vecN &rhs) const {
    vecN result(dimensionNum);
    for (int i = 0; i < dimensionNum; i++) {
        result[i] = 0.0f;
        for (int j = 0; j < dimensionNum; j++) {
            result[i] += data[i][j] * rhs[j];
        }
    }
    return result;
}

inline matN matN::operator*(const matN &rhs) const {
    matN result(dimensionNum);
    for (int i = 0; i < dimensionNum; i++) {
        for (int j = 0; j < dimensionNum; j++) {
            result.data[i][j] = 0.0f;
            for (int k = 0; k < dimensionNum; k++) {
                result.data[i][j] += data[i][k] * rhs.data[k][j];
            }
        }
    }
    return result;
}

} // namespace bezel

namespace bezel {
inline float dot(const vecN &a, const vecN &b) {
    if (a.number != b.number) {
        return 0.0f;
    }
    float result = 0.0f;
    for (int i = 0; i < a.number; i++) {
        result += a.data[i] * b.data[i];
    }
    return result;
}
} // namespace bezel

#endif // BEZEL_ABSTRACT_H