#include "parser.h"
#include <iostream>
#include <cstring>

using namespace std;
using namespace tinyxml2;


bool SimpleParser::parseXMLFile(const std::string& filename,
                                Window& window,
                                Camera& camera,
                                Group& group) {
    // O XML é carregado apenas uma vez no arranque;
    // depois o engine usa as estruturas em memória durante todo o loop render.
    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS) {
        cerr << "Erro ao carregar arquivo XML: " << filename << endl;
        return false;
    }

    XMLElement* worldElement = doc.FirstChildElement("world");
    if (!worldElement) {
        cerr << "Erro: elemento 'world' não encontrado" << endl;
        return false;
    }

    XMLElement* windowElement = worldElement->FirstChildElement("window");
    parseWindow(windowElement, window);

    XMLElement* cameraElement = worldElement->FirstChildElement("camera");
    parseCamera(cameraElement, camera);

    XMLElement* groupElement = worldElement->FirstChildElement("group");
    if (groupElement) {
        group = parseGroup(groupElement);
    } else {
        cerr << "Aviso: elemento 'group' não encontrado" << endl;
    }

    return true;
}


Group SimpleParser::parseGroup(XMLElement* groupElement) {
    Group group;
    if (!groupElement) return group;

    XMLElement* transformElement = groupElement->FirstChildElement("transform");
    if (transformElement) {
        parseTransforms(transformElement, group);
    }

    XMLElement* modelsElement = groupElement->FirstChildElement("models");
    if (modelsElement) {
        parseModels(modelsElement, group);
    }

    XMLElement* childGroupElement = groupElement->FirstChildElement("group");
    while (childGroupElement) {
        // Parse recursivo: preserva a hierarquia pai->filho para herança de transformações.
        group.children.push_back(parseGroup(childGroupElement));
        childGroupElement = childGroupElement->NextSiblingElement("group");
    }

    return group;
}


void SimpleParser::parseTransforms(XMLElement* transformElement, Group& group) {
    if (!transformElement) return;

    // A ordem em que os nós aparecem no XML é preservada aqui,
    // porque a multiplicação de matrizes não é comutativa.
    XMLElement* node = transformElement->FirstChildElement();
    while (node) {
        const char* tag = node->Value();

        // ── TRANSLATE ────────────────────────────────────────────────────────
        if (strcmp(tag, "translate") == 0) {
            float timeVal = 0.0f;
            // Verifica se tem atributo "time" → translação animada
            if (node->QueryFloatAttribute("time", &timeVal) == XML_SUCCESS && timeVal > 0) {
                // Lê alinhamento (opcional, default false)
                bool alignVal = false;
                const char* alignStr = node->Attribute("align");
                if (alignStr && (strcmp(alignStr, "True") == 0 ||
                                 strcmp(alignStr, "true") == 0 ||
                                 strcmp(alignStr, "1") == 0)) {
                    alignVal = true;
                }

                // Lê os pontos de controlo da curva Catmull-Rom
                vector<Point3D> pts;
                XMLElement* ptNode = node->FirstChildElement("point");
                while (ptNode) {
                    float px = 0, py = 0, pz = 0;
                    ptNode->QueryFloatAttribute("x", &px);
                    ptNode->QueryFloatAttribute("y", &py);
                    ptNode->QueryFloatAttribute("z", &pz);
                    pts.push_back(Point3D(px, py, pz));
                    ptNode = ptNode->NextSiblingElement("point");
                }

                // Catmull-Rom precisa de pelo menos 4 pontos para definir segmentos válidos.
                if (pts.size() >= 4) {
                    group.transformations.push_back(
                        Transformation::makeTranslateAnim(timeVal, alignVal, pts));
                } else {
                    cerr << "Aviso: translate animado precisa de pelo menos 4 pontos" << endl;
                }
            } else {
                // Translação estática normal
                float x = 0.0f, y = 0.0f, z = 0.0f;
                node->QueryFloatAttribute("x", &x);
                node->QueryFloatAttribute("y", &y);
                node->QueryFloatAttribute("z", &z);
                group.transformations.push_back(Transformation(TRANSLATE, x, y, z));
            }

        // ── ROTATE ───────────────────────────────────────────────────────────
        } else if (strcmp(tag, "rotate") == 0) {
            float timeVal = 0.0f;
            // Verifica se tem atributo "time" → rotação animada
            if (node->QueryFloatAttribute("time", &timeVal) == XML_SUCCESS && timeVal > 0) {
                float x = 0.0f, y = 0.0f, z = 0.0f;
                node->QueryFloatAttribute("x", &x);
                node->QueryFloatAttribute("y", &y);
                node->QueryFloatAttribute("z", &z);
                group.transformations.push_back(
                    Transformation::makeRotateAnim(timeVal, x, y, z));
            } else {
                // Rotação estática normal
                float angle = 0.0f, x = 0.0f, y = 0.0f, z = 0.0f;
                node->QueryFloatAttribute("angle", &angle);
                node->QueryFloatAttribute("x", &x);
                node->QueryFloatAttribute("y", &y);
                node->QueryFloatAttribute("z", &z);
                group.transformations.push_back(Transformation(ROTATE, x, y, z, angle));
            }

        // ── SCALE ────────────────────────────────────────────────────────────
        } else if (strcmp(tag, "scale") == 0) {
            float x = 1.0f, y = 1.0f, z = 1.0f;
            node->QueryFloatAttribute("x", &x);
            node->QueryFloatAttribute("y", &y);
            node->QueryFloatAttribute("z", &z);
            group.transformations.push_back(Transformation(SCALE, x, y, z));
        }

        node = node->NextSiblingElement();
    }
}


void SimpleParser::parseWindow(XMLElement* windowElement, Window& window) {
    if (!windowElement) {
        cerr << "Aviso: 'window' não encontrado, usando padrão (800x600)" << endl;
        return;
    }
    int width = window.width, height = window.height;
    windowElement->QueryIntAttribute("width", &width);
    windowElement->QueryIntAttribute("height", &height);
    if (width < 100 || height < 100) {
        cerr << "Aviso: dimensões inválidas, usando padrão" << endl;
        return;
    }
    window.width = width;
    window.height = height;
    cout << "Janela: " << window.width << "x" << window.height << endl;
}


void SimpleParser::parseCamera(XMLElement* cameraElement, Camera& camera) {
    if (!cameraElement) {
        cerr << "Aviso: 'camera' não encontrado, usando câmera padrão" << endl;
        return;
    }
    float posX = 0, posY = 0, posZ = 5;
    XMLElement* el = cameraElement->FirstChildElement("position");
    if (el) {
        el->QueryFloatAttribute("x", &posX);
        el->QueryFloatAttribute("y", &posY);
        el->QueryFloatAttribute("z", &posZ);
    }
    float lookX = 0, lookY = 0, lookZ = 0;
    el = cameraElement->FirstChildElement("lookAt");
    if (el) {
        el->QueryFloatAttribute("x", &lookX);
        el->QueryFloatAttribute("y", &lookY);
        el->QueryFloatAttribute("z", &lookZ);
    }
    float upX = 0, upY = 1, upZ = 0;
    el = cameraElement->FirstChildElement("up");
    if (el) {
        el->QueryFloatAttribute("x", &upX);
        el->QueryFloatAttribute("y", &upY);
        el->QueryFloatAttribute("z", &upZ);
    }
    float fov = 60, near = 1, far = 1000;
    el = cameraElement->FirstChildElement("projection");
    if (el) {
        el->QueryFloatAttribute("fov", &fov);
        el->QueryFloatAttribute("near", &near);
        el->QueryFloatAttribute("far", &far);
    }
    camera.setPosition(posX, posY, posZ);
    camera.setLookAt(lookX, lookY, lookZ);
    camera.setUp(upX, upY, upZ);
    camera.setProjection(fov, near, far);
}


void SimpleParser::parseModels(XMLElement* modelsElement, Group& group) {
    if (!modelsElement) return;

    // Cada grupo mantém os seus próprios modelos;
    // o engine percorre a árvore e aplica a transformação acumulada antes de desenhar.
    XMLElement* modelElement = modelsElement->FirstChildElement("model");
    while (modelElement) {
        const char* filename = modelElement->Attribute("file");
        if (filename) {
            Model model;
            model.filename = string(filename);
            group.models.push_back(model);
            cout << "Modelo: " << model.filename << endl;
        }
        modelElement = modelElement->NextSiblingElement("model");
    }
}