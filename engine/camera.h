#pragma once
#define _USE_MATH_DEFINES
#include <math.h>

/**
 * @class Camera
 * @brief Gerencia a câmera 3D da cena, controlando posição, orientação e projeção.
 *
 * Esta classe implementa um sistema de câmera utilizando coordenadas esféricas para
 * facilitar rotações em torno de um ponto central (lookAt). As coordenadas esféricas
 * (alpha, beta, radius) são convertidas para coordenadas cartesianas quando necessário.
 *
 * Coordenadas Esféricas:
 * - alpha: ângulo horizontal em radianos
 * - beta: ângulo vertical (elevação) em radianos, limitado a [-π/2, π/2]
 * - radius: distância entre a câmera e o ponto de interesse (lookAt)
 */
class Camera {
private:
    // Posição da câmera em coordenadas cartesianas
    float posX, posY, posZ;
    
    // Ponto para o qual a câmera está olhando
    float lookAtX, lookAtY, lookAtZ;
    
    // Vetor "para cima" (up vector) - define a orientação vertical da câmera
    float upX, upY, upZ;
    
    // Parâmetros de projeção (perspectiva)
    float fov;       ///< Campo de visão (Field of View) em graus
    float nearPlane; ///< Plano de clipping próximo
    float farPlane;  ///< Plano de clipping distante
    
    // Coordenadas esféricas - utilizadas para facilitar as rotações
    float alpha;  ///< Ângulo horizontal (azimute) em radianos
    float beta;   ///< Ângulo vertical (elevação) em radianos
    float radius; ///< Distância da câmera ao lookAt

    /**
     * @brief Converte as coordenadas esféricas (alpha, beta, radius) para cartesianas.
     *
     * Esta função atualiza posX, posY, posZ com base nos valores esféricos atuais,
     * mantendo o ponto lookAt como centro de rotação. É chamada automaticamente
     * após qualquer operação que modifique as coordenadas esféricas.
     */
    void spherical2Cartesian();

public:
    /**
     * @brief Construtor padrão da câmera.
     *
     * Inicializa a câmera com valores padrão:
     * - Posição: (0, 0, 5)
     * - LookAt: (0, 0, 0) - origem
     * - Up vector: (0, 1, 0) - eixo Y
     * - Projeção: FOV=60°, near=1, far=1000
     */
    Camera();
    
    /**
     * @brief Construtor parametrizado da câmera.
     *
     * @param px, py, pz Posição inicial da câmera
     * @param lx, ly, lz Ponto para o qual a câmera está olhando
     * @param ux, uy, uz Vetor "para cima"
     * @param fovVal Campo de visão em graus
     * @param near Plano de clipping próximo
     * @param far Plano de clipping distante
     */
    Camera(float px, float py, float pz, 
           float lx, float ly, float lz, 
           float ux, float uy, float uz, 
           float fovVal, float near, float far);
    
    
    /// @brief Retorna a coordenada X da posição da câmera
    float getPosX() const { return posX; }
    /// @brief Retorna a coordenada Y da posição da câmera
    float getPosY() const { return posY; }
    /// @brief Retorna a coordenada Z da posição da câmera
    float getPosZ() const { return posZ; }
    
    /// @brief Retorna a coordenada X do ponto lookAt
    float getLookAtX() const { return lookAtX; }
    /// @brief Retorna a coordenada Y do ponto lookAt
    float getLookAtY() const { return lookAtY; }
    /// @brief Retorna a coordenada Z do ponto lookAt
    float getLookAtZ() const { return lookAtZ; }
    
    /// @brief Retorna a coordenada X do vetor "para cima"
    float getUpX() const { return upX; }
    /// @brief Retorna a coordenada Y do vetor "para cima"
    float getUpY() const { return upY; }
    /// @brief Retorna a coordenada Z do vetor "para cima"
    float getUpZ() const { return upZ; }
    
    /// @brief Retorna o campo de visão em graus
    float getFov() const { return fov; }
    /// @brief Retorna a distância do plano de clipping próximo
    float getNearPlane() const { return nearPlane; }
    /// @brief Retorna a distância do plano de clipping distante
    float getFarPlane() const { return farPlane; }
    
    /// @brief Retorna o ângulo horizontal (azimute) em radianos
    float getAlpha() const { return alpha; }
    /// @brief Retorna o ângulo vertical (elevação) em radianos
    float getBeta() const { return beta; }
    /// @brief Retorna a distância da câmera ao ponto lookAt
    float getRadius() const { return radius; }
    
    

    /**
     * @brief Define a posição da câmera.
     *
     * @param x, y, z Novas coordenadas cartesianas da câmera
     *
     * @note Recalcula automaticamente as coordenadas esféricas
     */
    void setPosition(float x, float y, float z);
    
    /**
     * @brief Define o ponto para o qual a câmera está olhando.
     *
     * @param x, y, z Novas coordenadas do ponto lookAt
     *
     * @note Recalcula automaticamente as coordenadas esféricas
     */
    void setLookAt(float x, float y, float z);
    
    /**
     * @brief Define o vetor "para cima" da câmera.
     *
     * @param x, y, z Componentes do novo vetor up
     *
     * @note Não recalcula coordenadas esféricas (parâmetro independente)
     */
    void setUp(float x, float y, float z);
    
    /**
     * @brief Define os parâmetros de projeção perspectiva.
     *
     * @param fovVal Campo de visão em graus
     * @param near Distância do plano de clipping próximo
     * @param far Distância do plano de clipping distante
     */
    void setProjection(float fovVal, float near, float far);
    
   
    
    /**
     * @brief Calcula as coordenadas esféricas a partir da posição cartesiana.
     *
     * Converte a diferença entre a posição atual e o ponto lookAt
     * para coordenadas esféricas (alpha, beta, radius).
     *
     * @note Chamada automaticamente pelos setters de posição e lookAt
     */
    void calculateSphericalCoords();
    
    
    /**
     * @brief Rotaciona a câmera para a esquerda (diminui alpha).
     *
     * A rotação é realizada em torno do eixo vertical (Y) passando pelo ponto lookAt.
     */
    void rotateLeft();
    
    /**
     * @brief Rotaciona a câmera para a direita (aumenta alpha).
     *
     * A rotação é realizada em torno do eixo vertical (Y) passando pelo ponto lookAt.
     */
    void rotateRight();
    
    /**
     * @brief Rotaciona a câmera para cima (aumenta beta).
     *
     * A rotação máxima é limitada a π/2 para evitar inversão da câmera.
     */
    void rotateUp();
    
    /**
     * @brief Rotaciona a câmera para baixo (diminui beta).
     *
     * A rotação mínima é limitada a -π/2 para evitar inversão da câmera.
     */
    void rotateDown();
    
    /**
     * @brief Aproxima a câmera do ponto lookAt (diminui o radius).
     *
     * O radius é limitado a um mínimo de 0.1 para evitar valores inválidos.
     */
    void zoomIn();
    
    /**
     * @brief Afasta a câmera do ponto lookAt (aumenta o radius).
     */
    void zoomOut();
    
    
    /**
     * @brief Aplica a transformação da câmera no contexto OpenGL.
     *
     * Deve ser chamada uma vez por frame, antes de desenhar os objetos da cena.
     * Utiliza gluLookAt() para posicionar a câmera com base em seus parâmetros atuais.
     */
    void place();
};