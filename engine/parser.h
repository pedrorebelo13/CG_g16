#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "camera.h"
#include "tinyxml2.h"

/**
 * @struct Window
 * @brief Armazena as dimensões da janela de visualização.
 */
struct Window {
    int width;  ///< Largura da janela em pixels
    int height; ///< Altura da janela em pixels
    
    /// Construtor padrão: cria janela 800x600
    Window() : width(800), height(600) {}
};

/**
 * @struct Model
 * @brief Representa uma referência para um arquivo de modelo 3D.
 */
struct Model {
    std::string filename; ///< Caminho do arquivo .3d a carregar
};

struct Model {
    std::string filename; ///< Caminho do arquivo .3d a carregar
};

enum TransformType {
    TRANSLATE,
    ROTATE,
    SCALE
};

struct Transformation {
    TransformType type; ///< Tipo de transformação (translate, rotate, scale)
    float x, y, z;      ///< Parâmetros da transformação (dependendo do tipo)
    float angle;        ///< Ângulo para rotações (em graus), 0 para outros tipos

    //construtor para translate e scale
     Transformation(TransformType t, float xVal, float yVal, float zVal)
        : type(t), x(xVal), y(yVal), z(zVal), angle(0) {}

    //construtor para rotate
    Transformation(TransformType t, float xVal, float yVal, float zVal, float angleVal = 0)
        : type(t), x(xVal), y(yVal), z(zVal), angle(angleVal) {}

    
};

/**
 * @struct Group
 * @brief Agrupa múltiplos modelos para a cena.
 */
struct Group {
    std::vector<Transformation> transformations; ///< Lista de transformações a aplicar ao grupo
    std::vector<Model> models; ///< Lista de modelos a renderizar
    std::vector<Group> children; ///< Grupos filhos para hierarquia de cena
};

/**
 * @class SimpleParser
 * @brief Responsável por parse de arquivos XML de configuração da cena.
 *
 * Esta classe oferece funcionalidades para ler arquivos XML que definem:
 * - Dimensões da janela
 * - Parâmetros da câmera (posição, orientação, projeção)
 * - Hierarquia de grupos com transformações geométricas e modelos
 *
 * A Fase 2 introduz suporte para cenas hierárquicas onde cada nó pode
 * contém transformações (translate, rotate, scale) e opcionalmente modelos.
 * Cada nó pode ter filhos, formando uma árvore de cena.
 *
 * O arquivo XML esperado segue a estrutura:
 * @code
 * <world>
 *   <window width="512" height="512" />
 *   <camera>
 *     <position x="3" y="2" z="1" />
 *     <lookAt x="0" y="0" z="0" />
 *     <up x="0" y="1" z="0" />
 *     <projection fov="60" near="1" far="1000" />
 *   </camera>
 *   <group>
 *     <transform>
 *       <translate x="4" y="0" z="0" />
 *       <rotate angle="30" x="0" y="1" z="0" />
 *       <scale x="2" y="1" z="1" />
 *     </transform>
 *     <models>
 *       <model file="plane.3d" />
 *       <model file="cone.3d" />
 *     </models>
 *     <group>
 *       <!-- Subgrupo herdando transformações do pai -->
 *     </group>
 *   </group>
 * </world>
 * @endcode
 */
class SimpleParser {
public:
    /**
     * @brief Realiza o parse completo do arquivo XML de configuração.
     *
     * Esta função lê o arquivo XML e extrai todos os parâmetros de:
     * - Window (dimensões)
     * - Camera (posição, orientação, projeção)
     * - Hierarquia de Group (transformações e modelos)
     *
     * @param filename Caminho do arquivo XML de configuração
     * @param window Struct que será preenchida com dimensões da janela
     * @param camera Objeto câmera que será configurado com os parâmetros lidos
     * @param rootGroup Struct que será preenchida com a hierarquia de grupos
     *
     * @return true se o parse foi bem-sucedido, false caso contrário
     */
    static bool parseXMLFile(const std::string& filename, 
                            Window& window, 
                            Camera& camera, 
                            Group& rootGroup);
    
private:
    /**
     * @brief Parse recursivo de um grupo e seus filhos.
     *
     * Extrai as transformações, modelos e subgrupos de um nó <group>.
     * Os subgrupos herdam as transformações do pai na renderização.
     *
     * @param groupElement Ponteiro para o elemento XML <group>
     * @return Group struct com todas as transformações, modelos e subgrupos
     *
     * @note Esta função é chamada recursivamente para cada <group> aninhado
     */
    static Group parseGroup(tinyxml2::XMLElement* groupElement);
    
    /**
     * @brief Extrai as transformações de um elemento <transform>.
     *
     * Percorre todos os subelementos de <transform> (translate, rotate, scale)
     * preservando a ORDEM exata no XML, que é crítica para o resultado final.
     *
     * @param transformElement Ponteiro para o elemento XML <transform>
     * @param group Struct onde as transformações serão armazenadas
     *
     * @note A ordem das transformações é essencial: translate(rotate(v)) != rotate(translate(v))
     * @note Pode haver apenas uma transformação de cada tipo dentro de um <transform>
     */
    static void parseTransforms(tinyxml2::XMLElement* transformElement, Group& group);
    
    /**
     * @brief Extrai a lista de modelos do elemento XML <models>.
     *
     * Percorre todos os elementos <model> dentro de <models> e
     * adiciona cada um à lista do Group.
     *
     * @param modelsElement Ponteiro para o elemento XML <models>
     * @param group Struct onde os modelos serão armazenados
     */
    static void parseModels(tinyxml2::XMLElement* modelsElement, Group& group);
};