#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <math.h>
#include <GL/glut.h>

// DevIL: biblioteca de carregamento de imagens usada na UC de CG.


#include <IL/il.h>

#include "tinyxml2.h"
#include "camera.h"
#include "parser.h"

using namespace std;
using namespace tinyxml2;

// ─────────────────────────────────────────────────────────────────────────────
// Vertex — estendido da Fase 3 com normal e coordenadas de textura.
// O layout em memória é: [x,y,z, nx,ny,nz, s,t] = 8 floats = stride 32 bytes.
// Este layout intercalado (interleaved) é mais eficiente que arrays separados
// porque cada vértice cabe numa única linha de cache.
// ─────────────────────────────────────────────────────────────────────────────
struct Vertex {
    float x, y, z;      // posição
    float nx, ny, nz;   // normal (unitária)
    float s, t;         // coordenadas de textura [0,1]

    Vertex() : x(0),y(0),z(0),nx(0),ny(1),nz(0),s(0),t(0) {}
    Vertex(float _x,float _y,float _z,
           float _nx,float _ny,float _nz,
           float _s,float _t)
        : x(_x),y(_y),z(_z),nx(_nx),ny(_ny),nz(_nz),s(_s),t(_t) {}
};

// Face triangular por índices — inalterada da Fase 3
struct Face {
    int v1, v2, v3;
    Face() : v1(0),v2(0),v3(0) {}
    Face(int a,int b,int c) : v1(a),v2(b),v3(c) {}
};

// ModelData — agora tem também o ID da textura na GPU
struct ModelData {
    string   filename;
    vector<Vertex> vertices;
    vector<Face>   faces;
    GLuint vboId;
    GLuint eboId;
    GLuint textureId;   // 0 = sem textura
    int indexCount;
    bool loaded;

    ModelData() : vboId(0),eboId(0),textureId(0),indexCount(0),loaded(false) {}
};


// ─────────────────────────────────────────────────────────────────────────────
// Variáveis globais
// ─────────────────────────────────────────────────────────────────────────────
Window  gWindow;
Camera* camera;
Group   sceneRootGroup;
map<string, ModelData> modelDataMap;
map<string, GLuint>    textureCache;  // evita carregar a mesma imagem 2x

bool showAxes     = false;
bool wireframeMode= false;


// ─────────────────────────────────────────────────────────────────────────────
// Catmull-Rom (inalterado da Fase 3)
// ─────────────────────────────────────────────────────────────────────────────
static const float CR[4][4] = {
    {-0.5f, 1.5f,-1.5f, 0.5f},
    { 1.0f,-2.5f, 2.0f,-0.5f},
    {-0.5f, 0.0f, 0.5f, 0.0f},
    { 0.0f, 1.0f, 0.0f, 0.0f}
};

void catmullRomPoint(float t,
                     const Point3D& p0,const Point3D& p1,
                     const Point3D& p2,const Point3D& p3,
                     float* pos,float* deriv){
    float T[4]={t*t*t,t*t,t,1};
    float dT[4]={3*t*t,2*t,1,0};
    float px[4]={p0.x,p1.x,p2.x,p3.x};
    float py[4]={p0.y,p1.y,p2.y,p3.y};
    float pz[4]={p0.z,p1.z,p2.z,p3.z};
    for(int a=0;a<3;a++){
        float* P=(a==0)?px:(a==1)?py:pz;
        float tmp[4]={0,0,0,0};
        for(int i=0;i<4;i++) for(int j=0;j<4;j++) tmp[i]+=CR[i][j]*P[j];
        pos[a]=0;deriv[a]=0;
        for(int i=0;i<4;i++){pos[a]+=T[i]*tmp[i];deriv[a]+=dT[i]*tmp[i];}
    }
}

void getCatmullRomPosition(float gt,const vector<Point3D>& pts,float* pos,float* deriv){
    int ns=(int)pts.size()-3;
    float ts=gt*ns; int seg=(int)ts; if(seg>=ns)seg=ns-1;
    catmullRomPoint(ts-seg,pts[seg],pts[seg+1],pts[seg+2],pts[seg+3],pos,deriv);
}

void buildAlignMatrix(float* d,float* m){
    float len=sqrt(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]);
    if(len<1e-6f)len=1;
    float X[3]={d[0]/len,d[1]/len,d[2]/len};
    float Z[3]={0,1,0};
    float dot=X[0]*Z[0]+X[1]*Z[1]+X[2]*Z[2];
    if(fabs(dot)>0.99f){Z[0]=0;Z[1]=0;Z[2]=1;}
    float Y[3]={Z[1]*X[2]-Z[2]*X[1],Z[2]*X[0]-Z[0]*X[2],Z[0]*X[1]-Z[1]*X[0]};
    len=sqrt(Y[0]*Y[0]+Y[1]*Y[1]+Y[2]*Y[2]);if(len<1e-6f)len=1;
    Y[0]/=len;Y[1]/=len;Y[2]/=len;
    float Zv[3]={X[1]*Y[2]-X[2]*Y[1],X[2]*Y[0]-X[0]*Y[2],X[0]*Y[1]-X[1]*Y[0]};
    m[0]=X[0];m[1]=X[1];m[2]=X[2];m[3]=0;
    m[4]=Y[0];m[5]=Y[1];m[6]=Y[2];m[7]=0;
    m[8]=Zv[0];m[9]=Zv[1];m[10]=Zv[2];m[11]=0;
    m[12]=0;m[13]=0;m[14]=0;m[15]=1;
}


// ─────────────────────────────────────────────────────────────────────────────
// Caminhos e referências
// ─────────────────────────────────────────────────────────────────────────────
bool fileExists(const string& p){ifstream f(p);return f.good();}

string resolveModelPath(const string& mf,const string& cfg){
    if(fileExists(mf))return mf;
    size_t pos=cfg.find_last_of("/\\");
    if(pos!=string::npos){string p=cfg.substr(0,pos)+"/"+mf;if(fileExists(p))return p;}
    string g1="../generator/files3d/"+mf; if(fileExists(g1))return g1;
    string g2="generator/files3d/"+mf;   if(fileExists(g2))return g2;
    return mf;
}

string resolveTexturePath(const string& tf,const string& cfg){
    if(fileExists(tf))return tf;
    size_t pos=cfg.find_last_of("/\\");
    if(pos!=string::npos){string p=cfg.substr(0,pos)+"/"+tf;if(fileExists(p))return p;}
    string t1="textures/"+tf; if(fileExists(t1))return t1;
    return tf;
}

void collectModelRefs(const Group& g,vector<pair<string,string>>& refs){
    for(const Model& m:g.models) refs.push_back({m.filename,m.textureFile});
    for(const Group& c:g.children) collectModelRefs(c,refs);
}


// ─────────────────────────────────────────────────────────────────────────────
// Carregamento de textura com stb_image
// Retorna o ID OpenGL da textura criada, ou 0 se falhar.
// A textura é carregada como RGBA e enviada para a GPU com glTexImage2D.
// GL_REPEAT: a textura repete se s/t saírem de [0,1].
// GL_LINEAR: interpolação bilinear — evita pixelização em close-up.
// ─────────────────────────────────────────────────────────────────────────────
GLuint loadTexture(const string& filename){
    if(filename.empty()) return 0;
    auto it=textureCache.find(filename);
    if(it!=textureCache.end()) return it->second;

    // Carregamento com DevIL — abordagem usada na UC de CG (guião PL11)
    unsigned int ilImg;
    unsigned int tw, th;
    unsigned char* texData;
    GLuint texID;

    // Iniciar o DevIL (idempotente — pode ser chamado várias vezes)
    ilInit();

    // Colocar a origem da textura no canto inferior esquerdo,
    // para que (0,0) corresponda ao canto inferior esquerdo da imagem
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

    // Carregar a imagem para memória com DevIL
    ilGenImages(1, &ilImg);
    ilBindImage(ilImg);
    ilLoadImage((ILstring)filename.c_str());

    tw = ilGetInteger(IL_IMAGE_WIDTH);
    th = ilGetInteger(IL_IMAGE_HEIGHT);

    if(tw == 0 || th == 0){
        cerr<<"Erro ao carregar textura: "<<filename<<endl;
        ilDeleteImages(1, &ilImg);
        return 0;
    }

    // Converter para RGBA com 1 byte por componente (0-255)
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
    texData = ilGetData();

    // Criar a textura OpenGL e enviar os dados para a GPU
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Upload dos dados de imagem para a GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, texData);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Libertar a imagem da memória DevIL
    ilDeleteImages(1, &ilImg);

    textureCache[filename] = texID;
    cout<<"Textura carregada: "<<filename<<" ("<<tw<<"x"<<th<<")"<<endl;
    return texID;
}


// ─────────────────────────────────────────────────────────────────────────────
// Carregamento de modelo com VBO+EBO
// Fase 4: o Vertex agora tem 8 floats (pos + normal + textura).
// O engine lê os atributos nx,ny,nz,s,t diretamente do XML gerado.
// ─────────────────────────────────────────────────────────────────────────────
bool loadModel(ModelData& md,const string& filename,const string& textureFile,const string& cfg){
    ifstream file(filename);
    if(!file.is_open()){cerr<<"Erro ao abrir: "<<filename<<endl;return false;}
    md.filename=filename;
    md.vertices.clear();
    md.faces.clear();

    string content((istreambuf_iterator<char>(file)),istreambuf_iterator<char>());
    file.close();

    XMLDocument doc;
    if(doc.Parse(content.c_str())!=XML_SUCCESS){cerr<<"Erro XML: "<<filename<<endl;return false;}
    XMLElement* root=doc.RootElement();
    if(!root)return false;

    // Chave de deduplicação agora inclui posição E normal E textura,
    // porque um mesmo ponto no espaço pode ter normais diferentes
    // (ex: aresta de uma caixa — cada face tem normal diferente).
    map<string,int> vertexIndex;
    int nextIdx=0;

    XMLElement* tri=root->FirstChildElement("triangle");
    while(tri){
        XMLElement* v=tri->FirstChildElement("vertex");
        vector<int> idxs;
        while(v&&(int)idxs.size()<3){
            float x=0,y=0,z=0,nx=0,ny=1,nz=0,s=0,t=0;
            v->QueryFloatAttribute("x",&x);
            v->QueryFloatAttribute("y",&y);
            v->QueryFloatAttribute("z",&z);
            v->QueryFloatAttribute("nx",&nx);
            v->QueryFloatAttribute("ny",&ny);
            v->QueryFloatAttribute("nz",&nz);
            v->QueryFloatAttribute("s",&s);
            v->QueryFloatAttribute("t",&t);

            // Chave única por combinação de todos os 8 atributos
            string key=to_string(x)+","+to_string(y)+","+to_string(z)
                      +","+to_string(nx)+","+to_string(ny)+","+to_string(nz)
                      +","+to_string(s)+","+to_string(t);

            if(vertexIndex.find(key)==vertexIndex.end()){
                md.vertices.push_back(Vertex(x,y,z,nx,ny,nz,s,t));
                vertexIndex[key]=nextIdx++;
            }
            idxs.push_back(vertexIndex[key]);
            v=v->NextSiblingElement("vertex");
        }
        if((int)idxs.size()==3)
            md.faces.push_back(Face(idxs[0],idxs[1],idxs[2]));
        tri=tri->NextSiblingElement("triangle");
    }

    md.indexCount=(int)md.faces.size()*3;

    // ── Upload VBO — stride 8 floats = 32 bytes ────────────────────────────
    // Layout: [x,y,z, nx,ny,nz, s,t] intercalado para eficiência de cache.
    vector<float> flat;
    flat.reserve(md.vertices.size()*8);
    for(const Vertex& vtx:md.vertices){
        flat.push_back(vtx.x);  flat.push_back(vtx.y);  flat.push_back(vtx.z);
        flat.push_back(vtx.nx); flat.push_back(vtx.ny); flat.push_back(vtx.nz);
        flat.push_back(vtx.s);  flat.push_back(vtx.t);
    }
    glGenBuffers(1,&md.vboId);
    glBindBuffer(GL_ARRAY_BUFFER,md.vboId);
    glBufferData(GL_ARRAY_BUFFER,flat.size()*sizeof(float),flat.data(),GL_STATIC_DRAW);

    // ── Upload EBO — inalterado da Fase 3 ─────────────────────────────────
    vector<unsigned int> idx;
    idx.reserve(md.indexCount);
    for(const Face& f:md.faces){
        idx.push_back(f.v1);idx.push_back(f.v2);idx.push_back(f.v3);
    }
    glGenBuffers(1,&md.eboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,md.eboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,idx.size()*sizeof(unsigned int),idx.data(),GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

    // Carrega textura se especificada
    if(!textureFile.empty())
        md.textureId=loadTexture(resolveTexturePath(textureFile,cfg));

    md.loaded=true;
    cout<<"VBO+EBO: "<<filename
        <<" ("<<md.vertices.size()<<" vértices, "<<md.faces.size()<<" faces)"<<endl;
    return true;
}


// ─────────────────────────────────────────────────────────────────────────────
// Aplicação de luzes OpenGL
// Cada luz ocupa um slot GL_LIGHT0..GL_LIGHT7 (máximo 8 luzes simultâneas).
// As luzes têm de ser posicionadas APÓS o gluLookAt para ficarem no espaço
// correto (world space), por isso são aplicadas no início de cada frame.
// ─────────────────────────────────────────────────────────────────────────────
void applyLights(const vector<Light>& lights){
    // Luz ambiente global da cena (fraca, ilumina tudo uniformemente)
    GLfloat globalAmb[]={0.1f,0.1f,0.1f,1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,globalAmb);

    for(int i=0;i<(int)lights.size()&&i<8;i++){
        GLenum lightId=GL_LIGHT0+i;
        const Light& l=lights[i];

        glEnable(lightId);

        // Cor da luz: branca por padrão
        GLfloat white[]={1,1,1,1};
        GLfloat dim[]  ={0.3f,0.3f,0.3f,1};
        glLightfv(lightId,GL_DIFFUSE, white);
        glLightfv(lightId,GL_SPECULAR,white);
        glLightfv(lightId,GL_AMBIENT, dim);

        if(l.type==LIGHT_DIRECTIONAL){
            // W=0: luz direcional (infinitamente distante)
            GLfloat pos[]={l.dirX,l.dirY,l.dirZ,0.0f};
            glLightfv(lightId,GL_POSITION,pos);

        } else if(l.type==LIGHT_POINT){
            // W=1: luz pontual com posição
            GLfloat pos[]={l.posX,l.posY,l.posZ,1.0f};
            glLightfv(lightId,GL_POSITION,pos);

        } else { // LIGHT_SPOT
            GLfloat pos[]={l.posX,l.posY,l.posZ,1.0f};
            GLfloat dir[]={l.dirX,l.dirY,l.dirZ};
            glLightfv(lightId,GL_POSITION,pos);
            glLightfv(lightId,GL_SPOT_DIRECTION,dir);
            glLightf (lightId,GL_SPOT_CUTOFF,l.cutoff);
            glLightf (lightId,GL_SPOT_EXPONENT,2.0f); // concentração
        }
    }
}


// ─────────────────────────────────────────────────────────────────────────────
// Aplicação de material e textura antes de desenhar um modelo
// ─────────────────────────────────────────────────────────────────────────────
void applyMaterial(const Material& mat){
    glMaterialfv(GL_FRONT,GL_DIFFUSE,  mat.diffuse);
    glMaterialfv(GL_FRONT,GL_AMBIENT,  mat.ambient);
    glMaterialfv(GL_FRONT,GL_SPECULAR, mat.specular);
    glMaterialfv(GL_FRONT,GL_EMISSION, mat.emissive);
    glMaterialf (GL_FRONT,GL_SHININESS,mat.shininess);
}


// ─────────────────────────────────────────────────────────────────────────────
// Desenho com VBO + EBO + normais + textura
// ─────────────────────────────────────────────────────────────────────────────
void drawModel(const ModelData& md, const Model& modelInfo){
    if(!md.loaded||md.vboId==0||md.eboId==0) return;

    // Aplica material de cor do modelo
    applyMaterial(modelInfo.material);

    // Ativa textura se disponível
    bool hasTexture = (md.textureId != 0);
    if(hasTexture){
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D,md.textureId);
    }

    // Liga VBO e define os 3 pointers com o mesmo stride de 8 floats
    glBindBuffer(GL_ARRAY_BUFFER,md.vboId);

    glEnableClientState(GL_VERTEX_ARRAY);
    // Posição: offset 0, stride 8 floats
    glVertexPointer(3,GL_FLOAT,sizeof(float)*8,(void*)0);

    glEnableClientState(GL_NORMAL_ARRAY);
    // Normal: offset 3 floats após o início
    glNormalPointer(GL_FLOAT,sizeof(float)*8,(void*)(3*sizeof(float)));

    if(hasTexture){
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        // Textura: offset 6 floats após o início
        glTexCoordPointer(2,GL_FLOAT,sizeof(float)*8,(void*)(6*sizeof(float)));
    }

    // Desenha usando índices do EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,md.eboId);
    glDrawElements(GL_TRIANGLES,md.indexCount,GL_UNSIGNED_INT,0);

    // Desativa tudo
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    if(hasTexture){
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D,0);
    }
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}


// ─────────────────────────────────────────────────────────────────────────────
// Transformações (inalteradas da Fase 3)
// ─────────────────────────────────────────────────────────────────────────────
void applyTransformation(const Transformation& t){
    switch(t.type){
        case TRANSLATE: glTranslatef(t.x,t.y,t.z); break;
        case SCALE:     glScalef(t.x,t.y,t.z); break;
        case ROTATE:    glRotatef(t.angle,t.x,t.y,t.z); break;
        case ROTATE_ANIM:{
            float e=(float)glutGet(GLUT_ELAPSED_TIME)/1000.0f;
            glRotatef(fmod(e/t.time*360.0f,360.0f),t.x,t.y,t.z);
            break;
        }
        case TRANSLATE_ANIM:{
            if(t.points.size()<4)break;
            float e=(float)glutGet(GLUT_ELAPSED_TIME)/1000.0f;
            float pos[3],deriv[3];
            getCatmullRomPosition(fmod(e/t.time,1.0f),t.points,pos,deriv);
            glTranslatef(pos[0],pos[1],pos[2]);
            if(t.align){float m[16];buildAlignMatrix(deriv,m);glMultMatrixf(m);}
            break;
        }
    }
}


// ─────────────────────────────────────────────────────────────────────────────
// Renderização hierárquica
// ─────────────────────────────────────────────────────────────────────────────
void renderGroup(const Group& group){
    glPushMatrix();
    for(const Transformation& tr:group.transformations) applyTransformation(tr);
    for(const Model& model:group.models){
        auto it=modelDataMap.find(model.filename);
        if(it!=modelDataMap.end())
            drawModel(it->second, model);
    }
    for(const Group& child:group.children) renderGroup(child);
    glPopMatrix();
}


// ─────────────────────────────────────────────────────────────────────────────
// Callbacks GLUT
// ─────────────────────────────────────────────────────────────────────────────
void drawAxes(){
    // Desativa iluminação para os eixos não ficarem afetados por ela
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
    glColor3f(1,0,0);glVertex3f(-100,0,0);glVertex3f(100,0,0);
    glColor3f(0,1,0);glVertex3f(0,-100,0);glVertex3f(0,100,0);
    glColor3f(0,0,1);glVertex3f(0,0,-100);glVertex3f(0,0,100);
    glEnd();
    glEnable(GL_LIGHTING);
}

void changeSize(int w,int h){
    if(h==0)h=1;
    glMatrixMode(GL_PROJECTION);glLoadIdentity();
    glViewport(0,0,w,h);
    gluPerspective(camera->getFov(),(float)w/h,camera->getNearPlane(),camera->getFarPlane());
    glMatrixMode(GL_MODELVIEW);
}

void renderScene(){
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK,wireframeMode?GL_LINE:GL_FILL);
    glLoadIdentity();
    camera->place();

    // Aplica luzes APÓS gluLookAt para ficarem no espaço do mundo
    applyLights(sceneRootGroup.lights);

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
    if(argc<2){cerr<<"Uso: "<<argv[0]<<" <config.xml>"<<endl;return 1;}

    camera=new Camera();
    if(!SimpleParser::parseXMLFile(argv[1],gWindow,*camera,sceneRootGroup)){
        cerr<<"Erro ao fazer parse do XML."<<endl;return 1;
    }

    // GLUT e janela têm de existir antes de criar VBOs/texturas
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DEPTH|GLUT_DOUBLE|GLUT_RGBA);
    glutInitWindowPosition(100,100);
    glutInitWindowSize(gWindow.width,gWindow.height);
    glutCreateWindow("Engine 3D - Fase 4");

    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);
    glutKeyboardFunc(processKeys);
    glutSpecialFunc(processSpecialKeys);

    // Inicializar DevIL para carregamento de texturas
    ilInit();
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Ativa iluminação global
    glEnable(GL_LIGHTING);
    // Permite que glColor funcione com iluminação (para os eixos)
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
    // Normaliza normais automaticamente (necessário quando scale != 1)
    glEnable(GL_NORMALIZE);

    // Carrega modelos e texturas após glutCreateWindow
    vector<pair<string,string>> modelRefs;
    collectModelRefs(sceneRootGroup,modelRefs);

    for(auto& [modelFile,texFile]:modelRefs){
        if(modelDataMap.count(modelFile)) continue;
        ModelData md;
        string mpath=resolveModelPath(modelFile,argv[1]);
        if(loadModel(md,mpath,texFile,argv[1]))
            modelDataMap[modelFile]=md;
        else
            cerr<<"Aviso: falha ao carregar: "<<modelFile<<endl;
    }

    cout<<"\n=== Engine Fase 4 ===\n"
        <<"Setas: rodar camara | W/S: zoom | A: eixos | L: wireframe | ESC: sair\n";

    glutMainLoop();
    delete camera;
    return 0;
}