#pragma once
#define _USE_MATH_DEFINES
#include <math.h>

class Camera {
private:
    // Camera position
    float posX, posY, posZ;
    
    // LookAt point
    float lookAtX, lookAtY, lookAtZ;
    
    // Up vector
    float upX, upY, upZ;
    
    // Projection parameters
    float fov, nearPlane, farPlane;
    
    // Spherical coordinates
    float alpha, beta, radius;

public:
    // Constructor
    Camera();
    
    // Constructor with parameters
    Camera(float px, float py, float pz, 
           float lx, float ly, float lz, 
           float ux, float uy, float uz, 
           float fov, float near, float far);
    
    // Getters
    float getPosX() const { return posX; }
    float getPosY() const { return posY; }
    float getPosZ() const { return posZ; }
    
    float getLookAtX() const { return lookAtX; }
    float getLookAtY() const { return lookAtY; }
    float getLookAtZ() const { return lookAtZ; }
    
    float getUpX() const { return upX; }
    float getUpY() const { return upY; }
    float getUpZ() const { return upZ; }
    
    float getFov() const { return fov; }
    float getNearPlane() const { return nearPlane; }
    float getFarPlane() const { return farPlane; }
    
    // Setters
    void setPosition(float x, float y, float z);
    void setLookAt(float x, float y, float z);
    void setUp(float x, float y, float z);
    void setProjection(float fov, float near, float far);
    
    // Calculate spherical coordinates from camera position
    void calculateSphericalCoords();
    
    // Update camera position based on spherical coordinates
    void spherical2Cartesian();
    
    // Movement methods
    void rotateLeft();
    void rotateRight();
    void rotateUp();
    void rotateDown();
    void zoomIn();
    void zoomOut();
    
    // Place the camera (to be called in the rendering loop)
    void place();
};