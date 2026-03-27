#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

string caminhoFicheiro(const string& filename) {
    string dirPath = "files3d";
    
    if (!fs::exists(dirPath)) {
        fs::create_directory(dirPath);
    }
    

    return dirPath + "/" + filename;
}

//Plano
void generatePlane(float length, int divisions, const string& filename) {
    string filePath = caminhoFicheiro(filename);
    
    ofstream file(filePath);
    if (!file.is_open()) {
        cerr << "Erro ao abrir o ficheiro: " << filePath << endl;
        return;
    }

    file << "<plane>\n";

    float step = length / divisions;
    float start = -length / 2;

    for (int i = 0; i < divisions; i++) {
        for (int j = 0; j < divisions; j++) {
            float x1 = start + j * step;
            float z1 = start + i * step;
            float x2 = start + (j + 1) * step;
            float z2 = start + i * step;
            float x3 = start + j * step;
            float z3 = start + (i + 1) * step;
            float x4 = start + (j + 1) * step;
            float z4 = start + (i + 1) * step;

            // Triângulo 1
            file << "  <triangle>\n";
            file << "    <vertex x='" << x1 << "' y='0' z='" << z1 << "'/>\n";
            file << "    <vertex x='" << x3 << "' y='0' z='" << z3 << "'/>\n";
            file << "    <vertex x='" << x2 << "' y='0' z='" << z2 << "'/>\n";
            file << "  </triangle>\n";

            // Triângulo 2
            file << "  <triangle>\n";
            file << "    <vertex x='" << x2 << "' y='0' z='" << z2 << "'/>\n";
            file << "    <vertex x='" << x3 << "' y='0' z='" << z3 << "'/>\n";
            file << "    <vertex x='" << x4 << "' y='0' z='" << z4 << "'/>\n";
            file << "  </triangle>\n";
        }
    }

    file << "</plane>\n";
    file.close();
    
    cout << "Ficheiro guardado em: " << filePath << endl;
}

//Cubo
void generateBox(float size, int divisions, const string& filename) {
    string filePath = caminhoFicheiro(filename);
    
    ofstream file(filePath);
    if (!file.is_open()) {
        cerr << "Erro ao abrir o ficheiro: " << filePath << endl;
        return;
    }

    file << "<box>\n";

    float halfSize = size / 2.0f;
    float step = size / divisions;

    for (int face = 0; face < 6; face++) {
        for (int i = 0; i < divisions; i++) {
            for (int j = 0; j < divisions; j++) {
                float x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4;
                
                // Calcula
                float u1 = -halfSize + j * step;
                float u2 = -halfSize + (j + 1) * step;
                float v1 = -halfSize + i * step;
                float v2 = -halfSize + (i + 1) * step;
                
                // Define vertices
                switch (face) {
                    case 0: // Front face (Z = halfSize)
                        x1 = u1; y1 = v1; z1 = halfSize;
                        x2 = u2; y2 = v1; z2 = halfSize;
                        x3 = u1; y3 = v2; z3 = halfSize;
                        x4 = u2; y4 = v2; z4 = halfSize;
                        break;
                    case 1: // Back face (Z = -halfSize)
                        x1 = u2; y1 = v1; z1 = -halfSize;
                        x2 = u1; y2 = v1; z2 = -halfSize;
                        x3 = u2; y3 = v2; z3 = -halfSize;
                        x4 = u1; y4 = v2; z4 = -halfSize;
                        break;
                    case 2: // Top face (Y = halfSize)
                        x1 = u1; y1 = halfSize; z1 = v2;
                        x2 = u2; y2 = halfSize; z2 = v2;
                        x3 = u1; y3 = halfSize; z3 = v1;
                        x4 = u2; y4 = halfSize; z4 = v1;
                        break;
                    case 3: // Bottom face (Y = -halfSize)
                        x1 = u1; y1 = -halfSize; z1 = v1;
                        x2 = u2; y2 = -halfSize; z2 = v1;
                        x3 = u1; y3 = -halfSize; z3 = v2;
                        x4 = u2; y4 = -halfSize; z4 = v2;
                        break;
                    case 4: // Left face (X = -halfSize)
                        x1 = -halfSize; y1 = v1; z1 = u2;
                        x2 = -halfSize; y2 = v1; z2 = u1;
                        x3 = -halfSize; y3 = v2; z3 = u2;
                        x4 = -halfSize; y4 = v2; z4 = u1;
                        break;
                    case 5: // Right face (X = halfSize)
                        x1 = halfSize; y1 = v1; z1 = u1;
                        x2 = halfSize; y2 = v1; z2 = u2;
                        x3 = halfSize; y3 = v2; z3 = u1;
                        x4 = halfSize; y4 = v2; z4 = u2;
                        break;
                }

                // Triangulo 1
                file << "  <triangle>\n";
                file << "    <vertex x='" << x1 << "' y='" << y1 << "' z='" << z1 << "'/>\n";
                file << "    <vertex x='" << x3 << "' y='" << y3 << "' z='" << z3 << "'/>\n";
                file << "    <vertex x='" << x2 << "' y='" << y2 << "' z='" << z2 << "'/>\n";
                file << "  </triangle>\n";

                // Triangulo 2
                file << "  <triangle>\n";
                file << "    <vertex x='" << x2 << "' y='" << y2 << "' z='" << z2 << "'/>\n";
                file << "    <vertex x='" << x3 << "' y='" << y3 << "' z='" << z3 << "'/>\n";
                file << "    <vertex x='" << x4 << "' y='" << y4 << "' z='" << z4 << "'/>\n";
                file << "  </triangle>\n";
            }
        }
    }

    file << "</box>\n";
    file.close();
    
    cout << "Ficheiro guardado em: " << filePath << endl;
}

//Esfera
void generateSphere(float radius, int slices, int stacks, const string& filename) {
    string filePath = caminhoFicheiro(filename);
    
    ofstream file(filePath);
    if (!file.is_open()) {
        cerr << "Erro ao abrir o ficheiro: " << filePath << endl;
        return;
    }

    file << "<sphere>\n";

    for (int i = 0; i < stacks; i++) {
        float theta1 = M_PI * i / stacks;
        float theta2 = M_PI * (i + 1) / stacks;

        for (int j = 0; j < slices; j++) {
            float phi1 = 2 * M_PI * j / slices;
            float phi2 = 2 * M_PI * (j + 1) / slices;

            float x1 = radius * sin(theta1) * cos(phi1);
            float y1 = radius * cos(theta1);
            float z1 = radius * sin(theta1) * sin(phi1);

            float x2 = radius * sin(theta1) * cos(phi2);
            float y2 = radius * cos(theta1);
            float z2 = radius * sin(theta1) * sin(phi2);

            float x3 = radius * sin(theta2) * cos(phi1);
            float y3 = radius * cos(theta2);
            float z3 = radius * sin(theta2) * sin(phi1);

            float x4 = radius * sin(theta2) * cos(phi2);
            float y4 = radius * cos(theta2);
            float z4 = radius * sin(theta2) * sin(phi2);

            // Triângulo 1
            file << "  <triangle>\n";
            file << "    <vertex x='" << x1 << "' y='" << y1 << "' z='" << z1 << "'/>\n";
            file << "    <vertex x='" << x3 << "' y='" << y3 << "' z='" << z3 << "'/>\n";
            file << "    <vertex x='" << x2 << "' y='" << y2 << "' z='" << z2 << "'/>\n";
            file << "  </triangle>\n";

            // Triângulo 2
            file << "  <triangle>\n";
            file << "    <vertex x='" << x2 << "' y='" << y2 << "' z='" << z2 << "'/>\n";
            file << "    <vertex x='" << x3 << "' y='" << y3 << "' z='" << z3 << "'/>\n";
            file << "    <vertex x='" << x4 << "' y='" << y4 << "' z='" << z4 << "'/>\n";
            file << "  </triangle>\n";
        }
    }

    file << "</sphere>\n";
    file.close();
    
    cout << "Ficheiro guardado em: " << filePath << endl;
}

//Cone
void generateCone(float radius, float height, int slices, int stacks, const string& filename) {
    string filePath = caminhoFicheiro(filename);
    
    ofstream file(filePath);
    if (!file.is_open()) {
        cerr << "Erro ao abrir o ficheiro: " << filePath << endl;
        return;
    }

    file << "<cone>\n";

    // Generate lado do cone
    for (int i = 0; i < stacks; i++) {
        float y1 = height * i / stacks;
        float y2 = height * (i + 1) / stacks;
        
        // Calcula raio
        float r1 = radius * (1 - y1 / height);
        float r2 = radius * (1 - y2 / height);

        for (int j = 0; j < slices; j++) {
            float theta1 = 2 * M_PI * j / slices;
            float theta2 = 2 * M_PI * (j + 1) / slices;

            float x1 = r1 * cos(theta1);
            float z1 = r1 * sin(theta1);

            float x2 = r1 * cos(theta2);
            float z2 = r1 * sin(theta2);

            if (i == stacks - 1) {
                file << "  <triangle>\n";
                file << "    <vertex x='" << x1 << "' y='" << y1 << "' z='" << z1 << "'/>\n";
                file << "    <vertex x='" << x2 << "' y='" << y1 << "' z='" << z2 << "'/>\n";
                file << "    <vertex x='0' y='" << height << "' z='0'/>\n";
                file << "  </triangle>\n";
            } else {

                float x3 = r2 * cos(theta1);
                float z3 = r2 * sin(theta1);
                float x4 = r2 * cos(theta2);
                float z4 = r2 * sin(theta2);

                // Triangle 1
                file << "  <triangle>\n";
                file << "    <vertex x='" << x1 << "' y='" << y1 << "' z='" << z1 << "'/>\n";
                file << "    <vertex x='" << x2 << "' y='" << y1 << "' z='" << z2 << "'/>\n";
                file << "    <vertex x='" << x3 << "' y='" << y2 << "' z='" << z3 << "'/>\n";
                file << "  </triangle>\n";

                // Triangle 2
                file << "  <triangle>\n";
                file << "    <vertex x='" << x2 << "' y='" << y1 << "' z='" << z2 << "'/>\n";
                file << "    <vertex x='" << x4 << "' y='" << y2 << "' z='" << z4 << "'/>\n";
                file << "    <vertex x='" << x3 << "' y='" << y2 << "' z='" << z3 << "'/>\n";
                file << "  </triangle>\n";
            }
        }
    }

    // Generate the base of the cone
    for (int j = 0; j < slices; j++) {
        float theta1 = 2 * M_PI * j / slices;
        float theta2 = 2 * M_PI * (j + 1) / slices;

        float x1 = radius * cos(theta1);
        float z1 = radius * sin(theta1);
        float x2 = radius * cos(theta2);
        float z2 = radius * sin(theta2);

        // Create triangle connecting points to center
        file << "  <triangle>\n";
        file << "    <vertex x='0' y='0' z='0'/>\n";
        file << "    <vertex x='" << x1 << "' y='0' z='" << z1 << "'/>\n";
        file << "    <vertex x='" << x2 << "' y='0' z='" << z2 << "'/>\n";
        file << "  </triangle>\n";
    }

    file << "</cone>\n";
    file.close();
    
    cout << "Ficheiro guardado em: " << filePath << endl;
}

//Main
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Parâmetros inválidos." << endl;
        return 1;
    }

    string shape = argv[1];

    if (shape == "sphere" && argc == 6) {
        float radius = atof(argv[2]);
        int slices = atoi(argv[3]);
        int stacks = atoi(argv[4]);
        string filename = argv[5];

        cout << "Gerando esfera: Raio=" << radius << ", Slices=" << slices
             << ", Stacks=" << stacks << ", Ficheiro=" << filename << endl;

        generateSphere(radius, slices, stacks, filename);
    }
    else if (shape == "plane" && argc == 5) {
        float length = atof(argv[2]);
        int divisions = atoi(argv[3]);
        string filename = argv[4];

        cout << "Gerando plano: Comprimento=" << length << ", Divisões=" << divisions
             << ", Ficheiro=" << filename << endl;

        generatePlane(length, divisions, filename);
    }
    else if (shape == "box" && argc == 5) {
        float size = atof(argv[2]);
        int divisions = atoi(argv[3]);
        string filename = argv[4];

        cout << "Gerando cubo: Tamanho=" << size << ", Divisões=" << divisions
             << ", Ficheiro=" << filename << endl;

        generateBox(size, divisions, filename);
    }
    else if (shape == "cone" && argc == 7) {
        float radius = atof(argv[2]);
        float height = atof(argv[3]);
        int slices = atoi(argv[4]);
        int stacks = atoi(argv[5]);
        string filename = argv[6];

        cout << "Gerando cone: Raio=" << radius << ", Altura=" << height
             << ", Slices=" << slices << ", Stacks=" << stacks
             << ", Ficheiro=" << filename << endl;

        generateCone(radius, height, slices, stacks, filename);
    }
    else {
        cout << "Parâmetros inválidos." << endl;
        return 1;
    }

    return 0;
}