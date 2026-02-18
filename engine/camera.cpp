#define _USE_MATH_DEFINES
#include "camera.h"
#include <math.h>
#include <GL/glut.h>
#include <iostream>

// Default constructor

Camera::Camera() {
    // Default camera position
    posX = 0.0f; posY = 0.0f; posZ = 5.0f;
    
    // Default lookAt point (origin)
    lookAtX = 0.0f; lookAtY = 0.0f; lookAtZ = 0.0f;
    
    // Default up vector
    upX = 0.0f; upY = 1.0f; upZ = 0.0f;
    
    // Default projection parameters
    fov = 60.0f; nearPlane = 1.0f; farPlane = 1000.0f;
    
    // Calculate initial spherical coordinates
    calculateSphericalCoords();
}

// Constructor with parameters
Camera::Camera(float px, float py, float pz, 
               float lx, float ly, float lz, 
               float ux, float uy, float uz, 
               float fovVal, float near, float far) {
    
    setPosition(px, py, pz);
    setLookAt(lx, ly, lz);
    setUp(ux, uy, uz);
    setProjection(fovVal, near, far);
    
    // Calculate initial spherical coordinates
    calculateSphericalCoords();
}

// Calculate spherical coordinates from current camera position
void Camera::calculateSphericalCoords() {
    // Calculate direction vector
    float dirX = lookAtX - posX;
    float dirY = lookAtY - posY;
    float dirZ = lookAtZ - posZ;
    
    // Utiliza o teorema de pitagoras para calcular a distancia entre a camera e o lookAt.
    radius = sqrt(pow(dirX, 2) + pow(dirY, 2) + pow(dirZ, 2));
    
    // Calcula o ângulo horizontal (alpha) usando a função atan2, que retorna o ângulo em radianos entre o eixo X positivo e um ponto (x,z).
    alpha = atan2(posX - lookAtX, posZ - lookAtZ);
    
    // Calcula o ângulo vertical (beta) usando a função arco seno. Este ângulo representa a elevação da câmera em relação ao plano horizontal.
    beta = asin((posY - lookAtY) / radius);
}

// Update camera position based on spherical coordinates
void Camera::spherical2Cartesian() {
    // converte as coordenadas esféricas (alpha, beta, radius) para coordenadas cartesianas (x, y, z) e atualiza a posição da câmera.
    posX = lookAtX + radius * sin(alpha) * cos(beta);
    posY = lookAtY + radius * sin(beta);
    posZ = lookAtZ + radius * cos(alpha) * cos(beta);
}

// Set camera position
void Camera::setPosition(float x, float y, float z) {
    posX = x;
    posY = y;
    posZ = z;
    calculateSphericalCoords();
}

// Set lookAt point
void Camera::setLookAt(float x, float y, float z) {
    lookAtX = x;
    lookAtY = y;
    lookAtZ = z;
    calculateSphericalCoords();
}

// Set up vector
void Camera::setUp(float x, float y, float z) {
    upX = x;
    upY = y;
    upZ = z;
}

// Set projection parameters
void Camera::setProjection(float fovVal, float near, float far) {
    fov = fovVal;
    nearPlane = near;
    farPlane = far;
}

// Rotate camera to the left
void Camera::rotateLeft() {
    alpha -= 0.1f;
    spherical2Cartesian();
}

// Rotate camera to the right
void Camera::rotateRight() {
    alpha += 0.1f;
    spherical2Cartesian();
}

// Rotate camera up
void Camera::rotateUp() {
    beta += 0.1f;
    if (beta >= M_PI / 2) beta = M_PI / 2 - 0.01f; // Prevent camera flip
    spherical2Cartesian();
}

// Rotate camera down
void Camera::rotateDown() {
    beta -= 0.1f;
    if (beta <= -M_PI / 2) beta = -M_PI / 2 + 0.01f; // Prevent camera flip
    spherical2Cartesian();
}

// Zoom in (decrease radius)
void Camera::zoomIn() {
    radius -= 0.1f;
    if (radius < 0.1f) radius = 0.1f; // Prevent negative or zero radius
    spherical2Cartesian();
}

// Zoom out (increase radius)
void Camera::zoomOut() {
    radius += 0.1f;
    spherical2Cartesian();
}

// Place the camera in the scene (to be called in the rendering loop)
void Camera::place() {
    gluLookAt(posX, posY, posZ,
              lookAtX, lookAtY, lookAtZ,
              upX, upY, upZ);
}




/*
Ou seja neste código nós trabalhamos sempre com coordenadas esféricas, e convertemos para coordenadas cartesianas apenas quando necessário.
As coordenadas esféricas são o beta que é o ângulo vertical e o alpha que é o ângulo horizontal, e o raio que é a distância entre a câmera e o lookAt.
Isto, porque como temos de rodar a camara em torno do lookAt, é mais fácil trabalhar com coordenadas esféricas.
*/