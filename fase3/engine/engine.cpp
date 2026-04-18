#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <math.h>
#include <GL/glut.h>
#include "tinyxml2.h"
#include "camera.h"
#include "parser.h"

using namespace std;
using namespace tinyxml2;

// ─────────────────────────────────────────────────────────────────────────────
// Estruturas de dados
// ─────────────────────────────────────────────────────────────────────────────

// Vértice com posição. Na Fase 4 basta acrescentar nx,ny,nz e u,v aqui.
struct Vertex {
    float x, y, z;
    Vertex() : x(0), y(0), z(0) {}
    Vertex(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

// Face triangular definida por 3 índices no array de vértices.
// Mantida da Fase 2: permite deduplicação de vértices e uso de EBO.
struct Face {
    int v1, v2, v3;
    Face() : v1(0), v2(0), v3(0) {}
    Face(int _v1, int _v2, int _v3) : v1(_v1), v2(_v2), v3(_v3) {}
};

// ModelData usa dois buffers na GPU:
//   vboId — vértices (GL_ARRAY_BUFFER)
//   eboId — índices  (GL_ELEMENT_ARRAY_BUFFER)
// Na Fase 4 basta estender o Vertex e re-fazer o upload do vboId.
struct ModelData {
    string filename;
    vector<Vertex> vertices;  // cópia CPU (útil para cálculo de normais na Fase 4)
    vector<Face>   faces;     // cópia CPU (útil para debug)
    GLuint vboId;
    GLuint eboId;
    int indexCount;
    bool loaded;

    ModelData() : vboId(0), eboId(0), indexCount(0), loaded(false) {}
};


// ─────────────────────────────────────────────────────────────────────────────
// Variáveis globais
// ─────────────────────────────────────────────────────────────────────────────

Window gWindow;
Camera* camera;
Group sceneRootGroup;
map<string, ModelData> modelDataMap;
// Chave = nome do ficheiro no XML, valor = buffers GPU já criados.
// Evita carregar o mesmo .3d múltiplas vezes quando aparece em vários grupos.

bool showAxes = false;
bool wireframeMode = false;


// ─────────────────────────────────────────────────────────────────────────────
// Catmull-Rom
// ─────────────────────────────────────────────────────────────────────────────

static const float CR[4][4] = {
    {-0.5f,  1.5f, -1.5f,  0.5f},
    { 1.0f, -2.5f,  2.0f, -0.5f},
    {-0.5f,  0.0f,  0.5f,  0.0f},
    { 0.0f,  1.0f,  0.0f,  0.0f}
};

void catmullRomPoint(float t,
                     const Point3D& p0, const Point3D& p1,
                     const Point3D& p2, const Point3D& p3,
                     float* pos, float* deriv) {
    float T[4]  = { t*t*t, t*t, t, 1.0f };
    float dT[4] = { 3*t*t, 2*t, 1.0f, 0.0f };
    float px[4]={p0.x,p1.x,p2.x,p3.x};
    float py[4]={p0.y,p1.y,p2.y,p3.y};
    float pz[4]={p0.z,p1.z,p2.z,p3.z};

    for (int axis = 0; axis < 3; axis++) {
        float* P = (axis==0)?px:(axis==1)?py:pz;
        float tmp[4]={0,0,0,0};
        for (int i=0;i<4;i++) for (int j=0;j<4;j++) tmp[i]+=CR[i][j]*P[j];
        pos[axis]=0; deriv[axis]=0;
        for (int i=0;i<4;i++) { pos[axis]+=T[i]*tmp[i]; deriv[axis]+=dT[i]*tmp[i]; }
    }
}

void getCatmullRomPosition(float globalT, const vector<Point3D>& pts,
                           float* pos, float* deriv) {
    // Usa uma spline "encadeada" por segmentos [p0..p3], [p1..p4], ...
    // globalT está normalizado em [0,1[ para facilitar ciclos infinitos.
    int numSeg=(int)pts.size()-3;
    float ts=globalT*numSeg;
    int seg=(int)ts; if(seg>=numSeg) seg=numSeg-1;
    catmullRomPoint(ts-seg,pts[seg],pts[seg+1],pts[seg+2],pts[seg+3],pos,deriv);
}

void buildAlignMatrix(float* deriv, float* matrix) {
    float len=sqrt(deriv[0]*deriv[0]+deriv[1]*deriv[1]+deriv[2]*deriv[2]);
    if(len<1e-6f) len=1.0f;
    float X[3]={deriv[0]/len,deriv[1]/len,deriv[2]/len};
    float Ztmp[3]={0,1,0};
    float dot=X[0]*Ztmp[0]+X[1]*Ztmp[1]+X[2]*Ztmp[2];
    if(fabs(dot)>0.99f){Ztmp[0]=0;Ztmp[1]=0;Ztmp[2]=1;}
    float Y[3]={Ztmp[1]*X[2]-Ztmp[2]*X[1],Ztmp[2]*X[0]-Ztmp[0]*X[2],Ztmp[0]*X[1]-Ztmp[1]*X[0]};
    len=sqrt(Y[0]*Y[0]+Y[1]*Y[1]+Y[2]*Y[2]); if(len<1e-6f)len=1.0f;
    Y[0]/=len;Y[1]/=len;Y[2]/=len;
    float Z[3]={X[1]*Y[2]-X[2]*Y[1],X[2]*Y[0]-X[0]*Y[2],X[0]*Y[1]-X[1]*Y[0]};
    matrix[ 0]=X[0];matrix[ 1]=X[1];matrix[ 2]=X[2];matrix[ 3]=0;
    matrix[ 4]=Y[0];matrix[ 5]=Y[1];matrix[ 6]=Y[2];matrix[ 7]=0;
    matrix[ 8]=Z[0];matrix[ 9]=Z[1];matrix[10]=Z[2];matrix[11]=0;
    matrix[12]=0;   matrix[13]=0;   matrix[14]=0;   matrix[15]=1;
}


// ─────────────────────────────────────────────────────────────────────────────
// Carregamento de modelos com VBO + EBO
// ─────────────────────────────────────────────────────────────────────────────

bool fileExists(const string& path){ ifstream f(path); return f.good(); }

string resolveModelPath(const string& modelFile, const string& configFilePath) {
    if(fileExists(modelFile)) return modelFile;
    size_t pos=configFilePath.find_last_of("/\\");
    if(pos!=string::npos){
        string p=configFilePath.substr(0,pos)+"/"+modelFile;
        if(fileExists(p)) return p;
    }
    string g1="../generator/files3d/"+modelFile;
    if(fileExists(g1)) return g1;
    string g2="generator/files3d/"+modelFile;
    if(fileExists(g2)) return g2;
    return modelFile;
}

void collectModelReferences(const Group& group, vector<string>& refs) {
    for(const Model& m:group.models) refs.push_back(m.filename);
    for(const Group& c:group.children) collectModelReferences(c,refs);
}

// Carrega o modelo XML, deduplica vértices (igual à Fase 2) e faz upload
// para dois buffers na GPU: VBO (vértices) e EBO (índices).
//
// Para a Fase 4: estender Vertex com nx,ny,nz e u,v e re-fazer só o upload do VBO.
// O EBO não precisa de ser alterado — os índices são os mesmos.
bool loadModel(ModelData& modelData, const string& filename) {
    ifstream file(filename);
    if(!file.is_open()){ cerr<<"Erro ao abrir: "<<filename<<endl; return false; }
    modelData.filename=filename;
    modelData.vertices.clear();
    modelData.faces.clear();

    string content((istreambuf_iterator<char>(file)),istreambuf_iterator<char>());
    file.close();

    XMLDocument doc;
    if(doc.Parse(content.c_str())!=XML_SUCCESS){ cerr<<"Erro XML: "<<filename<<endl; return false; }
    XMLElement* root=doc.RootElement();
    if(!root) return false;

    // Deduplicação de vértices (igual à Fase 2)
    map<string,int> vertexIndex;
    int nextIdx=0;

    XMLElement* tri=root->FirstChildElement("triangle");
    while(tri){
        XMLElement* v=tri->FirstChildElement("vertex");
        vector<int> idxs;
        while(v && (int)idxs.size()<3){
            float x=0,y=0,z=0;
            v->QueryFloatAttribute("x",&x);
            v->QueryFloatAttribute("y",&y);
            v->QueryFloatAttribute("z",&z);
            string key=to_string(x)+","+to_string(y)+","+to_string(z);
            if(vertexIndex.find(key)==vertexIndex.end()){
                modelData.vertices.push_back(Vertex(x,y,z));
                vertexIndex[key]=nextIdx++;
            }
            idxs.push_back(vertexIndex[key]);
            v=v->NextSiblingElement("vertex");
        }
        if((int)idxs.size()==3)
            modelData.faces.push_back(Face(idxs[0],idxs[1],idxs[2]));
        tri=tri->NextSiblingElement("triangle");
    }

    modelData.indexCount=(int)modelData.faces.size()*3;

    // ── Upload VBO (vértices únicos) ──────────────────────────────────────
    // Array flat: [x0,y0,z0, x1,y1,z1, ...]
    // Fase 4: alargar para [x,y,z, nx,ny,nz, u,v, ...] e ajustar stride em drawModel
    vector<float> flatVerts;
    flatVerts.reserve(modelData.vertices.size()*3);
    for(const Vertex& vtx:modelData.vertices){
        flatVerts.push_back(vtx.x);
        flatVerts.push_back(vtx.y);
        flatVerts.push_back(vtx.z);
    }
    glGenBuffers(1,&modelData.vboId);
    glBindBuffer(GL_ARRAY_BUFFER,modelData.vboId);
    glBufferData(GL_ARRAY_BUFFER,flatVerts.size()*sizeof(float),flatVerts.data(),GL_STATIC_DRAW);

    // ── Upload EBO (índices) ──────────────────────────────────────────────
    // Array de unsigned int: [v1,v2,v3, v1,v2,v3, ...]
    // Este buffer NÃO muda na Fase 4 — os índices são independentes dos dados
    vector<unsigned int> flatIdx;
    flatIdx.reserve(modelData.indexCount);
    for(const Face& f:modelData.faces){
        flatIdx.push_back((unsigned int)f.v1);
        flatIdx.push_back((unsigned int)f.v2);
        flatIdx.push_back((unsigned int)f.v3);
    }
    glGenBuffers(1,&modelData.eboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,modelData.eboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,flatIdx.size()*sizeof(unsigned int),flatIdx.data(),GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

    modelData.loaded=true;
    cout<<"VBO+EBO: "<<filename
        <<" ("<<modelData.vertices.size()<<" vértices únicos, "
        <<modelData.faces.size()<<" faces)"<<endl;
    return true;
}


// ─────────────────────────────────────────────────────────────────────────────
// Desenho com VBO + EBO
// ─────────────────────────────────────────────────────────────────────────────

void drawModel(const ModelData& md) {
    if(!md.loaded||md.vboId==0||md.eboId==0) return;

    glBindBuffer(GL_ARRAY_BUFFER,md.vboId);
    glEnableClientState(GL_VERTEX_ARRAY);
    // stride = 3 floats por vértice (Fase 4: mudar para sizeof(float)*8)
    glVertexPointer(3,GL_FLOAT,sizeof(float)*3,0);

    // Fase 4 — descomentar e ajustar offsets:
    // glEnableClientState(GL_NORMAL_ARRAY);
    // glNormalPointer(GL_FLOAT, sizeof(float)*8, (void*)(3*sizeof(float)));
    // glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    // glTexCoordPointer(2, GL_FLOAT, sizeof(float)*8, (void*)(6*sizeof(float)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,md.eboId);
    glDrawElements(GL_TRIANGLES,md.indexCount,GL_UNSIGNED_INT,0);

    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}


// ─────────────────────────────────────────────────────────────────────────────
// Transformações
// ─────────────────────────────────────────────────────────────────────────────

void applyTransformation(const Transformation& t) {
    switch(t.type){
        case TRANSLATE: glTranslatef(t.x,t.y,t.z); break;
        case SCALE:     glScalef(t.x,t.y,t.z); break;
        case ROTATE:    glRotatef(t.angle,t.x,t.y,t.z); break;
        case ROTATE_ANIM:{
            // Ângulo depende do tempo global da app; não acumula erro por frame.
            float elapsed=(float)glutGet(GLUT_ELAPSED_TIME)/1000.0f;
            glRotatef(fmod(elapsed/t.time*360.0f,360.0f),t.x,t.y,t.z);
            break;
        }
        case TRANSLATE_ANIM:{
            if(t.points.size()<4) break;
            float elapsed=(float)glutGet(GLUT_ELAPSED_TIME)/1000.0f;
            float pos[3],deriv[3];
            getCatmullRomPosition(fmod(elapsed/t.time,1.0f),t.points,pos,deriv);
            glTranslatef(pos[0],pos[1],pos[2]);
            // align=true orienta o objeto segundo a tangente da curva,
            // útil para cometas/naves seguirem a trajetória.
            if(t.align){ float m[16]; buildAlignMatrix(deriv,m); glMultMatrixf(m); }
            break;
        }
    }
}

void renderGroup(const Group& group){
    // Stack de matrizes garante isolamento entre ramos da árvore de cena.
    glPushMatrix();
    for(const Transformation& tr:group.transformations) applyTransformation(tr);
    for(const Model& model:group.models){
        auto it=modelDataMap.find(model.filename);
        if(it!=modelDataMap.end()) drawModel(it->second);
    }
    for(const Group& child:group.children) renderGroup(child);
    glPopMatrix();
}


// ─────────────────────────────────────────────────────────────────────────────
// Callbacks GLUT
// ─────────────────────────────────────────────────────────────────────────────

void drawAxes(){
    glBegin(GL_LINES);
    glColor3f(1,0,0);glVertex3f(-100,0,0);glVertex3f(100,0,0);
    glColor3f(0,1,0);glVertex3f(0,-100,0);glVertex3f(0,100,0);
    glColor3f(0,0,1);glVertex3f(0,0,-100);glVertex3f(0,0,100);
    glEnd();
}

void changeSize(int w,int h){
    if(h==0)h=1;
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glViewport(0,0,w,h);
    gluPerspective(camera->getFov(),(float)w/h,camera->getNearPlane(),camera->getFarPlane());
    glMatrixMode(GL_MODELVIEW);
}

void renderScene(){
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK,wireframeMode?GL_LINE:GL_FILL);
    glLoadIdentity(); camera->place();
    glColor3f(1,1,1);
    if(showAxes) drawAxes();
    renderGroup(sceneRootGroup);
    glutSwapBuffers();
    glutPostRedisplay();
}

void processKeys(unsigned char key,int xx,int yy){
    switch(key){
        case 'a':case 'A':showAxes=!showAxes;break;
        case 'l':case 'L':wireframeMode=!wireframeMode;break;
        case 'w':case 'W':camera->zoomIn();break;
        case 's':case 'S':camera->zoomOut();break;
        case 27:exit(0);
    }
    glutPostRedisplay();
}

void processSpecialKeys(int key,int xx,int yy){
    switch(key){
        case GLUT_KEY_UP:   camera->rotateUp();break;
        case GLUT_KEY_DOWN: camera->rotateDown();break;
        case GLUT_KEY_LEFT: camera->rotateLeft();break;
        case GLUT_KEY_RIGHT:camera->rotateRight();break;
    }
    glutPostRedisplay();
}


// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc,char** argv){
    if(argc<2){ cerr<<"Uso: "<<argv[0]<<" <config.xml>"<<endl; return 1; }

    camera=new Camera();
    if(!SimpleParser::parseXMLFile(argv[1],gWindow,*camera,sceneRootGroup)){
        cerr<<"Erro ao fazer parse do XML."<<endl; return 1;
    }

    // GLUT e janela OpenGL têm de existir ANTES de criar VBOs/EBOs
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DEPTH|GLUT_DOUBLE|GLUT_RGBA);
    glutInitWindowPosition(100,100);
    glutInitWindowSize(gWindow.width,gWindow.height);
    glutCreateWindow("Engine 3D - Fase 3");

    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);
    glutKeyboardFunc(processKeys);
    glutSpecialFunc(processSpecialKeys);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Carrega modelos e cria VBO+EBO (após glutCreateWindow)
    vector<string> modelRefs;
    collectModelReferences(sceneRootGroup,modelRefs);
    for(const string& ref:modelRefs){
        // Mesmo modelo em vários planetas/lua => 1 upload, vários draws.
        if(modelDataMap.count(ref)) continue;
        ModelData md;
        if(loadModel(md,resolveModelPath(ref,argv[1])))
            modelDataMap[ref]=md;
        else
            cerr<<"Aviso: falha ao carregar: "<<ref<<endl;
    }

    cout<<"\n=== Engine Fase 3 ===\n"
        <<"Setas: rodar camara | W/S: zoom | A: eixos | L: wireframe | ESC: sair\n";

    glutMainLoop();
    delete camera;
    return 0;
}