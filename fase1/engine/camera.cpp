#define _USE_MATH_DEFINES
#include "camera.h"
#include <math.h>
#include <GL/glut.h>
#include <iostream>

/// Incremento de rotação em radianos por chamada
const float ROTATION_INCREMENT = 0.1f;
/// Incremento de zoom em unidades por chamada
const float ZOOM_INCREMENT = 0.1f;
/// Raio mínimo permitido para evitar câmera muito próxima
const float MIN_RADIUS = 0.1f;
/// Margem de segurança para limites de beta (elevação)
const float BETA_MARGIN = 0.01f;


Camera::Camera() {
    // Define a posição inicial da câmera no eixo Z positivo
    posX = 0.0f; posY = 0.0f; posZ = 5.0f;
    
    // Define o ponto de interesse na origem (0, 0, 0)
    lookAtX = 0.0f; lookAtY = 0.0f; lookAtZ = 0.0f;
    
    // Define o vetor "up" apontando para o eixo Y positivo
    upX = 0.0f; upY = 1.0f; upZ = 0.0f;
    
    // Define parâmetros de projeção padrão para perspectiva
    fov = 60.0f; nearPlane = 1.0f; farPlane = 1000.0f;
    
    // Calcula as coordenadas esféricas equivalentes à posição inicial
    calculateSphericalCoords();
}

//construtor parametrizado
Camera::Camera(float px, float py, float pz, 
               float lx, float ly, float lz, 
               float ux, float uy, float uz, 
               float fovVal, float near, float far) {
    
    setPosition(px, py, pz);
    setLookAt(lx, ly, lz);
    setUp(ux, uy, uz);
    setProjection(fovVal, near, far);
    
    // Calcula as coordenadas esféricas equivalentes
    calculateSphericalCoords();
}

// calculos de coordenadas

void Camera::calculateSphericalCoords() {
    // Calcula o vetor direção: da câmera para o ponto lookAt
    float dirX = lookAtX - posX;
    float dirY = lookAtY - posY;
    float dirZ = lookAtZ - posZ;
    
    // Calcula o raio (distância) entre a câmera e o lookAt usando Pitágoras
    radius = sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
    
    // Calcula alpha (ângulo horizontal/azimute) usando atan2
    // atan2(y, x) retorna o ângulo em radianos entre -π e π
    // Posiciona a câmera em relação ao eixo Z da origem (lookAt)
    alpha = atan2(posX - lookAtX, posZ - lookAtZ);
    
    // Calcula beta (ângulo vertical/elevação) usando asin
    // Representa a altura da câmera em relação ao plano horizontal que passa por lookAt
    beta = asin((posY - lookAtY) / radius);
}

void Camera::spherical2Cartesian() {
    // Converte coordenadas esféricas para cartesianas
    // Mantendo lookAt como centro de rotação
    
    posX = lookAtX + radius * sin(alpha) * cos(beta);
    posY = lookAtY + radius * sin(beta);
    posZ = lookAtZ + radius * cos(alpha) * cos(beta);
}


void Camera::setPosition(float x, float y, float z) {
    posX = x;
    posY = y;
    posZ = z;
    calculateSphericalCoords();
}

void Camera::setLookAt(float x, float y, float z) {
    lookAtX = x;
    lookAtY = y;
    lookAtZ = z;
    calculateSphericalCoords();
}

void Camera::setUp(float x, float y, float z) {
    upX = x;
    upY = y;
    upZ = z;
}

void Camera::setProjection(float fovVal, float near, float far) {
    fov = fovVal;
    nearPlane = near;
    farPlane = far;
}


void Camera::rotateLeft() {
    alpha -= ROTATION_INCREMENT;
    spherical2Cartesian();
}

void Camera::rotateRight() {
    alpha += ROTATION_INCREMENT;
    spherical2Cartesian();
}

void Camera::rotateUp() {
    beta += ROTATION_INCREMENT;
    
    // Limita a rotação vertical para evitar que a câmera "inverta"
    // quando atinge o topo (π/2)
    if (beta >= M_PI / 2) {
        beta = M_PI / 2 - BETA_MARGIN;
    }
    
    spherical2Cartesian();
}

void Camera::rotateDown() {
    beta -= ROTATION_INCREMENT;
    
    // Limita a rotação vertical para evitar inversão na base (-π/2)
    if (beta <= -M_PI / 2) {
        beta = -M_PI / 2 + BETA_MARGIN;
    }
    
    spherical2Cartesian();
}

// ==================== Controles de Zoom ====================

void Camera::zoomIn() {
    radius -= ZOOM_INCREMENT;
    
    // Previne que o raio fique muito pequeno ou negativo
    if (radius < MIN_RADIUS) {
        radius = MIN_RADIUS;
    }
    
    spherical2Cartesian();
}

void Camera::zoomOut() {
    radius += ZOOM_INCREMENT;
    spherical2Cartesian();
}

// ==================== Renderização ====================

void Camera::place() {
    // Aplica a transformação de câmera usando a função gluLookAt do OpenGL
    // Esta função define a matriz de visualização (view matrix) com base em:
    // - Posição da câmera (posX, posY, posZ)
    // - Ponto para o qual está olhando (lookAtX, lookAtY, lookAtZ)
    // - Vetor "para cima" (upX, upY, upZ)
    gluLookAt(posX, posY, posZ,
              lookAtX, lookAtY, lookAtZ,
              upX, upY, upZ);
}