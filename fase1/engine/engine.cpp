#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <math.h>
#include <GL/glut.h>
#include "tinyxml2.h"
#include "camera.h"
#include "parser.h"
#include <map>

using namespace std;
using namespace tinyxml2;


/**
 * @struct Vertex
 * @brief Representa um vértice 3D no espaço.
 */
struct Vertex {
    float x, y, z; ///< Coordenadas cartesianas do vértice
    
    Vertex() : x(0), y(0), z(0) {}
    Vertex(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

/**
 * @struct Face
 * @brief Representa uma face triangular do modelo (composta por 3 vértices).
 */
struct Face {
    int v1, v2, v3;  ///< Índices dos 3 vértices que formam o triângulo
    
    Face() : v1(0), v2(0), v3(0) {}
    Face(int _v1, int _v2, int _v3) : v1(_v1), v2(_v2), v3(_v3) {}
};

/**
 * @struct ModelData
 * @brief Armazena dados de um modelo 3D carregado de um arquivo.
 *
 * Cada modelo carregado mantém:
 * - Lista de vértices (coordenadas 3D)
 * - Lista de faces (triângulos definidos por índices de vértices)
 * - Informação se o modelo foi carregado com sucesso
 */
struct ModelData {
    string filename;              ///< Caminho do arquivo de origem
    vector<Vertex> vertices;      ///< Lista de vértices do modelo
    vector<Face> faces;           ///< Lista de faces (triângulos)
    bool loaded;                  ///< Flag indicando se foi carregado com sucesso
    
    ModelData() : loaded(false) {}
};


Window window;                              ///< Dimensões da janela de visualização
Camera* camera;                             ///< Ponteiro para a câmera da cena
vector<ModelData> modelDataList;            ///< Lista de todos os modelos carregados

bool showAxes = false;                      ///< Flag para mostrar/esconder eixos coordenados
bool wireframeMode = false;                 ///< Flag para ativar/desativar modo wireframe


/**
 * @brief Carrega um modelo 3D de um arquivo .3d em formato XML.
 *
 * @param modelData Referência para struct que será preenchida com os dados
 * @param filename Caminho do arquivo .3d
 *
 * @return true se carregado com sucesso, false caso contrário
 */
bool loadModel(ModelData& modelData, const string& filename);

/**
 * @brief Callback GLUT para redimensionamento da janela.
 *
 * @param w Nova largura da janela em pixels
 * @param h Nova altura da janela em pixels
 */
void changeSize(int w, int h);

/**
 * @brief Callback GLUT para renderização da cena.
 *
 * Chamada repetidamente em cada frame. Responsável por:
 * - Limpar buffers
 * - Posicionar câmera
 * - Desenhar modelos
 * - Trocar buffers (double buffering)
 */
void renderScene();

/**
 * @brief Desenha os eixos coordenados (X, Y, Z) na origem.
 *
 * Usa cores padrão:
 * - X: Vermelho
 * - Y: Verde
 * - Z: Azul
 */
void drawAxes();

/**
 * @brief Callback GLUT para entrada de teclado normal (caracteres ASCII).
 *
 * @param key Código ASCII da tecla pressionada
 * @param xx Posição X do mouse (não utilizada)
 * @param yy Posição Y do mouse (não utilizada)
 */
void processKeys(unsigned char key, int xx, int yy);

/**
 * @brief Callback GLUT para entrada de teclas especiais (setas, F1, etc).
 *
 * @param key Código da tecla especial
 * @param xx Posição X do mouse (não utilizada)
 * @param yy Posição Y do mouse (não utilizada)
 */
void processSpecialKeys(int key, int xx, int yy);



int main(int argc, char** argv) {
    // Valida argumentos de entrada
    if (argc < 2) {
        cerr << "Uso: " << argv[0] << " <arquivo_config.xml>" << endl;
        cerr << "Exemplo: " << argv[0] << " config.xml" << endl;
        return 1;
    }
    
    // Cria câmera com valores padrão
    camera = new Camera();
    
    // Estrutura para armazenar modelos a carregar
    Group group;
    
    // Realiza o parse do arquivo XML de configuração
    // O arquivo é lido apenas uma vez na inicialização (conforme requerido na Fase 1)
    cout << "\n=== Inicializando Engine 3D - Fase 1 ===" << endl;
    cout << "Carregando configuração de: " << argv[1] << endl;
    
    if (!SimpleParser::parseXMLFile(argv[1], window, *camera, group)) {
        cerr << "Erro: falha ao fazer parse do arquivo XML." << endl;
        return 1;
    }
    
    // Carrega todos os modelos especificados no arquivo XML
    cout << "\nCarregando modelos..." << endl;
    for (const Model& model : group.models) {
        ModelData modelData;
        if (loadModel(modelData, model.filename)) {
            modelDataList.push_back(modelData);
        } else {
            cerr << "Aviso: falha ao carregar modelo: " << model.filename << endl;
        }
    }
    
    if (modelDataList.empty()) {
        cerr << "Erro: nenhum modelo foi carregado com sucesso." << endl;
        return 1;
    }
    
    cout << "\nTotal de modelos carregados com sucesso: " << modelDataList.size() << endl;
    
        
    // Inicializa GLUT com argumentos da linha de comando
    glutInit(&argc, argv);
    
    // Define modo de visualização:
    // GLUT_DEPTH: ativa buffer de profundidade (Z-buffer) para depth testing
    // GLUT_DOUBLE: ativa double buffering para animação suave
    // GLUT_RGBA: modo de cores RGBA
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    
    // Define posição inicial da janela na tela
    glutInitWindowPosition(100, 100);
    
    // Define dimensões da janela conforme configurado
    glutInitWindowSize(window.width, window.height);
    
    // Cria a janela
    glutCreateWindow("Engine 3D - Fase 1");
    
    // Registra callbacks para eventos
    glutDisplayFunc(renderScene);      // Renderização de cada frame
    glutReshapeFunc(changeSize);       // Redimensionamento da janela
    glutKeyboardFunc(processKeys);     // Teclado (ASCII)
    glutSpecialFunc(processSpecialKeys); // Teclas especiais (setas, etc)
    
    
    // Ativa teste de profundidade para renderização correta de objetos 3D
    glEnable(GL_DEPTH_TEST);
    
    // Ativa culling de faces para otimizar (descartar faces traseiras)
    glEnable(GL_CULL_FACE);
    
    
    // Exibe controles disponíveis para o usuário
    cout << "\n=== Controles da Engine ===" << endl;
    cout << "Setas (cima/baixo): Rotacionar câmera verticalmente" << endl;
    cout << "Setas (esq/dir): Rotacionar câmera horizontalmente" << endl;
    cout << "W: Aproximar (zoom in)" << endl;
    cout << "S: Afastar (zoom out)" << endl;
    cout << "A: Mostrar/ocultar eixos coordenados" << endl;
    cout << "L: Ativar/desativar modo wireframe" << endl;
    cout << "ESC: Sair da aplicação" << endl;
    cout << "============================\n" << endl;
    
    
    
    // Entra no loop de eventos GLUT
    // A função renderScene() será chamada repetidamente
    glutMainLoop();
    
    
    // Liberta a memória alocada para a câmera
    delete camera;
    
    return 0;
}


/**
 * @brief Carrega um modelo 3D em formato XML do arquivo especificado.
 *
 * O arquivo deve estar em formato XML com estrutura:
 * @code
 * <plane|box|sphere|cone>
 *   <triangle>
 *     <vertex x="x1" y="y1" z="z1" />
 *     <vertex x="x2" y="y2" z="z2" />
 *     <vertex x="x3" y="y3" z="z3" />
 *   </triangle>
 *   ...
 * </plane|box|sphere|cone>
 * @endcode
 *
 * A função realiza:
 * 1. Abertura e leitura do arquivo
 * 2. Parse XML dos vértices e faces
 * 3. Deduplicação de vértices para otimizar uso de memória
 * 4. Armazenamento em estrutura ModelData
 */
bool loadModel(ModelData& modelData, const string& filename) {
    // Tenta abrir o arquivo do modelo
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Erro ao abrir arquivo do modelo: " << filename << endl;
        return false;
    }
    
    // Armazena o nome do arquivo para referência
    modelData.filename = filename;
    
    // Limpa dados anteriores (em caso de reutilização da estrutura)
    modelData.vertices.clear();
    modelData.faces.clear();
    
    // Lê todo o conteúdo do arquivo em uma string
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();
    
    
    // Parse do conteúdo XML usando TinyXML2
    XMLDocument doc;
    if (doc.Parse(content.c_str()) != XML_SUCCESS) {
        cerr << "Erro ao fazer parse XML do arquivo: " << filename << endl;
        return false;
    }
    
    // Obtém elemento raiz (deve ser um de: plane, box, sphere, cone)
    XMLElement* rootElement = doc.RootElement();
    if (!rootElement) {
        cerr << "Nenhum elemento raiz encontrado no arquivo: " << filename << endl;
        return false;
    }
    
        
    // Mapa para armazenar vértices já adicionados e seus índices
    // Chave: string "x,y,z" | Valor: índice no array de vértices
    // Isto otimiza memória evitando vértices duplicados
    map<string, int> vertexIndices;
    int nextIndex = 0;
    
        
    // Processa todos os elementos <triangle> do documento
    XMLElement* triangleElement = rootElement->FirstChildElement("triangle");
    int faceCount = 0;
    
    while (triangleElement) {
        // Obtém os 3 vértices do triângulo
        XMLElement* vertex1 = triangleElement->FirstChildElement("vertex");
        XMLElement* vertex2 = vertex1 ? vertex1->NextSiblingElement("vertex") : nullptr;
        XMLElement* vertex3 = vertex2 ? vertex2->NextSiblingElement("vertex") : nullptr;
        
        // Valida se todos os 3 vértices existem
        if (vertex1 && vertex2 && vertex3) {
            // Array para armazenar índices dos vértices desta face
            vector<int> vertexIndicesForTriangle;
            
            // Processa cada um dos 3 vértices do triângulo
            for (XMLElement* vertex : {vertex1, vertex2, vertex3}) {
                // Extrai coordenadas com valores padrão 0
                float x = 0, y = 0, z = 0;
                vertex->QueryFloatAttribute("x", &x);
                vertex->QueryFloatAttribute("y", &y);
                vertex->QueryFloatAttribute("z", &z);
                
                // Cria chave única para este vértice (coordenadas como string)
                string vertexKey = to_string(x) + "," + to_string(y) + "," + to_string(z);
                
                // Verifica se vértice já foi visto antes
                if (vertexIndices.find(vertexKey) == vertexIndices.end()) {
                    // Novo vértice: adiciona ao array de vértices
                    modelData.vertices.push_back(Vertex(x, y, z));
                    vertexIndices[vertexKey] = nextIndex;
                    vertexIndicesForTriangle.push_back(nextIndex);
                    nextIndex++;
                } else {
                    // Existing vertex, reuse its index
                    vertexIndicesForTriangle.push_back(vertexIndices[vertexKey]);
                }
            }
            
            // Add the face if we have three valid vertices
            if (vertexIndicesForTriangle.size() == 3) {
                modelData.faces.push_back(Face(
                    vertexIndicesForTriangle[0],
                    vertexIndicesForTriangle[1],
                    vertexIndicesForTriangle[2]
                ));
                faceCount++;
            }
        } else {
            cerr << "Triangle missing vertices in model file: " << filename << endl;
        }
        
        triangleElement = triangleElement->NextSiblingElement("triangle");
    }
    
    modelData.loaded = true;
    cout << "Modelo carregado: " << filename << " (" << modelData.vertices.size() 
         << " vértices, " << faceCount << " faces)" << endl;
    
    return true;
}

/**
 * @brief Desenha os eixos coordenados X, Y, Z na origem em cores padrão.
 *
 * Cada eixo é representado como uma linha:
 * - X (vermelho): -100 a +100
 * - Y (verde): -100 a +100
 * - Z (azul): -100 a +100
 */
void drawAxes() {
    glBegin(GL_LINES);
    
    // Eixo X (vermelho)
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-100.0f, 0.0f, 0.0f);
    glVertex3f(100.0f, 0.0f, 0.0f);
    
    // Eixo Y (verde)
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, -100.0f, 0.0f);
    glVertex3f(0.0f, 100.0f, 0.0f);
    
    // Eixo Z (azul)
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, -100.0f);
    glVertex3f(0.0f, 0.0f, 100.0f);
    
    glEnd();
}



/**
 * @brief Callback de redimensionamento de janela.
 *
 * Ajusta o viewport e a matriz de projeção quando a janela é redimensionada.
 */
void changeSize(int w, int h) {
    // Previne divisão por zero
    if (h == 0) h = 1;
    
    // Calcula a proporção da janela (aspect ratio)
    float ratio = w * 1.0f / h;
    
    // Seleciona e configura a matriz de projeção
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // Define a área de visualização (viewport)
    glViewport(0, 0, w, h);
    
    // Define a perspectiva com base nos parâmetros da câmera
    gluPerspective(camera->getFov(), ratio, camera->getNearPlane(), camera->getFarPlane());
    
    // Retorna para a matriz de visão (modelview)
    glMatrixMode(GL_MODELVIEW);
}



/**
 * @brief Callback de renderização GLUT.
 *
 * Chamada a cada frame para:
 * 1. Limpar buffers
 * 2. Posicionar câmera
 * 3. Desenhar modelos
 * 4. Trocar buffers (double buffering)
 */
void renderScene() {
    // Limpa todos os buffers de desenho
    glDisable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Define modo de preenchimento de polígonos
    if (wireframeMode) {
        // Modo wireframe: desenha apenas as arestas dos triângulos
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        // Modo preenchimento: desenha polígonos sólidos
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    // Carrega matriz identidade e posiciona a câmera
    glLoadIdentity();
    camera->place();
    
    // Desenha eixos coordenados se ativado
    if (showAxes) {
        drawAxes();
    }
    
    // Renderiza todos os modelos carregados
    for (const ModelData& modelData : modelDataList) {
        // Ignora modelos não carregados
        if (!modelData.loaded) continue;
        
        // Se o modelo tem faces definidas, usa-as para renderização
        if (!modelData.faces.empty()) {
            glBegin(GL_TRIANGLES);
            
            // Renderiza cada face (triângulo) do modelo
            for (const Face& face : modelData.faces) {
                // Alterna cores para melhor visualização
                static int colorToggle = 0;
                if (colorToggle % 2 == 0) {
                    glColor3f(0.8f, 0.6f, 0.2f);  // Laranja
                } else {
                    glColor3f(0.2f, 0.6f, 0.8f);  // Azul
                }
                colorToggle++;
                
                // Desenha o triângulo usando os índices de vértices
                const Vertex& v1 = modelData.vertices[face.v1];
                const Vertex& v2 = modelData.vertices[face.v2];
                const Vertex& v3 = modelData.vertices[face.v3];
                
                glVertex3f(v1.x, v1.y, v1.z);
                glVertex3f(v2.x, v2.y, v2.z);
                glVertex3f(v3.x, v3.y, v3.z);
            }
            glEnd();
        } else {
            // Fallback: renderiza vértices diretamente em grupos de 3 (triângulos)
            glBegin(GL_TRIANGLES);
            for (size_t i = 0; i < modelData.vertices.size(); i += 3) {
                if (i + 2 < modelData.vertices.size()) {
                    // Alterna cores para melhor visualização
                    static int colorToggle = 0;
                    if (colorToggle % 2 == 0) {
                        glColor3f(0.8f, 0.6f, 0.2f);  // Laranja
                    } else {
                        glColor3f(0.2f, 0.6f, 0.8f);  // Azul
                    }
                    colorToggle++;
                    
                    // Desenha o triângulo
                    const Vertex& v1 = modelData.vertices[i];
                    const Vertex& v2 = modelData.vertices[i + 1];
                    const Vertex& v3 = modelData.vertices[i + 2];
                    
                    glVertex3f(v1.x, v1.y, v1.z);
                    glVertex3f(v2.x, v2.y, v2.z);
                    glVertex3f(v3.x, v3.y, v3.z);
                }
            }
            glEnd();
        }
    }
    
    // Troca os buffers (double buffering) para exibir a cena renderizada
    glutSwapBuffers();
}


/**
 * @brief Processa entrada de teclado (caracteres ASCII).
 *
 * Controles:
 * - 'A': Mostrar/ocultar eixos
 * - 'L': Ativar/desativar wireframe
 * - 'W': Aproximar câmera (zoom in)
 * - 'S': Afastar câmera (zoom out)
 * - ESC: Sair da aplicação
 */
void processKeys(unsigned char key, int xx, int yy) {
    switch (key) {
        case 'a':
        case 'A':
            // Alterna visualização dos eixos coordenados
            showAxes = !showAxes;
            cout << "Eixos: " << (showAxes ? "LIGADOS" : "DESLIGADOS") << endl;
            break;
        
        case 'l':
        case 'L':
            // Alterna modo wireframe (mostra apenas arestas)
            wireframeMode = !wireframeMode;
            cout << "Wireframe: " << (wireframeMode ? "LIGADO" : "DESLIGADO") << endl;
            break;
        
        case 'w':
        case 'W':
            // Aproxima a câmera do objeto (diminui raio)
            camera->zoomIn();
            break;
        
        case 's':
        case 'S':
            // Afasta a câmera do objeto (aumenta raio)
            camera->zoomOut();
            break;
        
        case 27:  // Tecla ESC
            // Sai da aplicação
            cout << "Encerrando aplicação..." << endl;
            exit(0);
            break;
    }
    
    // Solicita redesenho da cena no próximo frame
    glutPostRedisplay();
}

/**
 * @brief Processa entrada de teclas especiais (setas, F1-F12, etc).
 *
 * Controles:
 * - Seta para cima: Rotacionar câmera para cima
 * - Seta para baixo: Rotacionar câmera para baixo
 * - Seta para esquerda: Rotacionar câmera para esquerda
 * - Seta para direita: Rotacionar câmera para direita
 */
void processSpecialKeys(int key, int xx, int yy) {
    switch (key) {
        case GLUT_KEY_UP:
            // Rotaciona câmera para cima (aumenta ângulo vertical/beta)
            camera->rotateUp();
            break;
        
        case GLUT_KEY_DOWN:
            // Rotaciona câmera para baixo (diminui ângulo vertical/beta)
            camera->rotateDown();
            break;
        
        case GLUT_KEY_LEFT:
            // Rotaciona câmera para esquerda (diminui ângulo horizontal/alpha)
            camera->rotateLeft();
            break;
        
        case GLUT_KEY_RIGHT:
            // Rotaciona câmera para direita (aumenta ângulo horizontal/alpha)
            camera->rotateRight();
            break;
    }
    
    // Solicita redesenho da cena no próximo frame
    glutPostRedisplay();
}