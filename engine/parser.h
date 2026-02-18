#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "camera.h"
#include "tinyxml2.h"

struct Window {
    int width, height;
    
    Window() : width(800), height(600) {}
};

struct Model {
    std::string filename;
};

struct Group {
    std::vector<Model> models;
};

class SimpleParser {
public:
    static bool parseXMLFile(const std::string& filename, Window& window, Camera& camera, Group& group);
    
private:
    static void parseModels(tinyxml2::XMLElement* modelsElement, Group& group);
};