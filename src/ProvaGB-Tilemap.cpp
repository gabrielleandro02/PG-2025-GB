/*
 * Jogo Isométrico - Sistema Simplificado
 * Implementa apenas:
 * - Terra (caminhável) = tile 0
 * - Lava (letal) = tile 1  
 * - Coin.png como coletável nos items
 */

 #include <iostream>
 #include <string>
 #include <assert.h>
 #include <cmath>
 #include <fstream>
 #include <sstream>
 #include <vector>
 
 using namespace std;
 
 // GLAD
 #include <glad/glad.h>
 
 // GLFW
 #include <GLFW/glfw3.h>
 
 // STB_IMAGE
 #define STB_IMAGE_IMPLEMENTATION
 #include <stb_image.h>
 
 //GLM
 #include <glm/glm.hpp> 
 #include <glm/gtc/matrix_transform.hpp>
 #include <glm/gtc/type_ptr.hpp>
 
 using namespace glm;
 
 struct Sprite
 {
     GLuint VAO;
     GLuint texID;
     vec3 position;
     vec3 dimensions;
     float ds, dt;
     int iAnimation, iFrame;
     int nAnimations, nFrames;
 };
 
 struct Tile
 {
     GLuint VAO;
     GLuint texID;
     int iTile;
     vec3 position;
     vec3 dimensions;
     float ds, dt;
     bool caminhavel;
     bool letal;
 };
 
 struct MapData
 {
     string tilesetPath;
     int numTiles;
     int tileWidth, tileHeight;
     int mapWidth, mapHeight;
     vector<vector<int>> tiles;
     vector<vector<int>> items; // 0=vazio, 1=moeda
 };
 
 // Protótipos das funções
 void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
 int setupShader();
 int setupSprite(int nAnimations, int nFrames, float &ds, float &dt);
 int setupTile(int nTiles, float &ds, float &dt);
 int setupCoin(float &ds, float &dt);
 int loadTexture(string filePath, int &width, int &height);
 void desenharMapa(GLuint shaderID);
 void desenharPersonagem(GLuint shaderID);
 bool carregarMapa(const string& filepath, MapData& mapData);
 void processarColisoes();
 
 // Dimensões da janela
 const GLuint WIDTH = 1024, HEIGHT = 768;
 
 // Variáveis globais
 vector<Tile> tileset;
 MapData mapa;
 vec2 pos;
 Sprite personagem;
 Sprite moeda;
 int moedasColetadas = 0;
 int moedasTotal = 0;
 bool jogoGanho = false;
 bool jogoPerdido = false;
 double lastTime = 0.0;
 double currTime = 0.0;
 double FPS = 8.0;
 
 // Shaders
 const GLchar *vertexShaderSource = R"(
  #version 400
  layout (location = 0) in vec3 position;
  layout (location = 1) in vec2 texc;
  out vec2 tex_coord;
  uniform mat4 model;
  uniform mat4 projection;
  void main()
  {
     tex_coord = vec2(texc.s, 1.0 - texc.t);
     gl_Position = projection * model * vec4(position, 1.0);
  }
  )";
 
 const GLchar *fragmentShaderSource = R"(
  #version 400
  in vec2 tex_coord;
  out vec4 color;
  uniform sampler2D tex_buff;
  uniform vec2 offsetTex;
  uniform vec3 colorTint;
 
  void main()
  {
      vec4 texColor = texture(tex_buff, tex_coord + offsetTex);
      color = vec4(texColor.rgb * colorTint, texColor.a);
  }
  )";
 
 int main()
 {
     // Inicialização da GLFW
     glfwInit();
     glfwWindowHint(GLFW_SAMPLES, 8);
 
     GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Jogo Isometrico - Colete todas as moedas!", nullptr, nullptr);
     if (!window)
     {
         std::cerr << "Falha ao criar a janela GLFW" << std::endl;
         glfwTerminate();
         return -1;
     }
     glfwMakeContextCurrent(window);
     glfwSetKeyCallback(window, key_callback);
 
     if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
     {
         std::cerr << "Falha ao inicializar GLAD" << std::endl;
         return -1;
     }
 
     const GLubyte *renderer = glGetString(GL_RENDERER);
     const GLubyte *version = glGetString(GL_VERSION);
     cout << "Renderer: " << renderer << endl;
     cout << "OpenGL version supported " << version << endl;
 
     int width, height;
     glfwGetFramebufferSize(window, &width, &height);
     glViewport(0, 0, width, height);
 
     GLuint shaderID = setupShader();
 
     // Carregar mapa do arquivo
     if (!carregarMapa("assets/maps/map.txt", mapa))
     {
         std::cerr << "Erro ao carregar o mapa!" << std::endl;
         return -1;
     }
 
     // Configurar tileset com apenas 2 tipos: terra(0) e lava(1)
     int imgWidth, imgHeight;
     GLuint texID = loadTexture(mapa.tilesetPath, imgWidth, imgHeight);
 
     int tileIndices[2] = {2, 3}; // 2 = terra, 4 = lava
     for (int i = 0; i < 2; i++)
     {
         Tile tile;
         tile.dimensions = vec3(mapa.tileWidth, mapa.tileHeight, 1.0);
         tile.iTile = tileIndices[i];
         tile.texID = texID;
         tile.VAO = setupTile(7, tile.ds, tile.dt); // Apenas 2 tiles
         
         if (i == 0) // Terra
         {
             tile.caminhavel = true;
             tile.letal = false;
         }
         else if (i == 1) // Lava
         {
             tile.caminhavel = true; // Pode pisar, mas é letal
             tile.letal = true;
         }
 
         tileset.push_back(tile);
     }
 
     // Configurar moeda (coin.png)
     GLuint moedaTexID = loadTexture("assets/sprites/coin.png", imgWidth, imgHeight);
     moeda.VAO = setupCoin(moeda.ds, moeda.dt);
     moeda.position = vec3(0, 0, 0);
     moeda.dimensions = vec3(32, 32, 1.0);
     moeda.texID = moedaTexID;
     moeda.iAnimation = 0;
     moeda.iFrame = 0;
 
     // Configurar personagem animado
     GLuint personagemTexID = loadTexture("assets/sprites/Vampires1_Walk_full.png", imgWidth, imgHeight);
     personagem.nAnimations = 4;
     personagem.nFrames = 6;
     personagem.VAO = setupSprite(personagem.nAnimations, personagem.nFrames, personagem.ds, personagem.dt);
     personagem.position = vec3(0, 0, 0);
     personagem.dimensions = vec3(imgWidth/personagem.nFrames*2, imgHeight/personagem.nAnimations*2, 1.0);
     personagem.texID = personagemTexID;
     personagem.iAnimation = 1;
     personagem.iFrame = 0;
 
     // Encontrar posição inicial segura (apenas em terra)
     pos.x = mapa.mapHeight / 2;
     pos.y = mapa.mapWidth / 2;
 
     bool posicaoValida = false;
     for (int tentativas = 0; tentativas < 100 && !posicaoValida; tentativas++)
     {
         int tileType = mapa.tiles[(int)pos.x][(int)pos.y];
         if (tileType == 0) // Apenas terra é segura
         {
             posicaoValida = true;
         }
         else
         {
             pos.x = (static_cast<int>(pos.x) + 1) % mapa.mapHeight;
             pos.y = (static_cast<int>(pos.y) + 1) % mapa.mapWidth;
         }
     }
     
     cout << "Posicao inicial do personagem: (" << pos.x << ", " << pos.y << ")" << endl;
 
     // Contar moedas totais
     for (int i = 0; i < mapa.mapHeight; i++)
     {
         for (int j = 0; j < mapa.mapWidth; j++)
         {
             if (mapa.items[i][j] == 1) // moeda
                 moedasTotal++;
         }
     }
 
     cout << "Total de moedas no mapa: " << moedasTotal << endl;
 
     glUseProgram(shaderID);
 
     double prev_s = glfwGetTime();
     double title_countdown_s = 0.1;
 
     glActiveTexture(GL_TEXTURE0);
     glUniform1i(glGetUniformLocation(shaderID, "tex_buff"), 0);
 
     mat4 projection = ortho(0.0, (double)WIDTH, (double)HEIGHT, 0.0, -1.0, 1.0);
     glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));
 
     glEnable(GL_DEPTH_TEST);
     glDepthFunc(GL_ALWAYS);
     glEnable(GL_BLEND);
     glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 
     // Game loop
     while (!glfwWindowShouldClose(window))
     {
         // FPS calculation
         {
             double curr_s = glfwGetTime();
             double elapsed_s = curr_s - prev_s;
             prev_s = curr_s;
 
             title_countdown_s -= elapsed_s;
             if (title_countdown_s <= 0.0 && elapsed_s > 0.0)
             {
                 double fps = 1.0 / elapsed_s;
                 char tmp[512];
                 sprintf(tmp, "Jogo Isometrico - Moedas: %d/%d - FPS %.2lf %s", 
                         moedasColetadas, moedasTotal, fps,
                         jogoGanho ? "- VOCE GANHOU!" : (jogoPerdido ? "- GAME OVER!" : ""));
                 glfwSetWindowTitle(window, tmp);
                 title_countdown_s = 0.1;
             }
         }
 
         glfwPollEvents();
 
         glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
         if (!jogoGanho && !jogoPerdido)
         {
             processarColisoes();
         }
 
         desenharMapa(shaderID);
         desenharPersonagem(shaderID);
 
         glfwSwapBuffers(window);
     }
 
     glfwTerminate();
     return 0;
 }
 
 void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
 {
     if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
         glfwSetWindowShouldClose(window, GL_TRUE);
 
     if (jogoGanho || jogoPerdido) return;
 
     vec2 novaPos = pos;
 
     if (action == GLFW_PRESS)
     {
         switch(key)
         {
             case GLFW_KEY_W: // NORTE
                 if (pos.x > 0)
                 {
                     novaPos.x--;
                 }
                 break;
             case GLFW_KEY_A: // OESTE
                 if (pos.y > 0)
                 {
                     novaPos.y--;
                 }
                 break;
             case GLFW_KEY_S: // SUL
                 if (pos.x < mapa.mapHeight - 1)
                 {
                     novaPos.x++;
                 }
                 break;
             case GLFW_KEY_D: // LESTE
                 if (pos.y < mapa.mapWidth - 1)
                 {
                     novaPos.y++;
                 }
                 break;
             case GLFW_KEY_Q: // NOROESTE
                 if (pos.x > 0 && pos.y > 0)
                 {
                     novaPos.x--;
                     novaPos.y--;
                 }
                 break;
             case GLFW_KEY_E: // NORDESTE
                 if (pos.x > 0 && pos.y < mapa.mapWidth - 1)
                 {
                     novaPos.x--;
                     novaPos.y++;
                 }
                 break;
             case GLFW_KEY_Z: // SUDOESTE
                 if (pos.x < mapa.mapHeight - 1 && pos.y > 0)
                 {
                     novaPos.x++;
                     novaPos.y--;
                 }
                 break;
             case GLFW_KEY_X: // SUDESTE
                 if (pos.x < mapa.mapHeight - 1 && pos.y < mapa.mapWidth - 1)
                 {
                     novaPos.x++;
                     novaPos.y++;
                 }
                 break;
         }
 
         // Verificar se pode mover - simplificado para apenas 2 tipos de tile
         if (novaPos.x >= 0 && novaPos.x < mapa.mapHeight && 
             novaPos.y >= 0 && novaPos.y < mapa.mapWidth)
         {
             int tileType = mapa.tiles[(int)novaPos.x][(int)novaPos.y];
             
             // Garantir que apenas tiles 0 e 1 são válidos
            if (tileType == 0 || tileType == 1) {
                pos = novaPos;
            } else {
                cout << "Movimento bloqueado - tile inválido: " << tileType << endl;
            }
         }
         else
         {
             cout << "Movimento bloqueado - fora dos limites" << endl;
         }
     }

     cout << "Posicao do personagem: (" << pos.x << ", " << pos.y << ")" << endl;
cout << "Tile na posicao: " << mapa.tiles[(int)pos.x][(int)pos.y] << endl;
 }
 
 void processarColisoes()
 {
     int x = (int)pos.x;
     int y = (int)pos.y;
 
     // Verificar se coletou moeda
     if (mapa.items[x][y] == 1) // Moeda
     {
         mapa.items[x][y] = 0; // Remove a moeda
         moedasColetadas++;
         cout << "Moeda coletada! Total: " << moedasColetadas << "/" << moedasTotal << endl;
         
         if (moedasColetadas >= moedasTotal)
         {
             jogoGanho = true;
             cout << "PARABENS! Voce coletou todas as moedas!" << endl;
         }
     }
 
     // Verificar se pisou em lava
     int tileType = mapa.tiles[x][y];
     if (tileType == 1) // Lava
     {
         jogoPerdido = true;
         cout << "GAME OVER! Voce pisou na lava!" << endl;
     }
 }
 
 void desenharMapa(GLuint shaderID)
 {
     float x0 = WIDTH / 2.0f;
     float y0 = 150.0f;
 
     glUniform3f(glGetUniformLocation(shaderID, "colorTint"), 1.0f, 1.0f, 1.0f);
 
     for (int i = 0; i < mapa.mapHeight; i++)
     {
         for (int j = 0; j < mapa.mapWidth; j++)
         {
             int tileIndex = mapa.tiles[i][j];

             if (tileIndex < 0 || tileIndex > 1) {
                continue; // não desenha tiles inválidos
            }
             
             // Garantir que só usamos tiles válidos (0=terra, 1=lava)
             if (tileIndex < 0 || tileIndex > 1) {
                 tileIndex = 0; // Default para terra se inválido
             }
             
             Tile curr_tile = tileset[tileIndex];
 
             // Fórmula isométrica
             float x = x0 + (j - i) * curr_tile.dimensions.x / 2.0f;
             float y = y0 + (i + j) * curr_tile.dimensions.y / 2.0f;
 
             mat4 model = mat4(1);
             model = translate(model, vec3(x, y, 0.0));
             model = scale(model, curr_tile.dimensions);
             glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
 
             vec2 offsetTex;
             offsetTex.s = curr_tile.iTile * curr_tile.ds;
             offsetTex.t = 0.0;
             glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t);
 
             glBindVertexArray(curr_tile.VAO);
             glBindTexture(GL_TEXTURE_2D, curr_tile.texID);
             glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
 
             // Desenhar moeda se houver (item = 1)
             if (mapa.items[i][j] == 1)
             {
                 // Elevar moeda um pouco acima do tile
                 model = mat4(1);
                 model = translate(model, vec3(x, y, 0.0));
                 model = scale(model, moeda.dimensions);
                 glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
 
                 offsetTex.s = 0.0;
                 offsetTex.t = 0.0;
                 glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t);
 
                 glBindVertexArray(moeda.VAO);
                 glBindTexture(GL_TEXTURE_2D, moeda.texID);
                 glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
             }
         }
     }
 }
 
 void desenharPersonagem(GLuint shaderID)
 {
     float x0 = WIDTH / 2.0f;
     float y0 = 150.0f;
 
     // CORREÇÃO: Usar o tile atual onde o personagem está, não sempre o base_tile
     int tileAtualIndex = mapa.tiles[(int)pos.x][(int)pos.y];
     
     // Garantir que o índice é válido (0=terra, 1=lava)
     if (tileAtualIndex < 0 || tileAtualIndex > 1) {
         tileAtualIndex = 0; // Default para terra se inválido
     }
     
     Tile tile_atual = tileset[tileAtualIndex];
     
     // Usar as dimensões do tile atual para o cálculo de posição
     float x = x0 + (pos.y - pos.x) * tile_atual.dimensions.x / 2.0f +
                tile_atual.dimensions.x / 2.0f;
     float y = y0 + (pos.x + pos.y) * tile_atual.dimensions.y / 2.0f ;
 
     currTime = glfwGetTime();
     double deltaT = currTime - lastTime;
 
     if (deltaT >= 1.0 / FPS)
     {
         personagem.iFrame = (personagem.iFrame + 1) % personagem.nFrames;
         lastTime = currTime;
     }
 
     mat4 model = mat4(1);
     model = translate(model, vec3(x, y, 0.0));
     model = scale(model, personagem.dimensions);
     glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
 
     vec2 offsetTex;
     offsetTex.s = personagem.iFrame * personagem.ds;
     offsetTex.t = personagem.iAnimation * personagem.dt;
     glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t);
 
     if (jogoPerdido)
         glUniform3f(glGetUniformLocation(shaderID, "colorTint"), 1.0f, 0.5f, 0.5f);
     else if (jogoGanho)
         glUniform3f(glGetUniformLocation(shaderID, "colorTint"), 0.5f, 1.0f, 0.5f);
     else
         glUniform3f(glGetUniformLocation(shaderID, "colorTint"), 1.0f, 1.0f, 1.0f);
 
     glBindVertexArray(personagem.VAO);
     glBindTexture(GL_TEXTURE_2D, personagem.texID);
     glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
 }
 
 bool carregarMapa(const string& filepath, MapData& mapData)
 {
     ifstream file(filepath);
     if (!file.is_open())
     {
         cerr << "Erro ao abrir arquivo: " << filepath << endl;
         return false;
     }
 
     string line;
     getline(file, line);
     mapData.tilesetPath = "assets/tilesets/" + line;
 
     file >> mapData.numTiles >> mapData.tileWidth >> mapData.tileHeight;
     file >> mapData.mapWidth >> mapData.mapHeight;
 
     mapData.tiles.resize(mapData.mapHeight, vector<int>(mapData.mapWidth));
     mapData.items.resize(mapData.mapHeight, vector<int>(mapData.mapWidth));
 
     // Ler tiles - converter qualquer valor >= 2 para terra (0)
     for (int i = 0; i < mapData.mapHeight; i++)
     {
         for (int j = 0; j < mapData.mapWidth; j++)
         {
             file >> mapData.tiles[i][j];
             // Simplificar: apenas terra(0) ou lava(1)
             int valor = mapData.tiles[i][j];
             if (valor == 2)
                 mapData.tiles[i][j] = 0; // Terra
             else if (valor == 4)
                 mapData.tiles[i][j] = 1; // Lava
             else
                 mapData.tiles[i][j] = -1; // Tile inválido
         }
     }
 
     // Ler items - apenas moedas(1) são válidas
     for (int i = 0; i < mapData.mapHeight; i++)
     {
         for (int j = 0; j < mapData.mapWidth; j++)
         {
             file >> mapData.items[i][j];
             // Garantir que apenas 0 (vazio) ou 1 (moeda) são válidos
             if (mapData.items[i][j] != 1) {
                 mapData.items[i][j] = 0;
             }
         }
     }
 
     file.close();
     return true;
 }
 
 int setupShader()
 {
     GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
     glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
     glCompileShader(vertexShader);
 
     GLint success;
     GLchar infoLog[512];
     glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
     if (!success)
     {
         glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
         std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
     }
 
     GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
     glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
     glCompileShader(fragmentShader);
 
     glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
     if (!success)
     {
         glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
         std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
     }
 
     GLuint shaderProgram = glCreateProgram();
     glAttachShader(shaderProgram, vertexShader);
     glAttachShader(shaderProgram, fragmentShader);
     glLinkProgram(shaderProgram);
 
     glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
     if (!success)
     {
         glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
         std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
     }
 
     glDeleteShader(vertexShader);
     glDeleteShader(fragmentShader);
 
     return shaderProgram;
 }
 
 int setupSprite(int nAnimations, int nFrames, float &ds, float &dt)
 {
    ds = 1.0 / (float)nFrames;
    dt = 1.0 / (float)nAnimations;
 
    GLfloat vertices[] = {
        -0.5,  0.5, 0.0, 0.0, 0.0,
        -0.5, -0.5, 0.0, 0.0, dt,
         0.5,  0.5, 0.0, ds,  0.0,
         0.5, -0.5, 0.0, ds,  dt
    };
 
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
 
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);
 
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
 
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
 
    return VAO;
 }
 
 int setupCoin(float &ds, float &dt)
 {
    ds = 1.0;
    dt = 1.0;
 
    GLfloat vertices[] = {
        -0.5,  0.5, 0.0, 0.0, 0.0,
        -0.5, -0.5, 0.0, 0.0, 1.0,
         0.5,  0.5, 0.0, 1.0, 0.0,
         0.5, -0.5, 0.0, 1.0, 1.0
    };
 
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
 
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);
 
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
 
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
 
    return VAO;
 }
 
 int setupTile(int nTiles, float &ds, float &dt)
 {
    ds = 1.0 / (float)nTiles;
    dt = 1.0;
 
    float th = 1.0, tw = 1.0;
 
    GLfloat vertices[] = {
        0.0,     th/2.0f, 0.0, 0.0,      dt/2.0f,
        tw/2.0f, th,      0.0, ds/2.0f,  dt,
        tw/2.0f, 0.0,     0.0, ds/2.0f,  0.0,
         tw,      th/2.0f, 0.0, ds,       dt/2.0f
     };
 
     GLuint VBO, VAO;
     glGenBuffers(1, &VBO);
     glBindBuffer(GL_ARRAY_BUFFER, VBO);
     glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
 
     glGenVertexArrays(1, &VAO);
     glBindVertexArray(VAO);
 
     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
     glEnableVertexAttribArray(0);
 
     glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
     glEnableVertexAttribArray(1);
 
     glBindBuffer(GL_ARRAY_BUFFER, 0);
     glBindVertexArray(0);
 
     return VAO;
 }
 
 int loadTexture(string filePath, int &width, int &height)
 {
     GLuint texID;
 
     glGenTextures(1, &texID);
     glBindTexture(GL_TEXTURE_2D, texID);
 
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
 
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 
     int nrChannels;
     unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);
  
     if (data)
     {
         if (nrChannels == 3)
         {
             glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
         }
         else
         {
             glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
         }
         glGenerateMipmap(GL_TEXTURE_2D);
     }
     else
     {
         std::cout << "Failed to load texture" << std::endl;
     }
 
     stbi_image_free(data);
     glBindTexture(GL_TEXTURE_2D, 0);
 
     return texID;
 }