#include "parser.h"
#include <iostream>

using namespace std;
using namespace tinyxml2;

bool SimpleParser::parseXMLFile(const std::string& filename, Window& window, Camera& camera, Group& group) {
    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS) {
        cerr << "Error loading XML file: " << filename << endl;
        return false;
    }

    XMLElement* worldElement = doc.FirstChildElement("world");
    if (!worldElement) {
        cerr << "No 'world' element found in XML" << endl;
        return false;
    }

    XMLElement* windowElement = worldElement->FirstChildElement("window");
    if (windowElement) {
        windowElement->QueryIntAttribute("width", &window.width);
        windowElement->QueryIntAttribute("height", &window.height);
    }

    XMLElement* cameraElement = worldElement->FirstChildElement("camera");
    if (cameraElement) {
        float posX = 0, posY = 0, posZ = 0;
        float lookX = 0, lookY = 0, lookZ = 0;
        float upX = 0, upY = 1, upZ = 0;
        float fov = 60, near = 1, far = 1000;

        XMLElement* positionElement = cameraElement->FirstChildElement("position");
        if (positionElement) {
            positionElement->QueryFloatAttribute("x", &posX);
            positionElement->QueryFloatAttribute("y", &posY);
            positionElement->QueryFloatAttribute("z", &posZ);
        }

        XMLElement* lookAtElement = cameraElement->FirstChildElement("lookAt");
        if (lookAtElement) {
            lookAtElement->QueryFloatAttribute("x", &lookX);
            lookAtElement->QueryFloatAttribute("y", &lookY);
            lookAtElement->QueryFloatAttribute("z", &lookZ);
        }

        XMLElement* upElement = cameraElement->FirstChildElement("up");
        if (upElement) {
            upElement->QueryFloatAttribute("x", &upX);
            upElement->QueryFloatAttribute("y", &upY);
            upElement->QueryFloatAttribute("z", &upZ);
        }

        XMLElement* projectionElement = cameraElement->FirstChildElement("projection");
        if (projectionElement) {
            projectionElement->QueryFloatAttribute("fov", &fov);
            projectionElement->QueryFloatAttribute("near", &near);
            projectionElement->QueryFloatAttribute("far", &far);
        }

        camera.setPosition(posX, posY, posZ);
        camera.setLookAt(lookX, lookY, lookZ);
        camera.setUp(upX, upY, upZ);
        camera.setProjection(fov, near, far);
    }

    XMLElement* groupElement = worldElement->FirstChildElement("group");
    if (groupElement) {
        XMLElement* modelsElement = groupElement->FirstChildElement("models");
        if (modelsElement) {
            parseModels(modelsElement, group);
        }
    }

    return true;
}

void SimpleParser::parseModels(XMLElement* modelsElement, Group& group) {
    if (!modelsElement) return;
    
    XMLElement* modelElement = modelsElement->FirstChildElement("model");
    while (modelElement) {
        const char* filename = modelElement->Attribute("file");
        if (filename) {
            Model model;
            model.filename = std::string(filename);
            group.models.push_back(model);
            
            cout << "Model found: " << model.filename << endl;
        }
        modelElement = modelElement->NextSiblingElement("model");
    }
}