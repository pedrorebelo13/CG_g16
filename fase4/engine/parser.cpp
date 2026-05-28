#include "parser.h"
#include <iostream>
#include <cstring>

using namespace std;
using namespace tinyxml2;


bool SimpleParser::parseXMLFile(const std::string& filename,
                                Window& window,
                                Camera& camera,
                                Group& group) {
    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS) {
        cerr << "Erro ao carregar XML: " << filename << endl;
        return false;
    }
    XMLElement* world = doc.FirstChildElement("world");
    if (!world) { cerr << "Elemento 'world' não encontrado" << endl; return false; }

    parseWindow(world->FirstChildElement("window"), window);
    parseCamera(world->FirstChildElement("camera"), camera);

    // Luzes: lidas do <lights> diretamente dentro de <world>
    XMLElement* lightsEl = world->FirstChildElement("lights");
    if (lightsEl) parseLights(lightsEl, group);

    // Grupos (hierarquia da cena)
    XMLElement* groupEl = world->FirstChildElement("group");
    if (groupEl) group = parseGroup(groupEl);
    else cerr << "Aviso: 'group' não encontrado" << endl;

    // Relinkamos as luzes ao rootGroup após o parseGroup (que cria um novo Group)
    // Nota: parseGroup devolve um Group novo, por isso relemos as luzes depois
    if (lightsEl) parseLights(lightsEl, group);

    return true;
}


Group SimpleParser::parseGroup(XMLElement* el) {
    Group group;
    if (!el) return group;

    if (auto* t = el->FirstChildElement("transform"))  parseTransforms(t, group);
    if (auto* m = el->FirstChildElement("models"))     parseModels(m, group);

    XMLElement* child = el->FirstChildElement("group");
    while (child) {
        group.children.push_back(parseGroup(child));
        child = child->NextSiblingElement("group");
    }
    return group;
}


void SimpleParser::parseTransforms(XMLElement* el, Group& group) {
    if (!el) return;
    XMLElement* node = el->FirstChildElement();
    while (node) {
        const char* tag = node->Value();

        if (strcmp(tag, "translate") == 0) {
            float timeVal = 0.0f;
            if (node->QueryFloatAttribute("time", &timeVal) == XML_SUCCESS && timeVal > 0) {
                bool alignVal = false;
                const char* a = node->Attribute("align");
                if (a && (strcmp(a,"True")==0||strcmp(a,"true")==0||strcmp(a,"1")==0))
                    alignVal = true;
                vector<Point3D> pts;
                XMLElement* pt = node->FirstChildElement("point");
                while (pt) {
                    float px=0,py=0,pz=0;
                    pt->QueryFloatAttribute("x",&px);
                    pt->QueryFloatAttribute("y",&py);
                    pt->QueryFloatAttribute("z",&pz);
                    pts.push_back({px,py,pz});
                    pt = pt->NextSiblingElement("point");
                }
                if (pts.size() >= 4)
                    group.transformations.push_back(Transformation::makeTranslateAnim(timeVal,alignVal,pts));
            } else {
                float x=0,y=0,z=0;
                node->QueryFloatAttribute("x",&x);
                node->QueryFloatAttribute("y",&y);
                node->QueryFloatAttribute("z",&z);
                group.transformations.push_back(Transformation(TRANSLATE,x,y,z));
            }

        } else if (strcmp(tag, "rotate") == 0) {
            float timeVal=0;
            if (node->QueryFloatAttribute("time",&timeVal)==XML_SUCCESS && timeVal>0) {
                float x=0,y=0,z=0;
                node->QueryFloatAttribute("x",&x);
                node->QueryFloatAttribute("y",&y);
                node->QueryFloatAttribute("z",&z);
                group.transformations.push_back(Transformation::makeRotateAnim(timeVal,x,y,z));
            } else {
                float angle=0,x=0,y=0,z=0;
                node->QueryFloatAttribute("angle",&angle);
                node->QueryFloatAttribute("x",&x);
                node->QueryFloatAttribute("y",&y);
                node->QueryFloatAttribute("z",&z);
                group.transformations.push_back(Transformation(ROTATE,x,y,z,angle));
            }

        } else if (strcmp(tag, "scale") == 0) {
            float x=1,y=1,z=1;
            node->QueryFloatAttribute("x",&x);
            node->QueryFloatAttribute("y",&y);
            node->QueryFloatAttribute("z",&z);
            group.transformations.push_back(Transformation(SCALE,x,y,z));
        }
        node = node->NextSiblingElement();
    }
}


// Lê o material de cor de um elemento <model>.
// Valores R,G,B no XML estão em [0,255]; normalizamos para [0,1] para OpenGL.
Material SimpleParser::parseMaterial(XMLElement* modelEl) {
    Material mat;
    XMLElement* colorEl = modelEl->FirstChildElement("color");
    if (!colorEl) return mat;

    auto readRGB = [&](const char* tag, float* out) {
        XMLElement* el = colorEl->FirstChildElement(tag);
        if (!el) return;
        float r=out[0]*255, g=out[1]*255, b=out[2]*255;
        el->QueryFloatAttribute("R",&r);
        el->QueryFloatAttribute("G",&g);
        el->QueryFloatAttribute("B",&b);
        out[0]=r/255.0f; out[1]=g/255.0f; out[2]=b/255.0f; out[3]=1.0f;
    };

    readRGB("diffuse",  mat.diffuse);
    readRGB("ambient",  mat.ambient);
    readRGB("specular", mat.specular);
    readRGB("emissive", mat.emissive);

    XMLElement* shEl = colorEl->FirstChildElement("shininess");
    if (shEl) shEl->QueryFloatAttribute("value", &mat.shininess);

    return mat;
}


// Lê <models> — agora cada <model> pode ter <texture> e <color>
void SimpleParser::parseModels(XMLElement* modelsEl, Group& group) {
    if (!modelsEl) return;
    XMLElement* modelEl = modelsEl->FirstChildElement("model");
    while (modelEl) {
        const char* file = modelEl->Attribute("file");
        if (file) {
            Model model;
            model.filename = string(file);

            // Textura (opcional)
            XMLElement* texEl = modelEl->FirstChildElement("texture");
            if (texEl) {
                const char* tf = texEl->Attribute("file");
                if (tf) model.textureFile = string(tf);
            }

            // Material de cor (opcional; usa defaults se não especificado)
            model.material = parseMaterial(modelEl);

            group.models.push_back(model);
            cout << "Modelo: " << model.filename;
            if (!model.textureFile.empty()) cout << " (textura: " << model.textureFile << ")";
            cout << endl;
        }
        modelEl = modelEl->NextSiblingElement("model");
    }
}


// Lê o bloco <lights> com luzes point, directional e spot.
// Para OpenGL, a diferença entre point e directional está no 4.º componente
// do vetor de posição: W=1 → point; W=0 → directional.
void SimpleParser::parseLights(XMLElement* lightsEl, Group& group) {
    if (!lightsEl) return;
    group.lights.clear();

    XMLElement* lightEl = lightsEl->FirstChildElement("light");
    while (lightEl) {
        const char* typeStr = lightEl->Attribute("type");
        if (!typeStr) { lightEl = lightEl->NextSiblingElement("light"); continue; }

        Light light;
        if (strcmp(typeStr, "point") == 0) {
            light.type = LIGHT_POINT;
            lightEl->QueryFloatAttribute("posX", &light.posX);
            lightEl->QueryFloatAttribute("posY", &light.posY);
            lightEl->QueryFloatAttribute("posZ", &light.posZ);

        } else if (strcmp(typeStr, "directional") == 0) {
            light.type = LIGHT_DIRECTIONAL;
            lightEl->QueryFloatAttribute("dirX", &light.dirX);
            lightEl->QueryFloatAttribute("dirY", &light.dirY);
            lightEl->QueryFloatAttribute("dirZ", &light.dirZ);

        } else if (strcmp(typeStr, "spot") == 0) {
            light.type = LIGHT_SPOT;
            lightEl->QueryFloatAttribute("posX",   &light.posX);
            lightEl->QueryFloatAttribute("posY",   &light.posY);
            lightEl->QueryFloatAttribute("posZ",   &light.posZ);
            lightEl->QueryFloatAttribute("dirX",   &light.dirX);
            lightEl->QueryFloatAttribute("dirY",   &light.dirY);
            lightEl->QueryFloatAttribute("dirZ",   &light.dirZ);
            lightEl->QueryFloatAttribute("cutoff", &light.cutoff);
        }

        group.lights.push_back(light);
        cout << "Luz: " << typeStr << endl;
        lightEl = lightEl->NextSiblingElement("light");
    }
}


void SimpleParser::parseWindow(XMLElement* el, Window& window) {
    if (!el) { cerr<<"Aviso: 'window' não encontrado\n"; return; }
    int w=window.width, h=window.height;
    el->QueryIntAttribute("width",&w);
    el->QueryIntAttribute("height",&h);
    if (w<100||h<100) { cerr<<"Aviso: dimensões inválidas\n"; return; }
    window.width=w; window.height=h;
    cout<<"Janela: "<<w<<"x"<<h<<endl;
}


void SimpleParser::parseCamera(XMLElement* el, Camera& camera) {
    if (!el) { cerr<<"Aviso: 'camera' não encontrado\n"; return; }
    float px=0,py=0,pz=5;
    if (auto* e=el->FirstChildElement("position")) {
        e->QueryFloatAttribute("x",&px);
        e->QueryFloatAttribute("y",&py);
        e->QueryFloatAttribute("z",&pz);
    }
    float lx=0,ly=0,lz=0;
    if (auto* e=el->FirstChildElement("lookAt")) {
        e->QueryFloatAttribute("x",&lx);
        e->QueryFloatAttribute("y",&ly);
        e->QueryFloatAttribute("z",&lz);
    }
    float ux=0,uy=1,uz=0;
    if (auto* e=el->FirstChildElement("up")) {
        e->QueryFloatAttribute("x",&ux);
        e->QueryFloatAttribute("y",&uy);
        e->QueryFloatAttribute("z",&uz);
    }
    float fov=60,near=1,far=1000;
    if (auto* e=el->FirstChildElement("projection")) {
        e->QueryFloatAttribute("fov",&fov);
        e->QueryFloatAttribute("near",&near);
        e->QueryFloatAttribute("far",&far);
    }
    camera.setPosition(px,py,pz);
    camera.setLookAt(lx,ly,lz);
    camera.setUp(ux,uy,uz);
    camera.setProjection(fov,near,far);
}