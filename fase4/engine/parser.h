#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "camera.h"
#include "tinyxml2.h"

/**
 * @file parser.h
 * @brief Parser XML da cena (Fase 4) — materiais, texturas, luzes e transformações.
 *
 * Contém as estruturas de dados usadas pelo engine e a classe `SimpleParser`
 * que converte um ficheiro XML de cena numa estrutura `Group` usada pelo
 * renderer. Os comentários Doxygen aqui documentam a API mínima para uso
 * por outras partes do código.
 */
// ─────────────────────────────────────────────────────────────────────────────
// Estruturas de dados base (inalteradas da Fase 3)
// ─────────────────────────────────────────────────────────────────────────────

struct Window {
    int width;
    int height;
    Window() : width(800), height(600) {}
};

enum TransformType {
    TRANSLATE,
    TRANSLATE_ANIM,
    ROTATE,
    ROTATE_ANIM,
    SCALE
};

struct Point3D {
    float x, y, z;
    Point3D() : x(0), y(0), z(0) {}
    Point3D(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Transformation {
    TransformType type;
    float x, y, z;
    float angle;
    float time;
    bool align;
    std::vector<Point3D> points;

    Transformation(TransformType t, float xVal, float yVal, float zVal, float angleVal = 0)
        : type(t), x(xVal), y(yVal), z(zVal), angle(angleVal), time(0), align(false) {}

    static Transformation makeRotateAnim(float timeVal, float xVal, float yVal, float zVal) {
        Transformation t(ROTATE_ANIM, xVal, yVal, zVal, 0);
        t.time = timeVal;
        return t;
    }
    static Transformation makeTranslateAnim(float timeVal, bool alignVal,
                                            const std::vector<Point3D>& pts) {
        Transformation t(TRANSLATE_ANIM, 0, 0, 0, 0);
        t.time = timeVal;
        t.align = alignVal;
        t.points = pts;
        return t;
    }
};


// ─────────────────────────────────────────────────────────────────────────────
// Fase 4 — Material de cor para cada modelo
// Componentes do modelo de iluminação Phong:
//   diffuse  — cor principal (luz difusa, depende do ângulo da normal)
//   ambient  — luz ambiente (iluminação mínima mesmo sem luz direta)
//   specular — reflexo especular (brilho)
//   emissive — cor própria (não depende de luz externa; útil para o Sol)
//   shininess — concentração do brilho especular (0-128)
// Os valores R,G,B vêm do XML em [0,255] e são normalizados para [0,1] aqui.
// ─────────────────────────────────────────────────────────────────────────────
struct Material {
    float diffuse[4]  = {0.8f, 0.8f, 0.8f, 1.0f};
    float ambient[4]  = {0.2f, 0.2f, 0.2f, 1.0f};
    float specular[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float emissive[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float shininess   = 0.0f;
};

// Fase 4 — Modelo estendido com textura e material
struct Model {
    std::string filename;
    std::string textureFile;  // vazio = sem textura
    Material material;
};


// ─────────────────────────────────────────────────────────────────────────────
// Fase 4 — Fontes de luz
// Três tipos suportados, conforme o enunciado:
//   point       — ponto de luz com posição; W=1 no vetor de posição OpenGL
//   directional — luz direcional sem posição; W=0 no vetor de posição OpenGL
//   spot        — cone de luz com posição, direção e ângulo de corte
// ─────────────────────────────────────────────────────────────────────────────
enum LightType { LIGHT_POINT, LIGHT_DIRECTIONAL, LIGHT_SPOT };

struct Light {
    LightType type;
    float posX=0, posY=0, posZ=0;   // posição (point e spot)
    float dirX=0, dirY=0, dirZ=0;   // direção (directional e spot)
    float cutoff=180.0f;             // ângulo de corte em graus (spot); 180 = desativado
};


// ─────────────────────────────────────────────────────────────────────────────
// Group — estendido com lista de luzes
// As luzes são definidas a nível global no XML (<lights> dentro de <world>)
// mas guardamos no rootGroup para simplificar o acesso no engine.
// ─────────────────────────────────────────────────────────────────────────────
struct Group {
    std::vector<Transformation> transformations;
    std::vector<Model>          models;
    std::vector<Group>          children;
    std::vector<Light>          lights;   // só preenchido no rootGroup
};


// ─────────────────────────────────────────────────────────────────────────────
// Parser
// ─────────────────────────────────────────────────────────────────────────────
class SimpleParser {
public:
    /**
     * @brief Faz parse do ficheiro XML de cena e preenche estruturas.
     * @param filename Caminho para o ficheiro XML.
     * @param window Saída com dimensões da janela.
     * @param camera Saída com parâmetros de câmara.
     * @param rootGroup Saída com a hierarquia de grupos, modelos e luzes.
     * @return true em sucesso, false se ocorrer um erro ao abrir/parsear.
     *
     * Esta função é o ponto de entrada do parser: valida o XML, lê a
     * secção `<lights>` e a hierarquia de `<group>` e devolve a cena
     * completa pronta a ser consumida pelo renderer.
     */
    static bool parseXMLFile(const std::string& filename,
                             Window& window,
                             Camera& camera,
                             Group& rootGroup);
private:
    /** @name Funções auxiliares do parser (uso interno) */
    /**@{*/
    static void parseWindow(tinyxml2::XMLElement* el, Window& w);
    static void parseCamera(tinyxml2::XMLElement* el, Camera& c);
    static Group parseGroup(tinyxml2::XMLElement* el);
    static void parseTransforms(tinyxml2::XMLElement* el, Group& g);
    static void parseModels(tinyxml2::XMLElement* el, Group& g);
    static void parseLights(tinyxml2::XMLElement* el, Group& g);
    static Material parseMaterial(tinyxml2::XMLElement* modelEl);
    /**@}*/
};