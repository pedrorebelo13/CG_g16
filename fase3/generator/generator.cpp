#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <string>
#include <vector>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Utilitários
// ─────────────────────────────────────────────────────────────────────────────

string caminhoFicheiro(const string& filename) {
    // Centraliza o output no mesmo diretório para manter engine/generator consistentes.
    string dir = "files3d";
    if (!fs::exists(dir)) fs::create_directory(dir);
    return dir + "/" + filename;
}

struct Vec3 {
    float x, y, z;
    Vec3(float x=0, float y=0, float z=0) : x(x), y(y), z(z) {}
    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator*(float s)       const { return {x*s,   y*s,   z*s};   }
};


// ─────────────────────────────────────────────────────────────────────────────
// Primitivas existentes (Fase 1, sem alterações)
// ─────────────────────────────────────────────────────────────────────────────

void generatePlane(float length, int divisions, const string& filename) {
    ofstream file(caminhoFicheiro(filename));
    file << "<plane>\n";
    float step = length / divisions, start = -length / 2;
    for (int i = 0; i < divisions; i++) {
        for (int j = 0; j < divisions; j++) {
            float x1=start+j*step, z1=start+i*step;
            float x2=start+(j+1)*step, z2=start+i*step;
            float x3=start+j*step, z3=start+(i+1)*step;
            float x4=start+(j+1)*step, z4=start+(i+1)*step;
            file << "  <triangle>\n"
                 << "    <vertex x='"<<x1<<"' y='0' z='"<<z1<<"'/>\n"
                 << "    <vertex x='"<<x3<<"' y='0' z='"<<z3<<"'/>\n"
                 << "    <vertex x='"<<x2<<"' y='0' z='"<<z2<<"'/>\n"
                 << "  </triangle>\n";
            file << "  <triangle>\n"
                 << "    <vertex x='"<<x2<<"' y='0' z='"<<z2<<"'/>\n"
                 << "    <vertex x='"<<x3<<"' y='0' z='"<<z3<<"'/>\n"
                 << "    <vertex x='"<<x4<<"' y='0' z='"<<z4<<"'/>\n"
                 << "  </triangle>\n";
        }
    }
    file << "</plane>\n";
    cout << "Plano gerado: " << caminhoFicheiro(filename) << endl;
}

void generateBox(float size, int divisions, const string& filename) {
    ofstream file(caminhoFicheiro(filename));
    file << "<box>\n";
    float h = size/2, step = size/divisions;
    for (int face = 0; face < 6; face++) {
        for (int i = 0; i < divisions; i++) {
            for (int j = 0; j < divisions; j++) {
                float u1=-h+j*step, u2=-h+(j+1)*step;
                float v1=-h+i*step, v2=-h+(i+1)*step;
                float x1,y1,z1,x2,y2,z2,x3,y3,z3,x4,y4,z4;
                switch(face){
                    case 0: x1=u1;y1=v1;z1=h;  x2=u2;y2=v1;z2=h;  x3=u1;y3=v2;z3=h;  x4=u2;y4=v2;z4=h; break;
                    case 1: x1=u2;y1=v1;z1=-h; x2=u1;y2=v1;z2=-h; x3=u2;y3=v2;z3=-h; x4=u1;y4=v2;z4=-h; break;
                    case 2: x1=u1;y1=h;z1=v2;  x2=u2;y2=h;z2=v2;  x3=u1;y3=h;z3=v1;  x4=u2;y4=h;z4=v1; break;
                    case 3: x1=u1;y1=-h;z1=v1; x2=u2;y2=-h;z2=v1; x3=u1;y3=-h;z3=v2; x4=u2;y4=-h;z4=v2; break;
                    case 4: x1=-h;y1=v1;z1=u2; x2=-h;y2=v1;z2=u1; x3=-h;y3=v2;z3=u2; x4=-h;y4=v2;z4=u1; break;
                    default:x1=h;y1=v1;z1=u1;  x2=h;y2=v1;z2=u2;  x3=h;y3=v2;z3=u1;  x4=h;y4=v2;z4=u2; break;
                }
                file << "  <triangle>\n"
                     << "    <vertex x='"<<x1<<"' y='"<<y1<<"' z='"<<z1<<"'/>\n"
                     << "    <vertex x='"<<x3<<"' y='"<<y3<<"' z='"<<z3<<"'/>\n"
                     << "    <vertex x='"<<x2<<"' y='"<<y2<<"' z='"<<z2<<"'/>\n"
                     << "  </triangle>\n"
                     << "  <triangle>\n"
                     << "    <vertex x='"<<x2<<"' y='"<<y2<<"' z='"<<z2<<"'/>\n"
                     << "    <vertex x='"<<x3<<"' y='"<<y3<<"' z='"<<z3<<"'/>\n"
                     << "    <vertex x='"<<x4<<"' y='"<<y4<<"' z='"<<z4<<"'/>\n"
                     << "  </triangle>\n";
            }
        }
    }
    file << "</box>\n";
    cout << "Cubo gerado: " << caminhoFicheiro(filename) << endl;
}

void generateSphere(float radius, int slices, int stacks, const string& filename) {
    ofstream file(caminhoFicheiro(filename));
    file << "<sphere>\n";
    for (int i = 0; i < stacks; i++) {
        float t1 = M_PI*i/stacks, t2 = M_PI*(i+1)/stacks;
        for (int j = 0; j < slices; j++) {
            float p1=2*M_PI*j/slices, p2=2*M_PI*(j+1)/slices;
            float x1=radius*sin(t1)*cos(p1), y1=radius*cos(t1), z1=radius*sin(t1)*sin(p1);
            float x2=radius*sin(t1)*cos(p2), y2=radius*cos(t1), z2=radius*sin(t1)*sin(p2);
            float x3=radius*sin(t2)*cos(p1), y3=radius*cos(t2), z3=radius*sin(t2)*sin(p1);
            float x4=radius*sin(t2)*cos(p2), y4=radius*cos(t2), z4=radius*sin(t2)*sin(p2);
            file << "  <triangle>\n"
                 << "    <vertex x='"<<x1<<"' y='"<<y1<<"' z='"<<z1<<"'/>\n"
                 << "    <vertex x='"<<x3<<"' y='"<<y3<<"' z='"<<z3<<"'/>\n"
                 << "    <vertex x='"<<x2<<"' y='"<<y2<<"' z='"<<z2<<"'/>\n"
                 << "  </triangle>\n"
                 << "  <triangle>\n"
                 << "    <vertex x='"<<x2<<"' y='"<<y2<<"' z='"<<z2<<"'/>\n"
                 << "    <vertex x='"<<x3<<"' y='"<<y3<<"' z='"<<z3<<"'/>\n"
                 << "    <vertex x='"<<x4<<"' y='"<<y4<<"' z='"<<z4<<"'/>\n"
                 << "  </triangle>\n";
        }
    }
    file << "</sphere>\n";
    cout << "Esfera gerada: " << caminhoFicheiro(filename) << endl;
}

void generateCone(float radius, float height, int slices, int stacks, const string& filename) {
    ofstream file(caminhoFicheiro(filename));
    file << "<cone>\n";
    for (int i = 0; i < stacks; i++) {
        float y1=height*i/stacks, y2=height*(i+1)/stacks;
        float r1=radius*(1-y1/height), r2=radius*(1-y2/height);
        for (int j = 0; j < slices; j++) {
            float t1=2*M_PI*j/slices, t2=2*M_PI*(j+1)/slices;
            float x1=r1*cos(t1), z1=r1*sin(t1), x2=r1*cos(t2), z2=r1*sin(t2);
            if (i == stacks-1) {
                file << "  <triangle>\n"
                     << "    <vertex x='"<<x1<<"' y='"<<y1<<"' z='"<<z1<<"'/>\n"
                     << "    <vertex x='"<<x2<<"' y='"<<y1<<"' z='"<<z2<<"'/>\n"
                     << "    <vertex x='0' y='"<<height<<"' z='0'/>\n"
                     << "  </triangle>\n";
            } else {
                float x3=r2*cos(t1), z3=r2*sin(t1), x4=r2*cos(t2), z4=r2*sin(t2);
                file << "  <triangle>\n"
                     << "    <vertex x='"<<x1<<"' y='"<<y1<<"' z='"<<z1<<"'/>\n"
                     << "    <vertex x='"<<x2<<"' y='"<<y1<<"' z='"<<z2<<"'/>\n"
                     << "    <vertex x='"<<x3<<"' y='"<<y2<<"' z='"<<z3<<"'/>\n"
                     << "  </triangle>\n"
                     << "  <triangle>\n"
                     << "    <vertex x='"<<x2<<"' y='"<<y1<<"' z='"<<z2<<"'/>\n"
                     << "    <vertex x='"<<x4<<"' y='"<<y2<<"' z='"<<z4<<"'/>\n"
                     << "    <vertex x='"<<x3<<"' y='"<<y2<<"' z='"<<z3<<"'/>\n"
                     << "  </triangle>\n";
            }
        }
    }
    // Base
    for (int j = 0; j < slices; j++) {
        float t1=2*M_PI*j/slices, t2=2*M_PI*(j+1)/slices;
        file << "  <triangle>\n"
             << "    <vertex x='0' y='0' z='0'/>\n"
             << "    <vertex x='"<<radius*cos(t1)<<"' y='0' z='"<<radius*sin(t1)<<"'/>\n"
             << "    <vertex x='"<<radius*cos(t2)<<"' y='0' z='"<<radius*sin(t2)<<"'/>\n"
             << "  </triangle>\n";
    }
    file << "</cone>\n";
    cout << "Cone gerado: " << caminhoFicheiro(filename) << endl;
}


// ─────────────────────────────────────────────────────────────────────────────
// Bézier bicúbico (NOVO na Fase 3)
// ─────────────────────────────────────────────────────────────────────────────

// Matriz de Bézier cúbica M
static const float MB[4][4] = {
    {-1,  3, -3,  1},
    { 3, -6,  3,  0},
    {-3,  3,  0,  0},
    { 1,  0,  0,  0}
};

// Avalia um polinómio de Bézier cúbico: U · M · P
// onde U = [t³, t², t, 1] e P são 4 pontos de controlo (escalares)
float bezier1D(float t, float p0, float p1, float p2, float p3) {
    float T[4] = { t*t*t, t*t, t, 1.0f };
    float P[4] = { p0, p1, p2, p3 };
    // Tmp = M · P
    float tmp[4] = {0,0,0,0};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            tmp[i] += MB[i][j] * P[j];
    // resultado = T · tmp
    float res = 0;
    for (int i = 0; i < 4; i++) res += T[i] * tmp[i];
    return res;
}

// Avalia a superfície de Bézier bicúbica P(u,v) para um patch de 16 pontos.
// Os pontos estão organizados como patch[linha][coluna], linha=v, coluna=u.
Vec3 bezierSurface(float u, float v, const Vec3 patch[4][4]) {
    // Calcula 4 pontos intermediários (cada um é um Bézier em u para cada linha)
    Vec3 q[4];
    for (int i = 0; i < 4; i++) {
        q[i].x = bezier1D(u, patch[i][0].x, patch[i][1].x, patch[i][2].x, patch[i][3].x);
        q[i].y = bezier1D(u, patch[i][0].y, patch[i][1].y, patch[i][2].y, patch[i][3].y);
        q[i].z = bezier1D(u, patch[i][0].z, patch[i][1].z, patch[i][2].z, patch[i][3].z);
    }
    // Interpola em v sobre os 4 pontos intermédios
    Vec3 result;
    result.x = bezier1D(v, q[0].x, q[1].x, q[2].x, q[3].x);
    result.y = bezier1D(v, q[0].y, q[1].y, q[2].y, q[3].y);
    result.z = bezier1D(v, q[0].z, q[1].z, q[2].z, q[3].z);
    return result;
}

// Lê um ficheiro .patch e gera os triângulos por tessellation.
// Formato do .patch:
//   Linha 1: número de patches
//   Linhas seguintes: 16 índices por patch (separados por vírgulas)
//   Depois: número de pontos de controlo
//   Linhas seguintes: x y z por ponto
void generateBezier(const string& patchFile, int tessLevel, const string& outFile) {
    ifstream in(patchFile);
    if (!in.is_open()) {
        cerr << "Erro ao abrir ficheiro de patches: " << patchFile << endl;
        return;
    }

    // Lê número de patches
    int numPatches;
    in >> numPatches;

    // Lê índices dos patches:
    // cada patch referencia 16 pontos do array global de controlPoints.
    vector<vector<int>> patchIndices(numPatches, vector<int>(16));
    for (int p = 0; p < numPatches; p++) {
        for (int i = 0; i < 16; i++) {
            char sep;
            in >> patchIndices[p][i];
            if (i < 15) in >> sep; // vírgula
        }
    }

    // Lê pontos de controlo (x,y,z). O ficheiro usa valores separados por vírgula.
    int numPoints;
    in >> numPoints;
    vector<Vec3> controlPoints(numPoints);
    for (int i = 0; i < numPoints; i++) {
        char sep;
        in >> controlPoints[i].x >> sep >> controlPoints[i].y >> sep >> controlPoints[i].z;
    }
    in.close();

    string outPath = caminhoFicheiro(outFile);
    ofstream out(outPath);
    out << "<bezier>\n";

    int triCount = 0;

    for (int p = 0; p < numPatches; p++) {
        // Monta a grelha 4×4 de pontos de controlo deste patch
        Vec3 patch[4][4];
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                patch[i][j] = controlPoints[patchIndices[p][i*4+j]];

        // Tessella o patch em tessLevel × tessLevel quadriláteros.
        // Cada quadrilátero vira 2 triângulos para manter o formato final .3d.
        float step = 1.0f / tessLevel;
        for (int i = 0; i < tessLevel; i++) {
            for (int j = 0; j < tessLevel; j++) {
                float u0 = i * step, u1 = (i+1) * step;
                float v0 = j * step, v1 = (j+1) * step;

                Vec3 p00 = bezierSurface(u0, v0, patch);
                Vec3 p10 = bezierSurface(u1, v0, patch);
                Vec3 p01 = bezierSurface(u0, v1, patch);
                Vec3 p11 = bezierSurface(u1, v1, patch);

                // Triângulo 1: p00, p10, p11
                out << "  <triangle>\n"
                    << "    <vertex x='"<<p00.x<<"' y='"<<p00.y<<"' z='"<<p00.z<<"'/>\n"
                    << "    <vertex x='"<<p10.x<<"' y='"<<p10.y<<"' z='"<<p10.z<<"'/>\n"
                    << "    <vertex x='"<<p11.x<<"' y='"<<p11.y<<"' z='"<<p11.z<<"'/>\n"
                    << "  </triangle>\n";

                // Triângulo 2: p00, p11, p01
                out << "  <triangle>\n"
                    << "    <vertex x='"<<p00.x<<"' y='"<<p00.y<<"' z='"<<p00.z<<"'/>\n"
                    << "    <vertex x='"<<p11.x<<"' y='"<<p11.y<<"' z='"<<p11.z<<"'/>\n"
                    << "    <vertex x='"<<p01.x<<"' y='"<<p01.y<<"' z='"<<p01.z<<"'/>\n"
                    << "  </triangle>\n";

                triCount += 2;
            }
        }
    }

    out << "</bezier>\n";
    cout << "Bézier gerado: " << outPath
         << " (" << numPatches << " patches, tessLevel=" << tessLevel
         << ", " << triCount << " triângulos)" << endl;
}


// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Uso:\n"
             << "  generator sphere <radius> <slices> <stacks> <file>\n"
             << "  generator plane  <length> <divisions> <file>\n"
             << "  generator box    <size> <divisions> <file>\n"
             << "  generator cone   <radius> <height> <slices> <stacks> <file>\n"
             << "  generator bezier <patch_file> <tessLevel> <file>\n";
        return 1;
    }

    string shape = argv[1];

    if (shape == "sphere" && argc == 6) {
        generateSphere(atof(argv[2]), atoi(argv[3]), atoi(argv[4]), argv[5]);
    } else if (shape == "plane" && argc == 5) {
        generatePlane(atof(argv[2]), atoi(argv[3]), argv[4]);
    } else if (shape == "box" && argc == 5) {
        generateBox(atof(argv[2]), atoi(argv[3]), argv[4]);
    } else if (shape == "cone" && argc == 7) {
        generateCone(atof(argv[2]), atof(argv[3]), atoi(argv[4]), atoi(argv[5]), argv[6]);
    } else if (shape == "bezier" && argc == 5) {
        // generator bezier <ficheiro.patch> <tessLevel> <saida.3d>
        generateBezier(argv[2], atoi(argv[3]), argv[4]);
    } else {
        cout << "Parâmetros inválidos." << endl;
        return 1;
    }

    return 0;
}