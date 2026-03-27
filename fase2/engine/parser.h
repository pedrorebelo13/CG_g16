#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "camera.h"
#include "tinyxml2.h"

// Estrutura que guarda as dimensoes da janela
struct Window {
    int width;  // Largura em pixeis
    int height; // Altura em pixeis
    
    // Default 800x600
    Window() : width(800), height(600) {}
};

// Estrutura para os modelos 3D
struct Model {
    std::string filename; // ficheiro .3d para carregar
};

enum TransformType {
    TRANSLATE,
    ROTATE,
    SCALE
};

struct Transformation {
    TransformType type; // translate, rotate ou scale
    float x, y, z;      // valores da transformacao
    float angle;        // angulo para o rotate, 0 nos outros casos

    //construtor para translate e scale
     Transformation(TransformType t, float xVal, float yVal, float zVal)
        : type(t), x(xVal), y(yVal), z(zVal), angle(0) {}

    //construtor para rotate
    Transformation(TransformType t, float xVal, float yVal, float zVal, float angleVal = 0)
        : type(t), x(xVal), y(yVal), z(zVal), angle(angleVal) {}

    
};

// Estrutura para os grupos
struct Group {
    std::vector<Transformation> transformations; // Lista de transformacoes a aplicar ao grupo
    std::vector<Model> models; // Lista dos modelos a desenhar
    std::vector<Group> children; // Subgrupos
};

// Classe para ler o XML
class SimpleParser {
public:
    // Faz o parse principal do ficheiro
    static bool parseXMLFile(const std::string& filename, 
                            Window& window, 
                            Camera& camera, 
                            Group& rootGroup);
    
private:
    // Le a tag da janela
    static void parseWindow(tinyxml2::XMLElement* windowElement, Window& window);

    // Le a tag da camara
    static void parseCamera(tinyxml2::XMLElement* cameraElement, Camera& camera);

    // Le os grupos de forma recursiva (para lidar com os subgrupos)
    static Group parseGroup(tinyxml2::XMLElement* groupElement);
    
    // Le as transformacoes tendo atencao a ordem delas no ficheiro
    static void parseTransforms(tinyxml2::XMLElement* transformElement, Group& group);
    
    // Le a lista de modelos
    static void parseModels(tinyxml2::XMLElement* modelsElement, Group& group);
};