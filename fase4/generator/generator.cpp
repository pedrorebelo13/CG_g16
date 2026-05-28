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
    string dir = "files3d";
    if (!fs::exists(dir)) fs::create_directory(dir);
    return dir + "/" + filename;
}

struct Vec3 {
    float x, y, z;
    Vec3(float x=0, float y=0, float z=0) : x(x), y(y), z(z) {}
    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s)       const { return {x*s,   y*s,   z*s};   }
};

// Produto externo de dois vetores — usado para calcular normais de Bézier
Vec3 cross(const Vec3& a, const Vec3& b) {
    return { a.y*b.z - a.z*b.y,
             a.z*b.x - a.x*b.z,
             a.x*b.y - a.y*b.x };
}

// Normaliza um vetor — garante que as normais têm comprimento 1
Vec3 normalize(const Vec3& v) {
    float len = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len < 1e-6f) return {0,1,0};
    return {v.x/len, v.y/len, v.z/len};
}

// Escreve um vértice completo com posição, normal e coordenadas de textura.
// Todos os geradores usam esta função para manter o formato consistente.
// O engine lê estes atributos pela mesma ordem: x,y,z,nx,ny,nz,s,t.
void writeVertex(ofstream& f,
                 float x, float y, float z,
                 float nx, float ny, float nz,
                 float s, float t) {
    f << "    <vertex x='" << x  << "' y='" << y  << "' z='" << z  << "'"
      << " nx='" << nx << "' ny='" << ny << "' nz='" << nz << "'"
      << " s='"  << s  << "' t='"  << t  << "'/>\n";
}


// ─────────────────────────────────────────────────────────────────────────────
// Plano
// Normal: sempre (0,1,0) — face virada para cima no plano XZ.
// Textura: s = j/divisions, t = i/divisions — mapeamento direto na grelha.
// ─────────────────────────────────────────────────────────────────────────────
void generatePlane(float length, int divisions, const string& filename) {
    ofstream file(caminhoFicheiro(filename));
    file << "<plane>\n";
    float step = length / divisions, start = -length / 2;
    for (int i = 0; i < divisions; i++) {
        for (int j = 0; j < divisions; j++) {
            float x1=start+j*step,     z1=start+i*step;
            float x2=start+(j+1)*step, z2=start+i*step;
            float x3=start+j*step,     z3=start+(i+1)*step;
            float x4=start+(j+1)*step, z4=start+(i+1)*step;

            // Coordenadas de textura: interpolam de 0 a 1 ao longo da grelha
            float s1=(float)j/divisions,     t1=(float)i/divisions;
            float s2=(float)(j+1)/divisions, t2=(float)i/divisions;
            float s3=(float)j/divisions,     t3=(float)(i+1)/divisions;
            float s4=(float)(j+1)/divisions, t4=(float)(i+1)/divisions;

            file << "  <triangle>\n";
            writeVertex(file, x1,0,z1, 0,1,0, s1,t1);
            writeVertex(file, x3,0,z3, 0,1,0, s3,t3);
            writeVertex(file, x2,0,z2, 0,1,0, s2,t2);
            file << "  </triangle>\n";

            file << "  <triangle>\n";
            writeVertex(file, x2,0,z2, 0,1,0, s2,t2);
            writeVertex(file, x3,0,z3, 0,1,0, s3,t3);
            writeVertex(file, x4,0,z4, 0,1,0, s4,t4);
            file << "  </triangle>\n";
        }
    }
    file << "</plane>\n";
    cout << "Plano gerado: " << caminhoFicheiro(filename) << endl;
}


// ─────────────────────────────────────────────────────────────────────────────
// Caixa
// Normal: cada face tem uma normal constante apontando para fora.
//   Face 0 (Z+): (0,0,1)   Face 1 (Z-): (0,0,-1)
//   Face 2 (Y+): (0,1,0)   Face 3 (Y-): (0,-1,0)
//   Face 4 (X-): (-1,0,0)  Face 5 (X+): (1,0,0)
// Textura: mapeamento por quadrícula dentro de cada face.
// ─────────────────────────────────────────────────────────────────────────────
void generateBox(float size, int divisions, const string& filename) {
    ofstream file(caminhoFicheiro(filename));
    file << "<box>\n";
    float h = size/2, step = size/divisions;

    // Normais por face (exterior da caixa)
    float normals[6][3] = {
        { 0, 0, 1}, { 0, 0,-1},
        { 0, 1, 0}, { 0,-1, 0},
        {-1, 0, 0}, { 1, 0, 0}
    };

    for (int face = 0; face < 6; face++) {
        float nx=normals[face][0], ny=normals[face][1], nz=normals[face][2];
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
                // Textura: coordenadas normalizadas dentro da face
                float s1=(float)j/divisions,     t1=(float)i/divisions;
                float s2=(float)(j+1)/divisions, t2=(float)i/divisions;
                float s3=(float)j/divisions,     t3=(float)(i+1)/divisions;
                float s4=(float)(j+1)/divisions, t4=(float)(i+1)/divisions;

                file << "  <triangle>\n";
                writeVertex(file, x1,y1,z1, nx,ny,nz, s1,t1);
                writeVertex(file, x3,y3,z3, nx,ny,nz, s3,t3);
                writeVertex(file, x2,y2,z2, nx,ny,nz, s2,t2);
                file << "  </triangle>\n";
                file << "  <triangle>\n";
                writeVertex(file, x2,y2,z2, nx,ny,nz, s2,t2);
                writeVertex(file, x3,y3,z3, nx,ny,nz, s3,t3);
                writeVertex(file, x4,y4,z4, nx,ny,nz, s4,t4);
                file << "  </triangle>\n";
            }
        }
    }
    file << "</box>\n";
    cout << "Cubo gerado: " << caminhoFicheiro(filename) << endl;
}


// ─────────────────────────────────────────────────────────────────────────────
// Esfera
// Normal: para uma esfera centrada na origem, a normal de qualquer ponto é
//   simplesmente esse ponto dividido pelo raio: n = (x,y,z)/r.
//   Isto funciona porque todos os pontos estão na superfície de uma esfera,
//   onde o vetor posição é perpendicular à superfície.
// Textura: mapeamento esférico standard.
//   s = phi / (2*PI)  → varia de 0 a 1 ao longo de um paralelo (longitude)
//   t = theta / PI    → varia de 0 a 1 do polo norte ao polo sul (latitude)
// ─────────────────────────────────────────────────────────────────────────────
void generateSphere(float radius, int slices, int stacks, const string& filename) {
    ofstream file(caminhoFicheiro(filename));
    file << "<sphere>\n";
    for (int i = 0; i < stacks; i++) {
        float t1 = M_PI*i/stacks,     t2 = M_PI*(i+1)/stacks;
        float tv1 = (float)i/stacks,  tv2 = (float)(i+1)/stacks;
        for (int j = 0; j < slices; j++) {
            float p1 = 2*M_PI*j/slices,     p2 = 2*M_PI*(j+1)/slices;
            float sv1 = (float)j/slices,     sv2 = (float)(j+1)/slices;

            // Posições
            float x1=radius*sin(t1)*cos(p1), y1=radius*cos(t1), z1=radius*sin(t1)*sin(p1);
            float x2=radius*sin(t1)*cos(p2), y2=radius*cos(t1), z2=radius*sin(t1)*sin(p2);
            float x3=radius*sin(t2)*cos(p1), y3=radius*cos(t2), z3=radius*sin(t2)*sin(p1);
            float x4=radius*sin(t2)*cos(p2), y4=radius*cos(t2), z4=radius*sin(t2)*sin(p2);

            // Normais = posição / raio (vetor unitário radial)
            float nx1=x1/radius, ny1=y1/radius, nz1=z1/radius;
            float nx2=x2/radius, ny2=y2/radius, nz2=z2/radius;
            float nx3=x3/radius, ny3=y3/radius, nz3=z3/radius;
            float nx4=x4/radius, ny4=y4/radius, nz4=z4/radius;

            file << "  <triangle>\n";
            writeVertex(file, x1,y1,z1, nx1,ny1,nz1, sv1,tv1);
            writeVertex(file, x3,y3,z3, nx3,ny3,nz3, sv1,tv2);
            writeVertex(file, x2,y2,z2, nx2,ny2,nz2, sv2,tv1);
            file << "  </triangle>\n";

            file << "  <triangle>\n";
            writeVertex(file, x2,y2,z2, nx2,ny2,nz2, sv2,tv1);
            writeVertex(file, x3,y3,z3, nx3,ny3,nz3, sv1,tv2);
            writeVertex(file, x4,y4,z4, nx4,ny4,nz4, sv2,tv2);
            file << "  </triangle>\n";
        }
    }
    file << "</sphere>\n";
    cout << "Esfera gerada: " << caminhoFicheiro(filename) << endl;
}


// ─────────────────────────────────────────────────────────────────────────────
// Cone
// Normal lateral: a superfície lateral de um cone é uma superfície reglada.
//   Para um cone de raio r e altura h, a normal num ponto (x,y,z) da lateral
//   é: n = normalize( (x, r/h, z) )   [componente Y = r/h porque é o seno
//   do ângulo de abertura do cone]
// Normal da base: (0,-1,0) — aponta para baixo (Y negativo).
// Textura: mapeamento cilíndrico na lateral; polar na base.
// ─────────────────────────────────────────────────────────────────────────────
void generateCone(float radius, float height, int slices, int stacks, const string& filename) {
    ofstream file(caminhoFicheiro(filename));
    file << "<cone>\n";

    // Inclinação da normal lateral: componente Y = r/h (seno do ângulo do cone)
    float slopeY = radius / height;

    for (int i = 0; i < stacks; i++) {
        float y1=height*i/stacks,     y2=height*(i+1)/stacks;
        float r1=radius*(1-y1/height), r2=radius*(1-y2/height);
        float tv1=(float)i/stacks,    tv2=(float)(i+1)/stacks;

        for (int j = 0; j < slices; j++) {
            float t1=2*M_PI*j/slices, t2=2*M_PI*(j+1)/slices;
            float sv1=(float)j/slices, sv2=(float)(j+1)/slices;

            float x1=r1*cos(t1), z1=r1*sin(t1);
            float x2=r1*cos(t2), z2=r1*sin(t2);

            // Normal lateral: normaliza (x, r/h, z) para cada ponto
            Vec3 n1 = normalize({cos(t1), slopeY, sin(t1)});
            Vec3 n2 = normalize({cos(t2), slopeY, sin(t2)});

            if (i == stacks-1) {
                // Último stack: triângulo até ao ápice
                // O ápice tem normal média dos dois vértices da base do triângulo
                Vec3 nApex = normalize({(n1.x+n2.x)*0.5f, (n1.y+n2.y)*0.5f, (n1.z+n2.z)*0.5f});
                file << "  <triangle>\n";
                writeVertex(file, x1,y1,z1, n1.x,n1.y,n1.z, sv1,tv1);
                writeVertex(file, x2,y1,z2, n2.x,n2.y,n2.z, sv2,tv1);
                writeVertex(file, 0,height,0, nApex.x,nApex.y,nApex.z, (sv1+sv2)*0.5f,1.0f);
                file << "  </triangle>\n";
            } else {
                float x3=r2*cos(t1), z3=r2*sin(t1);
                float x4=r2*cos(t2), z4=r2*sin(t2);
                Vec3 n3 = normalize({cos(t1), slopeY, sin(t1)});
                Vec3 n4 = normalize({cos(t2), slopeY, sin(t2)});

                file << "  <triangle>\n";
                writeVertex(file, x1,y1,z1, n1.x,n1.y,n1.z, sv1,tv1);
                writeVertex(file, x2,y1,z2, n2.x,n2.y,n2.z, sv2,tv1);
                writeVertex(file, x3,y2,z3, n3.x,n3.y,n3.z, sv1,tv2);
                file << "  </triangle>\n";
                file << "  <triangle>\n";
                writeVertex(file, x2,y1,z2, n2.x,n2.y,n2.z, sv2,tv1);
                writeVertex(file, x4,y2,z4, n4.x,n4.y,n4.z, sv2,tv2);
                writeVertex(file, x3,y2,z3, n3.x,n3.y,n3.z, sv1,tv2);
                file << "  </triangle>\n";
            }
        }
    }

    // Base do cone: normal (0,-1,0) para todos os vértices
    for (int j = 0; j < slices; j++) {
        float t1=2*M_PI*j/slices, t2=2*M_PI*(j+1)/slices;
        float x1=radius*cos(t1), z1=radius*sin(t1);
        float x2=radius*cos(t2), z2=radius*sin(t2);
        // Textura polar: centro = (0.5, 0.5), bordas a distância 0.5
        float s0=0.5f+0.5f*cos(t1), t0=0.5f+0.5f*sin(t1);
        float s1=0.5f+0.5f*cos(t2), t1b=0.5f+0.5f*sin(t2);
        file << "  <triangle>\n";
        writeVertex(file, 0,0,0,   0,-1,0, 0.5f,0.5f);
        writeVertex(file, x1,0,z1, 0,-1,0, s0,t0);
        writeVertex(file, x2,0,z2, 0,-1,0, s1,t1b);
        file << "  </triangle>\n";
    }
    file << "</cone>\n";
    cout << "Cone gerado: " << caminhoFicheiro(filename) << endl;
}


// ─────────────────────────────────────────────────────────────────────────────
// Bézier bicúbico
// Normal: produto externo das derivadas parciais ∂P/∂u × ∂P/∂v.
//   A derivada em u usa dT = [3t², 2t, 1, 0] em vez de T = [t³, t², t, 1].
//   O produto externo das duas derivadas dá o vetor perpendicular à superfície.
// Textura: s = u, t = v (mapeamento direto do parâmetro da superfície).
// ─────────────────────────────────────────────────────────────────────────────

static const float MB[4][4] = {
    {-1,  3, -3,  1},
    { 3, -6,  3,  0},
    {-3,  3,  0,  0},
    { 1,  0,  0,  0}
};

float bezier1D(float t, float p0, float p1, float p2, float p3) {
    float T[4] = { t*t*t, t*t, t, 1.0f };
    float P[4] = { p0, p1, p2, p3 };
    float tmp[4] = {0,0,0,0};
    for (int i=0;i<4;i++) for (int j=0;j<4;j++) tmp[i]+=MB[i][j]*P[j];
    float res=0; for (int i=0;i<4;i++) res+=T[i]*tmp[i];
    return res;
}

// Derivada de Bézier cúbico (usa dT = [3t², 2t, 1, 0])
float bezier1D_deriv(float t, float p0, float p1, float p2, float p3) {
    float dT[4] = { 3*t*t, 2*t, 1.0f, 0.0f };
    float P[4] = { p0, p1, p2, p3 };
    float tmp[4] = {0,0,0,0};
    for (int i=0;i<4;i++) for (int j=0;j<4;j++) tmp[i]+=MB[i][j]*P[j];
    float res=0; for (int i=0;i<4;i++) res+=dT[i]*tmp[i];
    return res;
}

Vec3 bezierSurface(float u, float v, const Vec3 patch[4][4]) {
    Vec3 q[4];
    for (int i=0;i<4;i++) {
        q[i].x=bezier1D(u,patch[i][0].x,patch[i][1].x,patch[i][2].x,patch[i][3].x);
        q[i].y=bezier1D(u,patch[i][0].y,patch[i][1].y,patch[i][2].y,patch[i][3].y);
        q[i].z=bezier1D(u,patch[i][0].z,patch[i][1].z,patch[i][2].z,patch[i][3].z);
    }
    Vec3 r;
    r.x=bezier1D(v,q[0].x,q[1].x,q[2].x,q[3].x);
    r.y=bezier1D(v,q[0].y,q[1].y,q[2].y,q[3].y);
    r.z=bezier1D(v,q[0].z,q[1].z,q[2].z,q[3].z);
    return r;
}

// Derivada parcial ∂P/∂u (mantém v fixo, deriva em u)
Vec3 bezierSurface_dU(float u, float v, const Vec3 patch[4][4]) {
    Vec3 q[4];
    for (int i=0;i<4;i++) {
        q[i].x=bezier1D_deriv(u,patch[i][0].x,patch[i][1].x,patch[i][2].x,patch[i][3].x);
        q[i].y=bezier1D_deriv(u,patch[i][0].y,patch[i][1].y,patch[i][2].y,patch[i][3].y);
        q[i].z=bezier1D_deriv(u,patch[i][0].z,patch[i][1].z,patch[i][2].z,patch[i][3].z);
    }
    Vec3 r;
    r.x=bezier1D(v,q[0].x,q[1].x,q[2].x,q[3].x);
    r.y=bezier1D(v,q[0].y,q[1].y,q[2].y,q[3].y);
    r.z=bezier1D(v,q[0].z,q[1].z,q[2].z,q[3].z);
    return r;
}

// Derivada parcial ∂P/∂v (mantém u fixo, deriva em v)
Vec3 bezierSurface_dV(float u, float v, const Vec3 patch[4][4]) {
    Vec3 q[4];
    for (int i=0;i<4;i++) {
        q[i].x=bezier1D(u,patch[i][0].x,patch[i][1].x,patch[i][2].x,patch[i][3].x);
        q[i].y=bezier1D(u,patch[i][0].y,patch[i][1].y,patch[i][2].y,patch[i][3].y);
        q[i].z=bezier1D(u,patch[i][0].z,patch[i][1].z,patch[i][2].z,patch[i][3].z);
    }
    Vec3 r;
    r.x=bezier1D_deriv(v,q[0].x,q[1].x,q[2].x,q[3].x);
    r.y=bezier1D_deriv(v,q[0].y,q[1].y,q[2].y,q[3].y);
    r.z=bezier1D_deriv(v,q[0].z,q[1].z,q[2].z,q[3].z);
    return r;
}

void generateBezier(const string& patchFile, int tessLevel, const string& outFile) {
    ifstream in(patchFile);
    if (!in.is_open()) { cerr<<"Erro ao abrir: "<<patchFile<<endl; return; }

    int numPatches; in >> numPatches;
    vector<vector<int>> patchIndices(numPatches, vector<int>(16));
    for (int p=0;p<numPatches;p++) {
        for (int i=0;i<16;i++) {
            char sep; in>>patchIndices[p][i];
            if(i<15) in>>sep;
        }
    }
    int numPoints; in>>numPoints;
    vector<Vec3> controlPoints(numPoints);
    for (int i=0;i<numPoints;i++) {
        char sep;
        in>>controlPoints[i].x>>sep>>controlPoints[i].y>>sep>>controlPoints[i].z;
    }
    in.close();

    string outPath = caminhoFicheiro(outFile);
    ofstream out(outPath);
    out << "<bezier>\n";
    int triCount=0;

    for (int p=0;p<numPatches;p++) {
        Vec3 patch[4][4];
        for (int i=0;i<4;i++)
            for (int j=0;j<4;j++)
                patch[i][j]=controlPoints[patchIndices[p][i*4+j]];

        float step=1.0f/tessLevel;
        for (int i=0;i<tessLevel;i++) {
            for (int j=0;j<tessLevel;j++) {
                float u0=i*step, u1=(i+1)*step;
                float v0=j*step, v1=(j+1)*step;

                Vec3 p00=bezierSurface(u0,v0,patch);
                Vec3 p10=bezierSurface(u1,v0,patch);
                Vec3 p01=bezierSurface(u0,v1,patch);
                Vec3 p11=bezierSurface(u1,v1,patch);

                // Normais: produto externo ∂P/∂u × ∂P/∂v em cada vértice
                Vec3 n00=normalize(cross(bezierSurface_dU(u0,v0,patch),bezierSurface_dV(u0,v0,patch)));
                Vec3 n10=normalize(cross(bezierSurface_dU(u1,v0,patch),bezierSurface_dV(u1,v0,patch)));
                Vec3 n01=normalize(cross(bezierSurface_dU(u0,v1,patch),bezierSurface_dV(u0,v1,patch)));
                Vec3 n11=normalize(cross(bezierSurface_dU(u1,v1,patch),bezierSurface_dV(u1,v1,patch)));

                // Textura: s=u, t=v (mapeamento direto dos parâmetros)
                out<<"  <triangle>\n";
                writeVertex(out,p00.x,p00.y,p00.z, n00.x,n00.y,n00.z, u0,v0);
                writeVertex(out,p10.x,p10.y,p10.z, n10.x,n10.y,n10.z, u1,v0);
                writeVertex(out,p11.x,p11.y,p11.z, n11.x,n11.y,n11.z, u1,v1);
                out<<"  </triangle>\n";

                out<<"  <triangle>\n";
                writeVertex(out,p00.x,p00.y,p00.z, n00.x,n00.y,n00.z, u0,v0);
                writeVertex(out,p11.x,p11.y,p11.z, n11.x,n11.y,n11.z, u1,v1);
                writeVertex(out,p01.x,p01.y,p01.z, n01.x,n01.y,n01.z, u0,v1);
                out<<"  </triangle>\n";
                triCount+=2;
            }
        }
    }
    out<<"</bezier>\n";
    cout<<"Bézier gerado: "<<outPath<<" ("<<numPatches<<" patches, tessLevel="<<tessLevel<<", "<<triCount<<" triângulos)"<<endl;
}


// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout<<"Uso:\n"
            <<"  generator sphere <radius> <slices> <stacks> <file>\n"
            <<"  generator plane  <length> <divisions> <file>\n"
            <<"  generator box    <size> <divisions> <file>\n"
            <<"  generator cone   <radius> <height> <slices> <stacks> <file>\n"
            <<"  generator bezier <patch_file> <tessLevel> <file>\n";
        return 1;
    }
    string shape=argv[1];
    if      (shape=="sphere" && argc==6) generateSphere(atof(argv[2]),atoi(argv[3]),atoi(argv[4]),argv[5]);
    else if (shape=="plane"  && argc==5) generatePlane(atof(argv[2]),atoi(argv[3]),argv[4]);
    else if (shape=="box"    && argc==5) generateBox(atof(argv[2]),atoi(argv[3]),argv[4]);
    else if (shape=="cone"   && argc==7) generateCone(atof(argv[2]),atof(argv[3]),atoi(argv[4]),atoi(argv[5]),argv[6]);
    else if (shape=="bezier" && argc==5) generateBezier(argv[2],atoi(argv[3]),argv[4]);
    else { cout<<"Parâmetros inválidos."<<endl; return 1; }
    return 0;
}