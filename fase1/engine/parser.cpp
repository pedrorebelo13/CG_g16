#include "parser.h"
#include <iostream>

using namespace std;
using namespace tinyxml2;


bool SimpleParser::parseXMLFile(const std::string& filename, 
                               Window& window, 
                               Camera& camera, 
                               Group& group) {
    // Carrega o arquivo XML em memória
    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS) {
        cerr << "Erro ao carregar arquivo XML: " << filename << endl;
        return false;
    }

    // Obtém o elemento raiz <world>
    XMLElement* worldElement = doc.FirstChildElement("world");
    if (!worldElement) {
        cerr << "Erro: elemento 'world' não encontrado no arquivo XML" << endl;
        return false;
    }

    // Parse de janela
    XMLElement* windowElement = worldElement->FirstChildElement("window");
    parseWindow(windowElement, window);
    
    // Parse de câmera
    XMLElement* cameraElement = worldElement->FirstChildElement("camera");
    parseCamera(cameraElement, camera);

    // Parse de grupo e modelos
    XMLElement* groupElement = worldElement->FirstChildElement("group");
    if (groupElement) {
        XMLElement* modelsElement = groupElement->FirstChildElement("models");
        if (modelsElement) {
            parseModels(modelsElement, group);
        } else {
            cerr << "Aviso: elemento 'models' não encontrado no grupo" << endl;
        }
    } else {
        cerr << "Aviso: elemento 'group' não encontrado" << endl;
    }

    return true;
}


void SimpleParser::parseWindow(XMLElement* windowElement, Window& window) {
    // Se o elemento não existir, mantém valores padrão
    if (!windowElement) {
        cerr << "Aviso: elemento 'window' não encontrado, usando padrão (800x600)" << endl;
        return;
    }

    // Extrai atributos width e height
    int width = window.width;
    int height = window.height;
    
    windowElement->QueryIntAttribute("width", &width);
    windowElement->QueryIntAttribute("height", &height);
    
    // Valida dimensões (mínimo 100x100)
    if (width < 100 || height < 100) {
        cerr << "Aviso: dimensões da janela inválidas, usando padrão" << endl;
        return;
    }
    
    window.width = width;
    window.height = height;
    
    cout << "Janela configurada: " << window.width << "x" << window.height << endl;
}


void SimpleParser::parseCamera(XMLElement* cameraElement, Camera& camera) {
    // Se o elemento câmera não existir, usa valores padrão da câmera
    if (!cameraElement) {
        cerr << "Aviso: elemento 'camera' não encontrado, usando câmera padrão" << endl;
        return;
    }

    // Valores padrão para posição
    float posX = 0, posY = 0, posZ = 5;
    XMLElement* positionElement = cameraElement->FirstChildElement("position");
    if (positionElement) {
        positionElement->QueryFloatAttribute("x", &posX);
        positionElement->QueryFloatAttribute("y", &posY);
        positionElement->QueryFloatAttribute("z", &posZ);
        cout << "Posição câmera: (" << posX << ", " << posY << ", " << posZ << ")" << endl;
    }

    // Valores padrão para ponto de interesse
    float lookX = 0, lookY = 0, lookZ = 0;
    XMLElement* lookAtElement = cameraElement->FirstChildElement("lookAt");
    if (lookAtElement) {
        lookAtElement->QueryFloatAttribute("x", &lookX);
        lookAtElement->QueryFloatAttribute("y", &lookY);
        lookAtElement->QueryFloatAttribute("z", &lookZ);
        cout << "LookAt câmera: (" << lookX << ", " << lookY << ", " << lookZ << ")" << endl;
    }

    // Valores padrão para vetor "para cima"
    float upX = 0, upY = 1, upZ = 0;
    XMLElement* upElement = cameraElement->FirstChildElement("up");
    if (upElement) {
        upElement->QueryFloatAttribute("x", &upX);
        upElement->QueryFloatAttribute("y", &upY);
        upElement->QueryFloatAttribute("z", &upZ);
        cout << "Vetor 'up' câmera: (" << upX << ", " << upY << ", " << upZ << ")" << endl;
    }

    // Valores padrão para projeção
    float fov = 60, near = 1, far = 1000;
    XMLElement* projectionElement = cameraElement->FirstChildElement("projection");
    if (projectionElement) {
        projectionElement->QueryFloatAttribute("fov", &fov);
        projectionElement->QueryFloatAttribute("near", &near);
        projectionElement->QueryFloatAttribute("far", &far);
        cout << "Projeção câmera - FOV: " << fov << "°, Near: " << near << ", Far: " << far << endl;
    }

    // Aplica todas as configurações à câmera
    camera.setPosition(posX, posY, posZ);
    camera.setLookAt(lookX, lookY, lookZ);
    camera.setUp(upX, upY, upZ);
    camera.setProjection(fov, near, far);
}


void SimpleParser::parseModels(XMLElement* modelsElement, Group& group) {
    // Valida entrada
    if (!modelsElement) {
        cerr << "Erro: elemento 'models' inválido" << endl;
        return;
    }
    
    // Limpa lista anterior de modelos
    group.models.clear();
    
    // Itera sobre todos os elementos <model>
    XMLElement* modelElement = modelsElement->FirstChildElement("model");
    int modelCount = 0;
    
    while (modelElement) {
        // Extrai o atributo "file" de cada modelo
        const char* filename = modelElement->Attribute("file");
        
        if (filename) {
            Model model;
            model.filename = std::string(filename);
            group.models.push_back(model);
            
            cout << "Modelo encontrado: " << model.filename << endl;
            modelCount++;
        } else {
            cerr << "Aviso: elemento <model> sem atributo 'file'" << endl;
        }
        
        // Próximo elemento <model>
        modelElement = modelElement->NextSiblingElement("model");
    }
    
    if (modelCount == 0) {
        cerr << "Aviso: nenhum modelo encontrado no arquivo XML" << endl;
    } else {
        cout << "Total de modelos a carregar: " << modelCount << endl;
    }
}