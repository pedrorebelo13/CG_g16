#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "camera.h"
#include "tinyxml2.h"

// Parser da Fase 3:
// - Mantém suporte da Fase 2 (hierarquia de grupos + transformações estáticas)
// - Acrescenta transformações temporais (rotate/translate com atributo time)
// - Para translate animado, armazena pontos de controlo para Catmull-Rom

// Estrutura que guarda as dimensoes da janela
struct Window {
    int width;
    int height;
    Window() : width(800), height(600) {}
};

// Estrutura para os modelos 3D
struct Model {
    std::string filename;
};

enum TransformType {
    TRANSLATE,          // Translação estática
    TRANSLATE_ANIM,     // Translação animada sobre curva Catmull-Rom
    ROTATE,             // Rotação estática
    ROTATE_ANIM,        // Rotação animada (tempo em segundos para 360°)
    SCALE
};

// Ponto 3D (usado nos pontos de controlo Catmull-Rom)
struct Point3D {
    float x, y, z;
    Point3D() : x(0), y(0), z(0) {}
    Point3D(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Transformation {
    TransformType type;

    // Translação estática / Scale / Rotate estático
    float x, y, z;
    float angle;        // Ângulo de rotação (graus), 0 nos outros casos

    // Animação temporal
    // Convenção usada no engine:
    // - ROTATE_ANIM: tempo para 360 graus
    // - TRANSLATE_ANIM: tempo para percorrer a curva completa (loop)
    float time;

    // Translação animada (Catmull-Rom)
    bool align;                     // Se true, alinha o objeto à tangente da curva
    std::vector<Point3D> points;    // Pontos de controlo da curva

    // Construtor para translação / scale estáticos
    Transformation(TransformType t, float xVal, float yVal, float zVal, float angleVal = 0)
        : type(t), x(xVal), y(yVal), z(zVal), angle(angleVal), time(0), align(false) {}

    // Construtor para rotação animada
    static Transformation makeRotateAnim(float timeVal, float xVal, float yVal, float zVal) {
        Transformation t(ROTATE_ANIM, xVal, yVal, zVal, 0);
        t.time = timeVal;
        return t;
    }

    // Construtor para translação animada
    static Transformation makeTranslateAnim(float timeVal, bool alignVal,
                                            const std::vector<Point3D>& pts) {
        Transformation t(TRANSLATE_ANIM, 0, 0, 0, 0);
        t.time = timeVal;
        t.align = alignVal;
        t.points = pts;
        return t;
    }
};

// Estrutura para os grupos
struct Group {
    std::vector<Transformation> transformations;
    std::vector<Model> models;
    std::vector<Group> children;
};

// Classe para ler o XML
class SimpleParser {
public:
    // Função principal de entrada: faz parse de window/camera e da árvore de grupos.
    static bool parseXMLFile(const std::string& filename,
                             Window& window,
                             Camera& camera,
                             Group& rootGroup);

private:
    static void parseWindow(tinyxml2::XMLElement* windowElement, Window& window);
    static void parseCamera(tinyxml2::XMLElement* cameraElement, Camera& camera);
    static Group parseGroup(tinyxml2::XMLElement* groupElement);
    static void parseTransforms(tinyxml2::XMLElement* transformElement, Group& group);
    static void parseModels(tinyxml2::XMLElement* modelsElement, Group& group);
};