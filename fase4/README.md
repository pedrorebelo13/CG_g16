# Fase 4 - Engine 3D

Resumo e instruções para a Fase 4 do projeto de Computação Gráfica.

## Dependências
- OpenGL (GL, GLU)
- freeglut / GLUT
- DevIL (para carregamento de texturas)
- tinyxml2 (já incluído no repositório)

Instalação (exemplo Windows / MinGW): instalar as bibliotecas e garantir que os headers/libs estão no `PATH`/`LIB`.

## Compilação (exemplo com g++)
g++ -std=c++17 -I. fase4/engine/*.cpp fase4/generator/generator.cpp -lglut -lGL -lGLU -lIL -o engine

Ou usar CMake (recomendado) — criar um `CMakeLists.txt` que linke `GL`, `GLU`, `GLUT` e `IL`.

## Execução
1. Gerar ou preparar modelos 3D com o `generator` (ex.: `generator sphere 1 16 16 sphere.3d`).
2. Criar/editar um ficheiro XML de cena (ver `xmlFiles/` e `test files/` para exemplos).
3. Executar o engine:

```powershell
./engine fase4/xmlFiles/solar_system.xml
```

## Formato XML (resumo)
- `<world>`: elemento raiz
  - `<window width="W" height="H"/>`
  - `<camera>`: posição, lookAt, up, projection
  - `<lights>`: lista de `<light type="point|directional|spot" .../>`
  - `<group>`: hierarquia de cenas com `<transform>`, `<models>`, `<group>` (filhos)
  - `<models>`: `<model file="...">` pode ter `<texture file="..."/>` e `<color>` com `<diffuse>`, `<ambient>`, `<specular>`, `<emissive>`, `<shininess value="..."/>`

Ver `fase4/engine/parser.h` e `fase4/engine/parser.cpp` para detalhes de atributos e tratamento.

## Desenvolvimento / Notas técnicas
- O engine usa VBO/EBO com layout interleaved: `[x,y,z, nx,ny,nz, s,t]`.
- Os generators escrevem ficheiros XML de triângulos que o engine lê.
- Recursos (VBOs, EBOs, texturas) são agora libertados com `atexit` quando o programa termina.
- Para avançar: considerar migrar para pipeline moderno (shaders GLSL) para materiais mais flexíveis.

## Contacto
Para questões sobre o código, editar `fase4/README.md` e apontar issues no repositório.
