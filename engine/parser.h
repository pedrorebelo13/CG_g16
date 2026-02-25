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

/**
 * @struct Group
 * @brief Agrupa múltiplos modelos para a cena.
 */
struct Group {
    std::vector<Model> models; ///< Lista de modelos a renderizar
};

/**
 * @class SimpleParser
 * @brief Responsável por parse de arquivos XML de configuração da cena.
 *
 * Esta classe oferece funcionalidades para ler arquivos XML que definem:
 * - Dimensões da janela
 * - Parâmetros da câmera (posição, orientação, projeção)
 * - Lista de modelos a carregar
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
 *     <models>
 *       <model file="plane.3d" />
 *       <model file="cone.3d" />
 *     </models>
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
     * - Group (modelos a carregar)
     *
     * @param filename Caminho do arquivo XML de configuração
     * @param window Struct que será preenchida com dimensões da janela
     * @param camera Objeto câmera que será configurado com os parâmetros lidos
     * @param group Struct que será preenchida com lista de modelos
     *
     * @return true se o parse foi bem-sucedido, false caso contrário
     *
     * @note O arquivo XML é lido uma única vez no início da aplicação,
     *       conforme requerido pela Fase 1 do projeto.
     */
    static bool parseXMLFile(const std::string& filename, 
                            Window& window, 
                            Camera& camera, 
                            Group& group);
    
private:
    /**
     * @brief Extrai a lista de modelos do elemento XML de modelos.
     *
     * Percorre todos os elementos <model> dentro de <models> e
     * adiciona cada um à lista do Group.
     *
     * @param modelsElement Ponteiro para o elemento XML <models>
     * @param group Struct onde os modelos serão armazenados
     *
     * @note Esta função é chamada internamente por parseXMLFile()
     */
    static void parseModels(tinyxml2::XMLElement* modelsElement, Group& group);
    
    /**
     * @brief Extrai os parâmetros de câmera do arquivo XML.
     *
     * Realiza o parse dos elementos de câmera:
     * - <position>
     * - <lookAt>
     * - <up>
     * - <projection>
     *
     * @param cameraElement Ponteiro para o elemento XML <camera>
     * @param camera Objeto câmera que será configurado
     *
     * @note Usa valores padrão se algum subelemento estiver ausente
     */
    static void parseCamera(tinyxml2::XMLElement* cameraElement, Camera& camera);
    
    /**
     * @brief Extrai os parâmetros de janela do arquivo XML.
     *
     * @param windowElement Ponteiro para o elemento XML <window>
     * @param window Struct que será preenchida com width e height
     *
     * @note Se o elemento for nulo, mantém os valores padrão (800x600)
     */
    static void parseWindow(tinyxml2::XMLElement* windowElement, Window& window);
};