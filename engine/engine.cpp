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

// Structure to represent a 3D vertex
struct Vertex {
    float x, y, z;
    
    Vertex() : x(0), y(0), z(0) {}
    Vertex(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

// Structure to represent a face (triangle)
struct Face {
    int v1, v2, v3;  // Vertex indices
    
    Face() : v1(0), v2(0), v3(0) {}
    Face(int _v1, int _v2, int _v3) : v1(_v1), v2(_v2), v3(_v3) {}
};

// Structure to represent a 3D model with vertices and faces
struct ModelData {
    string filename;
    vector<Vertex> vertices;
    vector<Face> faces;
    
    bool loaded;
    
    ModelData() : loaded(false) {}
};

// Global variables
Window window;
Camera* camera;
vector<ModelData> modelDataList; // List of loaded model data

bool showAxes = false;
bool wireframeMode = false;

// Function prototypes
bool loadModel(ModelData& modelData, const string& filename);
void changeSize(int w, int h);
void renderScene();
void drawAxes();
void processKeys(unsigned char key, int xx, int yy);
void processSpecialKeys(int key, int xx, int yy);

int main(int argc, char** argv) {
    // Check if config file is provided
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <config.xml>" << endl;
        return 1;
    }
    
    // Create camera with default values
    camera = new Camera();
    
    // Initialize Group to hold models
    Group group;
    
    // Parse the XML file using SimpleParser
    if (!SimpleParser::parseXMLFile(argv[1], window, *camera, group)) {
        cerr << "Failed to parse XML file." << endl;
        return 1;
    }
    
    // Load all models from the parsed Group
    for (const Model& model : group.models) {
        ModelData modelData;
        if (loadModel(modelData, model.filename)) {
            modelDataList.push_back(modelData);
        }
    }
    
    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(window.width, window.height);
    glutCreateWindow("3D Engine - Phase 1");
    
    // Register callback functions
    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);
    glutKeyboardFunc(processKeys);
    glutSpecialFunc(processSpecialKeys);
    
    // OpenGL settings
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    // Display keyboard controls
    cout << "\n--- 3D Engine Controls ---" << endl;
    cout << "Arrow keys: Rotate camera" << endl;
    cout << "W/S: Zoom in/out" << endl;
    cout << "A: Toggle axes display" << endl;
    cout << "L: Toggle wireframe mode" << endl;
    
    // Enter GLUT main loop
    glutMainLoop();
    
    // Clean up
    delete camera;
    
    return 0;
}

// Load a 3D model from file
bool loadModel(ModelData& modelData, const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening model file: " << filename << endl;
        return false;
    }
    
    // Set filename
    modelData.filename = filename;
    
    // Clear any existing data
    modelData.vertices.clear();
    modelData.faces.clear();
    
    // Read the entire file content into a string
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();
    
    // Parse XML content using TinyXML2
    XMLDocument doc;
    if (doc.Parse(content.c_str()) != XML_SUCCESS) {
        cerr << "Error parsing XML in model file: " << filename << endl;
        return false;
    }
    
    // Get the root element (should be one of: plane, box, sphere, cone)
    XMLElement* rootElement = doc.RootElement();
    if (!rootElement) {
        cerr << "No root element found in model file: " << filename << endl;
        return false;
    }
    
    // Maps to store vertex indices
    map<string, int> vertexIndices;
    int nextIndex = 0;
    
    // Process all triangle elements
    XMLElement* triangleElement = rootElement->FirstChildElement("triangle");
    int faceCount = 0;
    
    while (triangleElement) {
        XMLElement* vertex1 = triangleElement->FirstChildElement("vertex");
        XMLElement* vertex2 = vertex1 ? vertex1->NextSiblingElement("vertex") : nullptr;
        XMLElement* vertex3 = vertex2 ? vertex2->NextSiblingElement("vertex") : nullptr;
        
        if (vertex1 && vertex2 && vertex3) {
            // Create three vertices for the triangle
            vector<int> vertexIndicesForTriangle;
            
            // Process each vertex of the triangle
            for (XMLElement* vertex : {vertex1, vertex2, vertex3}) {
                float x = 0, y = 0, z = 0;
                vertex->QueryFloatAttribute("x", &x);
                vertex->QueryFloatAttribute("y", &y);
                vertex->QueryFloatAttribute("z", &z);
                
                // Create a unique key for this vertex
                string vertexKey = to_string(x) + "," + to_string(y) + "," + to_string(z);
                
                // Check if we've seen this vertex before
                if (vertexIndices.find(vertexKey) == vertexIndices.end()) {
                    // New vertex, add it to the model
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
    cout << "Model loaded: " << filename << " (" << modelData.vertices.size() << " vertices, " 
         << faceCount << " faces)" << endl;
    
    return true;
}

// Draw coordinate axes
void drawAxes() {
    glBegin(GL_LINES);
    
    // X axis (red)
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-100.0f, 0.0f, 0.0f);
    glVertex3f(100.0f, 0.0f, 0.0f);
    
    // Y axis (green)
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, -100.0f, 0.0f);
    glVertex3f(0.0f, 100.0f, 0.0f);
    
    // Z axis (blue)
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, -100.0f);
    glVertex3f(0.0f, 0.0f, 100.0f);
    
    glEnd();
}

// GLUT reshape function
void changeSize(int w, int h) {
    // Prevent division by zero
    if (h == 0) h = 1;
    
    // Compute window's aspect ratio
    float ratio = w * 1.0f / h;
    
    // Set the projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // Set the viewport
    glViewport(0, 0, w, h);
    
    // Set perspective
    gluPerspective(camera->getFov(), ratio, camera->getNearPlane(), camera->getFarPlane());
    
    // Return to modelview matrix
    glMatrixMode(GL_MODELVIEW);
}

// GLUT display function
void renderScene() {
    // Clear buffers
    glDisable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set polygon drawing mode
    if (wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    // Set the camera
    glLoadIdentity();
    camera->place();
    
    // Draw axes if enabled
    if (showAxes) {
        drawAxes();
    }
    
    // Render all models
    for (const ModelData& modelData : modelDataList) {
        if (!modelData.loaded) continue;
        
        // If model has faces defined, use them for rendering
        if (!modelData.faces.empty()) {
            glBegin(GL_TRIANGLES);
            for (const Face& face : modelData.faces) {
                // Use alternating colors for triangles
                static int colorToggle = 0;
                if (colorToggle % 2 == 0) {
                    glColor3f(0.8f, 0.6f, 0.2f);  // Orange-ish
                } else {
                    glColor3f(0.2f, 0.6f, 0.8f);  // Blue-ish
                }
                colorToggle++;
                
                // Draw the triangle
                const Vertex& v1 = modelData.vertices[face.v1];
                const Vertex& v2 = modelData.vertices[face.v2];
                const Vertex& v3 = modelData.vertices[face.v3];
                
                glVertex3f(v1.x, v1.y, v1.z);
                glVertex3f(v2.x, v2.y, v2.z);
                glVertex3f(v3.x, v3.y, v3.z);
            }
            glEnd();
        } else {
            // No faces defined, render vertices directly in triangle order
            glBegin(GL_TRIANGLES);
            for (size_t i = 0; i < modelData.vertices.size(); i += 3) {
                if (i + 2 < modelData.vertices.size()) {
                    // Use alternating colors for triangles
                    static int colorToggle = 0;
                    if (colorToggle % 2 == 0) {
                        glColor3f(0.8f, 0.6f, 0.2f);  // Orange-ish
                    } else {
                        glColor3f(0.2f, 0.6f, 0.8f);  // Blue-ish
                    }
                    colorToggle++;
                    
                    // Draw the triangle
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
    
    // Swap buffers
    glutSwapBuffers();
}

// Keyboard input processing
void processKeys(unsigned char key, int xx, int yy) {
    switch (key) {
        case 'a':
        case 'A':
            showAxes = !showAxes;
            break;
        
        case 'l':
        case 'L':
            wireframeMode = !wireframeMode;
            break;
        
        case 'w':
        case 'W':
            camera->zoomIn();
            break;
        
        case 's':
        case 'S':
            camera->zoomOut();
            break;
        
        case 27:  // Escape key
            exit(0);
            break;
    }
    
    glutPostRedisplay();
}

// Special key input processing
void processSpecialKeys(int key, int xx, int yy) {
    switch (key) {
        case GLUT_KEY_UP:
            camera->rotateUp();
            break;
        
        case GLUT_KEY_DOWN:
            camera->rotateDown();
            break;
        
        case GLUT_KEY_LEFT:
            camera->rotateLeft();
            break;
        
        case GLUT_KEY_RIGHT:
            camera->rotateRight();
            break;
    }
    
    glutPostRedisplay();
}