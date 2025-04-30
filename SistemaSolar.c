#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h> // Para o tipo bool

// Define M_PI se não estiver definido
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/* Criar textura de tabuleiro de xadrez */
#define checkImageWidth 64
#define checkImageHeight 64
static GLubyte checkImage[checkImageHeight][checkImageWidth][4];

#ifdef GL_VERSION_1_1
static GLuint texName;
static GLuint earthTexName;
static GLuint sunTexName;     // Textura para o sol
static GLuint moonTexName;    // Textura para a lua
static GLuint mercuryTexName; // Textura para Mercúrio
static GLuint venusTexName;   // Textura para Vênus
static GLuint marsTexName;    // Textura para Marte
static GLuint jupiterTexName; // Textura para Júpiter
static GLuint saturnTexName;  // Textura para Saturno
static GLuint uranusTexName;  // Textura para Urano
static GLuint neptuneTexName; // Textura para Netuno
#endif

// Variáveis da janela
int windowWidth = 800;
int windowHeight = 600;
bool fullscreen = false;

// Variáveis de posição da câmera
float cameraX = 0.0f;
float cameraY = 0.0f;
float cameraZ = 50.0f;
float cameraSpeed = 1.0f;

// Variáveis de rotação da câmera
float cameraYaw = 180.0f;   // Rotação em torno do eixo Y (esquerda-direita)
float cameraPitch = 0.0f;   // Rotação em torno do eixo X (cima-baixo)
int lastMouseX = -1;
int lastMouseY = -1;
float mouseSensitivity = 0.2f;

// Variável para captura do mouse
bool mouseActive = true; // Se o movimento do mouse está ativo

// Variável para controle do modo de câmera
int cameraFollowMode = -1; // -1: modo livre, 0-9: índice do planeta a seguir
bool earthAxisView = false; // Modo de visualização do eixo da Terra

// Vetores da câmera para movimento
float forwardX, forwardY, forwardZ;  // Vetor para frente
float rightX, rightY, rightZ;        // Vetor para direita
float upX, upY, upZ;                 // Vetor para cima

// Definição do tipo de objeto celeste
typedef struct {
    float posX, posY, posZ;     // Posição
    float velX, velY, velZ;     // Velocidade
    float mass;                // Massa em kg
    float radius;              // Raio em unidades GL
    float rotationAngle;       // Ângulo de rotação em torno do próprio eixo
    float rotationSpeed;       // Velocidade de rotação
    GLuint texture;            // Textura do objeto
    float r, g, b;             // Cor do objeto (para backup se não tiver textura)
    bool fixed;                // Se o objeto está fixo no espaço (não se move pela gravidade)
    char name[50];             // Nome do objeto celeste
} CelestialObject;

// Array de objetos celestes
#define MAX_OBJECTS 10
CelestialObject objects[MAX_OBJECTS];
int objectCount = 0;

// Variáveis de física
float timeStep = 0.1f;     // Fator de escala de tempo para ajustar velocidade da simulação
float rotationAngles[MAX_OBJECTS]; // Ângulos de rotação para cada planeta

// Flags de estado
int lightEnabled = 1;  // Iluminação habilitada por padrão
bool simulationPaused = false;
bool showOrbits = true;  // Mostrar órbitas por padrão

// Propriedades de iluminação
GLfloat lightAmbient[] = { 0.5f, 0.5f, 0.5f, 1.0f };  // Luz ambiente aumentada
GLfloat lightDiffuse[] = { 1.0f, 1.0f, 0.8f, 1.0f };  // Luz difusa amarelada para o sol
GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Componente especular branca
GLfloat lightPosition[4];                            // Posição da luz, será atualizada durante o rendering

// Raios orbitais para cada planeta (distâncias do Sol)
const float orbitalRadii[] = {
    0.0f,   // Sol (no centro)
    5.0f,   // Mercúrio
    7.0f,   // Vênus
    10.0f,  // Terra
    15.0f,  // Marte
    25.0f,  // Júpiter
    35.0f,  // Saturno
    45.0f,  // Urano
    55.0f   // Netuno
};

// Velocidades orbitais de cada planeta (em graus por segundo)
float orbitalSpeeds[] = {
    0.0f,   // Sol (não orbita)
    4.1f,   // Mercúrio (mais rápido)
    3.0f,   // Vênus
    2.5f,   // Terra
    2.0f,   // Marte
    1.0f,   // Júpiter
    0.7f,   // Saturno
    0.5f,   // Urano
    0.4f    // Netuno (mais lento)
};

// Protótipos de funções
void updateCamera();
void calculateCameraVectors();
void updatePhysics();
void loadEarthTexture();
void loadSunTexture();
void loadMoonTexture();
void loadMercuryTexture();
void loadVenusTexture();
void loadMarsTexture();
void loadJupiterTexture();
void loadSaturnTexture();
void loadUranusTexture();
void loadNeptuneTexture();
void setupLighting();
void toggleFullscreen();
void resizeWindow(int width, int height);
void addCelestialObject(float posX, float posY, float posZ, 
                       float velX, float velY, float velZ,
                       float mass, float radius, GLuint texture,
                       float r, float g, float b, bool fixed,
                       const char* name);
void renderText3D(const char* text, float x, float y, float z);
void renderOrbitPaths(); // Nova função para desenhar as órbitas

// Função genérica para carregar texturas
void loadTexture(const char* filename, GLuint* texId) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (data) {
        glGenTextures(1, texId);
        glBindTexture(GL_TEXTURE_2D, *texId);
        // Configurar parâmetros de wrapping e filtragem da textura
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Carregar os dados da textura (verificar se é RGB ou RGBA)
        if (nrChannels == 3) {
            gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, width, height, GL_RGB,
                              GL_UNSIGNED_BYTE, data);
        } else if (nrChannels == 4) {
            gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA,
                              GL_UNSIGNED_BYTE, data);
        }
        stbi_image_free(data);
    } else {
        fprintf(stderr, "Falha ao carregar textura: %s\n", filename);
    }
}

void loadEarthTexture() {
    loadTexture("texturas/2k_earth_daymap.jpg", &earthTexName);
}

void loadSunTexture() {
    loadTexture("texturas/2k_sun.jpg", &sunTexName);
}

void loadMoonTexture() {
    loadTexture("moon_texture.jpg", &moonTexName);
}

void loadMercuryTexture() {
    loadTexture("texturas/2k_mercury.jpg", &mercuryTexName);
}

void loadVenusTexture() {
    loadTexture("texturas/2k_venus_surface.jpg", &venusTexName);
}

void loadMarsTexture() {
    loadTexture("texturas/2k_mars.jpg", &marsTexName);
}

void loadJupiterTexture() {
    loadTexture("texturas/2k_jupiter.jpg", &jupiterTexName);
}

void loadSaturnTexture() {
    loadTexture("texturas/2k_saturn.jpg", &saturnTexName);
}

void loadUranusTexture() {
    loadTexture("texturas/2k_uranus.jpg", &uranusTexName);
}

void loadNeptuneTexture() {
    loadTexture("texturas/2k_neptune.jpg", &neptuneTexName);
}

// Configurar iluminação
void setupLighting() {
    // Habilitar iluminação
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    // Definir propriedades da luz
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    
    // Definir propriedades do material padrão
    GLfloat materialAmbient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat materialDiffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat materialSpecular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat materialShininess[] = { 50.0f };
    
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess);
    
    // Habilitar materiais
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    
    // Outros estados do OpenGL para renderização mais realista
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    
    // Garantir que a iluminação está no estado correto de acordo com a flag
    if (lightEnabled) {
        glEnable(GL_LIGHTING);
    } else {
        glDisable(GL_LIGHTING);
    }
}

// Alternar modo tela cheia
void toggleFullscreen() {
    fullscreen = !fullscreen;
    if (fullscreen) {
        glutFullScreen(); // Entrar em modo tela cheia
    } else {
        glutReshapeWindow(windowWidth, windowHeight); // Voltar para janela
        glutPositionWindow(100, 100); // Reposicionar a janela
    }
}

// Redimensionar a janela
void resizeWindow(int width, int height) {
    if (!fullscreen) {
        windowWidth = width;
        windowHeight = height;
    }
    
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat)width / (GLfloat)height, 0.1, 1000.0); // Aumentado o far plane
    
    calculateCameraVectors();
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Aplicar rotações primeiro (yaw em torno do eixo Y, pitch em torno do eixo X)
    glRotatef(cameraPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraYaw, 0.0f, 1.0f, 0.0f);
    
    // Depois aplicar translação
    glTranslatef(cameraX, cameraY, cameraZ);
}

// Adicionar um objeto celeste ao sistema
void addCelestialObject(float posX, float posY, float posZ, 
                       float velX, float velY, float velZ,
                       float mass, float radius, GLuint texture,
                       float r, float g, float b, bool fixed,
                       const char* name) {
    if (objectCount < MAX_OBJECTS) {
        CelestialObject obj = {
            .posX = posX, .posY = posY, .posZ = posZ,
            .velX = velX, .velY = velY, .velZ = velZ,
            .mass = mass,
            .radius = radius,
            .rotationAngle = 0.0f,
            .rotationSpeed = 1.0f, // velocidade padrão de rotação
            .texture = texture,
            .r = r, .g = g, .b = b,
            .fixed = fixed,
            .name = ""
        };
        strcpy(obj.name, name);
        objects[objectCount++] = obj;
    } else {
        printf("Erro: Número máximo de objetos atingido.\n");
    }
}

// Atualizar a física de todos os objetos
void updatePhysics() {
    if (simulationPaused) return;
    
    // Atualizar a posição de cada planeta (exceto o Sol)
    for (int i = 1; i < objectCount; i++) {
        // Incrementar o ângulo de rotação específico deste planeta
        rotationAngles[i] += orbitalSpeeds[i] * timeStep * 0.2f;
        if (rotationAngles[i] > 360.0f) {
            rotationAngles[i] -= 360.0f;
        }
        
        // Atualizar a posição do planeta para orbitar ao redor do Sol
        objects[i].posX = orbitalRadii[i] * cos(rotationAngles[i] * M_PI / 180.0f);
        objects[i].posZ = orbitalRadii[i] * sin(rotationAngles[i] * M_PI / 180.0f);
        // Manter os planetas no plano XZ
        objects[i].posY = 0.0f;
        
        // Também atualizar a rotação do próprio planeta
        objects[i].rotationAngle += objects[i].rotationSpeed * timeStep;
        if (objects[i].rotationAngle > 360.0f) {
            objects[i].rotationAngle -= 360.0f;
        }
    }
}

void init(void) {
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glShadeModel(GL_SMOOTH); // Sombreamento suave
    glEnable(GL_DEPTH_TEST);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    // Carregar as texturas
    loadEarthTexture();
    loadSunTexture();
    //loadMoonTexture();
    loadMercuryTexture();
    loadVenusTexture();
    loadMarsTexture();
    loadJupiterTexture();
    loadSaturnTexture();
    loadUranusTexture();
    loadNeptuneTexture();
    
    // Configurar iluminação
    setupLighting();
    
    // Limpar o array de objetos celestes
    objectCount = 0;
    
    // Inicializar ângulos de rotação
    for (int i = 0; i < MAX_OBJECTS; i++) {
        rotationAngles[i] = 0.0f;
    }
    
    // === Criar objetos celestes (Sol e planetas) ===
    
    // Sol (fixo no centro)
    addCelestialObject(
        0.0f, 0.0f, 0.0f,     // posição
        0.0f, 0.0f, 0.0f,     // velocidade
        1.989e30,             // massa do Sol em kg
        3.0f,                 // raio visual
        sunTexName,           // textura
        1.0f, 1.0f, 0.0f,     // cor amarela (backup)
        true,                 // fixo, não se move
        "Sol"                // nome do objeto
    );
    // Ajustar rotação do Sol
    objects[objectCount-1].rotationSpeed = 0.5f;  // Rotação mais lenta para o Sol
    
    // Mercúrio
    addCelestialObject(
        orbitalRadii[1], 0.0f, 0.0f,     // posição (usar raio orbital global)
        0.0f, 0.0f, 0.0f,     // velocidade (será calculada na física)
        3.3011e23,            // massa em kg
        0.4f,                 // raio visual
        mercuryTexName,       // textura
        0.7f, 0.7f, 0.7f,     // cor cinza (backup)
        true,                 // fixo inicialmente
        "Mercurio"          // nome do objeto
    );
    objects[objectCount-1].rotationSpeed = 0.1f;  // Mercúrio é lento (58.6 dias terrestres)
    
    // Vênus
    addCelestialObject(
        orbitalRadii[2], 0.0f, 0.0f,     // posição (usar raio orbital global)
        0.0f, 0.0f, 0.0f,     // velocidade (será calculada na física)
        4.8675e24,            // massa em kg
        0.9f,                 // raio visual
        venusTexName,         // textura
        0.9f, 0.7f, 0.0f,     // cor laranja-amarelada (backup)
        true,                 // fixo inicialmente
        "Venus"            // nome do objeto
    );
    objects[objectCount-1].rotationSpeed = 0.05f;  // Vênus é muito lento (243 dias terrestres) e retrógrado
    
    // Terra
    addCelestialObject(
        orbitalRadii[3], 0.0f, 0.0f,    // posição (usar raio orbital global) 
        0.0f, 0.0f, 0.0f,     // velocidade (será calculada na física)
        5.972e24,             // massa da Terra em kg
        1.0f,                 // raio
        earthTexName,         // textura
        0.0f, 0.5f, 1.0f,     // cor azul (backup)
        true,                 // fixo inicialmente
        "Terra"            // nome do objeto
    );
    objects[objectCount-1].rotationSpeed = 2.0f;  // Rotação da Terra (1 dia)
    
    // Marte
    addCelestialObject(
        orbitalRadii[4], 0.0f, 0.0f,    // posição (usar raio orbital global)
        0.0f, 0.0f, 0.0f,     // velocidade (será calculada na física)
        6.4171e23,            // massa em kg
        0.5f,                 // raio visual
        marsTexName,          // textura
        1.0f, 0.3f, 0.0f,     // cor vermelha (backup)
        true,                 // fixo inicialmente
        "Marte"            // nome do objeto
    );
    objects[objectCount-1].rotationSpeed = 1.9f;  // Marte (24.6 horas)
    
    // Júpiter
    addCelestialObject(
        orbitalRadii[5], 0.0f, 0.0f,    // posição (usar raio orbital global)
        0.0f, 0.0f, 0.0f,     // velocidade (será calculada na física)
        1.8982e27,            // massa em kg
        2.0f,                 // raio visual
        jupiterTexName,       // textura
        0.9f, 0.7f, 0.5f,     // cor bege (backup)
        true,                 // fixo inicialmente
        "Jupiter"            // nome do objeto
    );
    objects[objectCount-1].rotationSpeed = 5.0f;  // Júpiter é rápido (9.9 horas)
    
    // Saturno
    addCelestialObject(
        orbitalRadii[6], 0.0f, 0.0f,    // posição (usar raio orbital global)
        0.0f, 0.0f, 0.0f,     // velocidade (será calculada na física)
        5.6834e26,            // massa em kg
        1.8f,                 // raio visual
        saturnTexName,        // textura
        0.9f, 0.8f, 0.5f,     // cor amarelada (backup)
        true,                 // fixo inicialmente
        "Saturno"            // nome do objeto
    );
    objects[objectCount-1].rotationSpeed = 4.5f;  // Saturno é rápido (10.7 horas)
    
    // Urano
    addCelestialObject(
        orbitalRadii[7], 0.0f, 0.0f,    // posição (usar raio orbital global)
        0.0f, 0.0f, 0.0f,     // velocidade (será calculada na física)
        8.6810e25,            // massa em kg
        1.5f,                 // raio visual
        uranusTexName,        // textura
        0.5f, 0.8f, 0.9f,     // cor azul-esverdeada (backup)
        true,                 // fixo inicialmente
        "Urano"            // nome do objeto
    );
    objects[objectCount-1].rotationSpeed = -3.0f;  // Urano tem rotação retrógrada (17.2 horas)
    
    // Netuno
    addCelestialObject(
        orbitalRadii[8], 0.0f, 0.0f,    // posição (usar raio orbital global)
        0.0f, 0.0f, 0.0f,     // velocidade (será calculada na física)
        1.02413e26,           // massa em kg
        1.4f,                 // raio visual
        neptuneTexName,       // textura
        0.0f, 0.0f, 0.8f,     // cor azul escuro (backup)
        true,                 // fixo inicialmente
        "Netuno"            // nome do objeto
    );
    objects[objectCount-1].rotationSpeed = 3.5f;  // Netuno (16.1 horas)
    
    // Imprimir instruções
    printf("\n--- Controles do Sistema Solar ---\n");
    printf("WASD: Movimento da câmera\n");
    printf("R/G: Subir/descer\n");
    printf("Mouse: Olhar ao redor\n");
    printf("M: Alternar controle do mouse\n");
    printf("L: Alternar iluminação\n");
    printf("F: Alternar tela cheia\n");
    printf("O: Alternar linhas das órbitas\n");
    printf("[/]: Diminuir/aumentar largura da janela\n");
    printf("-/+: Diminuir/aumentar altura da janela\n");
    printf(",/.: Diminuir/aumentar velocidade da simulação\n");
    printf("P: Pausar/Continuar simulação\n");
    printf("ESC: Sair\n");
    printf("----------------------------------\n\n");
}

// Calcular vetores de direção da câmera com base em yaw e pitch
void calculateCameraVectors() {
    // Calcular o novo vetor Forward
    forwardX = -sin(cameraYaw * M_PI / 180.0f) * cos(cameraPitch * M_PI / 180.0f);
    forwardY = sin(cameraPitch * M_PI / 180.0f);
    forwardZ = cos(cameraYaw * M_PI / 180.0f) * cos(cameraPitch * M_PI / 180.0f);
    
    // Calcular o vetor Right
    rightX = -cos(cameraYaw * M_PI / 180.0f);
    rightY = 0.0f;
    rightZ = -sin(cameraYaw * M_PI / 180.0f);
    
    // Calcular o vetor Up usando produto vetorial
    upX = forwardY * rightZ - forwardZ * rightY;
    upY = forwardZ * rightX - forwardX * rightZ;
    upZ = forwardX * rightY - forwardY * rightX;
    
    // Normalizar vetores
    float lenForward = sqrt(forwardX*forwardX + forwardY*forwardY + forwardZ*forwardZ);
    forwardX /= lenForward;
    forwardY /= lenForward;
    forwardZ /= lenForward;
    
    float lenRight = sqrt(rightX*rightX + rightY*rightY + rightZ*rightZ);
    rightX /= lenRight;
    rightY /= lenRight;
    rightZ /= lenRight;
    
    float lenUp = sqrt(upX*upX + upY*upY + upZ*upZ);
    upX /= lenUp;
    upY /= lenUp;
    upZ /= lenUp;
}

void updateCamera() {
    calculateCameraVectors();
    
    // Se estiver no modo de seguir planeta, atualizar posição da câmera
    if (cameraFollowMode >= 0 && cameraFollowMode < objectCount) {
        // Obter a posição do planeta atual
        float planetX = objects[cameraFollowMode].posX;
        float planetY = objects[cameraFollowMode].posY;
        float planetZ = objects[cameraFollowMode].posZ;
        
        if (earthAxisView && strcmp(objects[cameraFollowMode].name, "Terra") == 0) {
            // Modo de visualização do eixo da Terra
            float distance = 3.0f; // Aplicar zoom na distância
            float height = 2.0f;   // Aplicar zoom na altura
            
            // Calcular ângulo de rotação da Terra
            float angle = objects[cameraFollowMode].rotationAngle * M_PI / 180.0f;
            
            // Posicionar a câmera em uma órbita fixa ao redor do eixo da Terra
            cameraX = planetX + distance * cos(angle);
            cameraY = planetY + height;
            cameraZ = planetZ + distance * sin(angle);
            
            // Fazer a câmera olhar para o centro da Terra
            float dx = planetX - cameraX;
            float dy = planetY - cameraY;
            float dz = planetZ - cameraZ;
            
            // Calcular os ângulos para olhar para a Terra
            float dist = sqrt(dx*dx + dz*dz);
            cameraPitch = -atan2(dy, dist) * 180.0f / M_PI;
            cameraYaw = atan2(dz, dx) * 180.0f / M_PI;
        } else {
            // Modo normal de seguir planeta
            float distance = 5.0f; // Aplicar zoom na distância
            cameraX = planetX - forwardX * distance;
            cameraY = planetY - forwardY * distance;
            cameraZ = planetZ - forwardZ * distance;
        }
    }
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Aplicar rotações primeiro (yaw em torno do eixo Y, pitch em torno do eixo X)
    glRotatef(cameraPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraYaw, 0.0f, 1.0f, 0.0f);
    
    // Depois aplicar translação
    glTranslatef(cameraX, cameraY, cameraZ);
    
    glutPostRedisplay();
}

void display(void) {
    // Atualiza a física
    updatePhysics();
    
    // Limpa o buffer de cores e profundidade
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Atualiza a posição da luz (sol)
    GLfloat lightPosition[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    // Configurar iluminação global
    if (lightEnabled) {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
    } else {
        glDisable(GL_LIGHTING);
        glDisable(GL_LIGHT0);
    }
    
    // Desenhar as órbitas dos planetas
    renderOrbitPaths();
    
    // Desenha os objetos celestes
    for (int i = 0; i < objectCount; i++) {
        glPushMatrix();
        
        // Posiciona o objeto
        glTranslatef(objects[i].posX, objects[i].posY, objects[i].posZ);
        
        // O sol (primeiro objeto) é autoluminoso, desligar iluminação para ele
        if (i == 0) {
            glDisable(GL_LIGHTING);
            // Configurar o Sol para emitir luz própria (material emissor)
            GLfloat emission[] = {1.0f, 0.9f, 0.2f, 1.0f};
            glMaterialfv(GL_FRONT, GL_EMISSION, emission);
        } else if (lightEnabled) {
            glEnable(GL_LIGHTING);
            // Desativar emissão para os planetas
            GLfloat no_emission[] = {0.0f, 0.0f, 0.0f, 1.0f};
            glMaterialfv(GL_FRONT, GL_EMISSION, no_emission);
        }
        
        // Usar textura se o objeto tiver uma textura válida
        if (objects[i].texture > 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, objects[i].texture);
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        } else {
            glDisable(GL_TEXTURE_2D);
        }
        
        // Definir cor (será usada como fator de multiplicação para a textura)
        glColor3f(objects[i].r, objects[i].g, objects[i].b);
        
        // Corrigir a orientação das texturas antes de aplicar a rotação do planeta
        // Girar 90 graus em torno do eixo X para alinhar corretamente as texturas
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        
        // Adicionar inclinações axiais para planetas específicos
        if (strcmp(objects[i].name, "Terra") == 0) {
            // Terra tem uma inclinação de 23.5 graus
            glRotatef(23.5f, 1.0f, 0.0f, 0.0f);
        } else if (strcmp(objects[i].name, "Urano") == 0) {
            // Urano tem uma inclinação extrema de cerca de 98 graus
            glRotatef(98.0f, 1.0f, 0.0f, 0.0f);
        } else if (strcmp(objects[i].name, "Saturno") == 0) {
            // Saturno tem uma inclinação de 26.7 graus
            glRotatef(26.7f, 1.0f, 0.0f, 0.0f);
        } else if (strcmp(objects[i].name, "Netuno") == 0) {
            // Netuno tem uma inclinação de cerca de 28 graus
            glRotatef(28.0f, 1.0f, 0.0f, 0.0f);
        } else if (strcmp(objects[i].name, "Marte") == 0) {
            // Marte tem uma inclinação de 25 graus
            glRotatef(25.0f, 1.0f, 0.0f, 0.0f);
        } else if (strcmp(objects[i].name, "Jupiter") == 0) {
            // Júpiter tem uma inclinação de 3.1 graus
            glRotatef(3.1f, 1.0f, 0.0f, 0.0f);
        } else if (strcmp(objects[i].name, "Venus") == 0) {
            // Vênus tem uma rotação retrógrada com inclinação de 177 graus
            glRotatef(177.0f, 1.0f, 0.0f, 0.0f);
        } else if (strcmp(objects[i].name, "Mercurio") == 0) {
            // Mercúrio tem uma pequena inclinação de 0.034 graus
            glRotatef(0.034f, 1.0f, 0.0f, 0.0f);
        }
        
        // Rotação do objeto em torno do próprio eixo
        // Rotacionar em torno do eixo Z que agora está alinhado com o polo norte-sul
        glRotatef(objects[i].rotationAngle, 0.0f, 0.0f, 1.0f);
        
        // Criar uma esfera para o objeto
        GLUquadric* quadric = gluNewQuadric();
        gluQuadricTexture(quadric, GL_TRUE);
        gluQuadricNormals(quadric, GLU_SMOOTH);
        
        // Definir a orientação da textura para os quadrics
        gluQuadricOrientation(quadric, GLU_OUTSIDE);
        gluQuadricDrawStyle(quadric, GLU_FILL);
        
        // Desenhar a esfera
        gluSphere(quadric, objects[i].radius, 32, 32);
        gluDeleteQuadric(quadric);
        
        // Adicionar anéis para Saturno
        if (strcmp(objects[i].name, "Saturno") == 0) {
            // Desfazer a rotação do planeta para os anéis
            glRotatef(-objects[i].rotationAngle, 0.0f, 0.0f, 1.0f);
            
            // Ajustar a orientação para os anéis (desfazendo a rotação em X)
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            
            // Rotacionar os anéis apropriadamente
            glRotatef(75.0f, 1.0f, 0.0f, 0.0f);
            
            // Criar um novo quadric para os anéis
            GLUquadric* ringQuadric = gluNewQuadric();
            gluQuadricTexture(ringQuadric, GL_TRUE);
            gluQuadricNormals(ringQuadric, GLU_SMOOTH);
            
            // Desabilitar iluminação para os anéis
            glDisable(GL_LIGHTING);
            
            // Definir cor dos anéis (tom amarelado)
            glColor3f(0.9f, 0.8f, 1.0f);
            
            // Desenhar três anéis com diferentes raios
            float innerRadius = objects[i].radius * 1.2f;
            float outerRadius = objects[i].radius * 2.0f;
            float ringThickness = 0.05f;
            
            // Anel médio
            gluDisk(ringQuadric, innerRadius + ringThickness * 2, 
                   innerRadius + ringThickness * 3, 32, 1);
            
            // 5 anéis entre o médio e o externo
            float spacing = (outerRadius - (innerRadius + ringThickness * 3)) / 6.0f;
            for(int j = 0; j < 5; j++) {
                float currentRadius = innerRadius + ringThickness * 3 + spacing * (j + 1);
                gluDisk(ringQuadric, currentRadius, currentRadius + ringThickness, 32, 1);
            }
            
            // Anel externo
            gluDisk(ringQuadric, outerRadius - ringThickness, 
                   outerRadius, 32, 1);
            
            // Reabilitar iluminação
            if (lightEnabled) {
                glEnable(GL_LIGHTING);
            }
            
            gluDeleteQuadric(ringQuadric);
        }
        
        glPopMatrix();
        
        // Renderizar o nome do planeta acima dele (posição ajustada para ficar mais próximo)
        renderText3D(objects[i].name, objects[i].posX, objects[i].posY + objects[i].radius + 0.5f, objects[i].posZ);
    }
    
    glDisable(GL_TEXTURE_2D);
    
    // Resetar emissão
    GLfloat no_emission[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT, GL_EMISSION, no_emission);
    
    // Usar double buffering para animação mais suave
    glutSwapBuffers();
    
    // Solicitar redesenho para animação
    glutPostRedisplay();
}

void reshape(int w, int h) {
    if (!fullscreen) {
        windowWidth = w;
        windowHeight = h;
    }
    
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat)w / (GLfloat)h, 0.1, 1000.0);
    
    calculateCameraVectors();
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Aplicar rotações primeiro (yaw em torno do eixo Y, pitch em torno do eixo X)
    glRotatef(cameraPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraYaw, 0.0f, 1.0f, 0.0f);
    
    // Depois aplicar translação
    glTranslatef(cameraX, cameraY, cameraZ);
}

void keyboard(unsigned char key, int x, int y) {
    float moveSpeed = cameraSpeed;
    
    switch (key) {
        case 27: // Tecla ESC
            exit(0);
            break;
        case 'w': // Mover para frente na direção da câmera
            cameraX += forwardX * moveSpeed;
            cameraY += forwardY * moveSpeed;
            cameraZ += forwardZ * moveSpeed;
            updateCamera();
            break;
        case 's': // Mover para trás na direção da câmera
            cameraX -= forwardX * moveSpeed;
            cameraY -= forwardY * moveSpeed;
            cameraZ -= forwardZ * moveSpeed;
            updateCamera();
            break;
        case 'd': // Mover para a direita em relação à direção da câmera
            cameraX += rightX * moveSpeed;
            cameraY += rightY * moveSpeed;
            cameraZ += rightZ * moveSpeed;
            updateCamera();
            break;
        case 'a': // Mover para a esquerda em relação à direção da câmera
            cameraX -= rightX * moveSpeed;
            cameraY -= rightY * moveSpeed;
            cameraZ -= rightZ * moveSpeed;
            updateCamera();
            break;
        case 'g': // Mover para baixo em relação ao mundo
            cameraX -= upX * moveSpeed;
            cameraY -= upY * moveSpeed;
            cameraZ -= upZ * moveSpeed;
            updateCamera();
            break;
        case 'r': // Mover para cima em relação ao mundo
            cameraX += upX * moveSpeed;
            cameraY += upY * moveSpeed;
            cameraZ += upZ * moveSpeed;
            updateCamera();
            break;
        case 'f': // Alternar tela cheia
            toggleFullscreen();
            break;
        case 'F': // Alternar entre modo livre e modo de seguir planeta
            if (cameraFollowMode == -1) {
                // Se estiver em modo livre, mudar para seguir o primeiro planeta
                cameraFollowMode = 0;
                printf("Modo: Seguindo planeta\n");
            } else {
                // Se estiver seguindo um planeta, voltar para modo livre
                cameraFollowMode = -1;
                printf("Modo: Livre\n");
            }
            break;
        case 'p': // Pausar/Continuar simulação
        case 'P':
            simulationPaused = !simulationPaused;
            if (simulationPaused) {
                printf("Simulação: PAUSADA\n");
            } else {
                printf("Simulação: ATIVA\n");
            }
            break;
        case 'm': // Alternar ativação do controle do mouse
            mouseActive = !mouseActive;
            if (mouseActive) {
                printf("Controle da câmera com o mouse: ATIVADO\n");
            } else {
                printf("Controle da câmera com o mouse: DESATIVADO\n");
            }
            break;
        case '+': // Aumentar altura da janela
            if (!fullscreen) {
                windowHeight += 50;
                resizeWindow(windowWidth, windowHeight);
            }
            break;
        case '-': // Diminuir altura da janela
            if (!fullscreen && windowHeight > 200) {
                windowHeight -= 50;
                resizeWindow(windowWidth, windowHeight);
            }
            break;
        case ']': // Aumentar largura da janela
            if (!fullscreen) {
                windowWidth += 50;
                resizeWindow(windowWidth, windowHeight);
            }
            break;
        case '[': // Diminuir largura da janela
            if (!fullscreen && windowWidth > 200) {
                windowWidth -= 50;
                resizeWindow(windowWidth, windowHeight);
            }
            break;
        case 'L': // Alternar iluminação
        case 'l':
            lightEnabled = !lightEnabled;
            if (lightEnabled) {
                glEnable(GL_LIGHTING);
                printf("Iluminação: ATIVADA\n");
            } else {
                glDisable(GL_LIGHTING);
                printf("Iluminação: DESATIVADA\n");
            }
            break;
        case '.': // Aumentar velocidade da simulação
            timeStep *= 1.2f;
            printf("Velocidade da simulação: %.2f\n", timeStep);
            break;
        case ',': // Diminuir velocidade da simulação
            timeStep /= 1.2f;
            if (timeStep < 0.1f) timeStep = 0.1f; // Mínimo de 0.1
            printf("Velocidade da simulação: %.2f\n", timeStep);
            break;
        case 'T': // Alternar modo de visualização do eixo da Terra
        case 't':
            // Procurar o índice da Terra
            for (int i = 0; i < objectCount; i++) {
                if (strcmp(objects[i].name, "Terra") == 0) {
                    if (cameraFollowMode == i && earthAxisView) {
                        // Se já estiver seguindo a Terra e no modo eixo, voltar ao modo livre
                        cameraFollowMode = -1;
                        earthAxisView = false;
                        printf("Modo: Livre\n");
                    } else {
                        // Mudar para seguir a Terra e ativar modo eixo
                        cameraFollowMode = i;
                        earthAxisView = true;
                        printf("Modo: Visualizando eixo da Terra\n");
                    }
                    break;
                }
            }
            break;
        case 'O': // Alternar exibição das órbitas
        case 'o':
            showOrbits = !showOrbits;
            if (showOrbits) {
                printf("Linhas das órbitas: ATIVADAS\n");
            } else {
                printf("Linhas das órbitas: DESATIVADAS\n");
            }
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

void mouseMotion(int x, int y) {
    if (mouseActive) {
        if (lastMouseX >= 0 && lastMouseY >= 0) {
            // Calcular quanto o mouse se moveu
            int deltaX = x - lastMouseX;
            int deltaY = y - lastMouseY;
            
            // Atualizar ângulos da câmera
            cameraYaw += deltaX * mouseSensitivity;
            cameraPitch += deltaY * mouseSensitivity;
            
            // Limitar pitch para evitar inversão
            if (cameraPitch > 89.0f) cameraPitch = 89.0f;
            if (cameraPitch < -89.0f) cameraPitch = -89.0f;
            
            updateCamera();
        }
    }
    
    lastMouseX = x;
    lastMouseY = y;
}

void mouseEntry(int state) {
    if (state == GLUT_LEFT) {
        // Mouse saiu da janela, resetar rastreamento
        lastMouseX = -1;
        lastMouseY = -1;
    }
}

// Função para renderizar texto 3D
void renderText3D(const char* text, float x, float y, float z) {
    // Salvar o estado atual da matriz de projeção
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    
    // Salvar o estado atual da matriz de visualização
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    
    // Desativar iluminação para o texto
    glDisable(GL_LIGHTING);
    
    // Definir cor do texto (branco)
    glColor3f(1.0f, 1.0f, 1.0f);
    
    // Posicionar o texto
    glTranslatef(x, y, z);
    
    // Fazer o texto sempre olhar para a câmera (billboarding)
    // Obter a matriz de visualização atual
    GLfloat modelview[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
    
    // Resetar a rotação da matriz de visualização
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (i == j) {
                modelview[i * 4 + j] = 1.0f;
            } else {
                modelview[i * 4 + j] = 0.0f;
            }
        }
    }
    
    // Aplicar a matriz modificada
    glLoadMatrixf(modelview);
    
    // Escalar o texto para um tamanho adequado (reduzido para 0.2)
    glScalef(0.015f, 0.015f, 0.015f);
    
    // Renderizar o texto
    for (const char* p = text; *p; p++) {
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *p);
    }
    
    // Restaurar iluminação
    if (lightEnabled) {
        glEnable(GL_LIGHTING);
    }
    
    // Restaurar as matrizes
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

// Renderizar as órbitas dos planetas
void renderOrbitPaths() {
    // Se a exibição de órbitas estiver desativada, não renderizar nada
    if (!showOrbits) return;
    
    // Desabilitar texturas e iluminação para desenhar linhas
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    
    // Salvar cor atual
    float currentColor[4];
    glGetFloatv(GL_CURRENT_COLOR, currentColor);
    
    // Definir largura da linha
    glLineWidth(1.0f);
    
    // Cores para as órbitas de cada planeta (começando do índice 1, o Sol não tem órbita)
    float orbitColors[9][3] = {
        {1.0f, 1.0f, 1.0f},  // Cor não usada (Sol)
        {0.8f, 0.8f, 0.8f},  // Mercúrio - cinza claro
        {0.9f, 0.7f, 0.0f},  // Vênus - laranja amarelado
        {0.0f, 0.5f, 1.0f},  // Terra - azul
        {1.0f, 0.3f, 0.0f},  // Marte - vermelho
        {0.9f, 0.7f, 0.5f},  // Júpiter - bege
        {0.9f, 0.8f, 0.5f},  // Saturno - amarelado
        {0.5f, 0.8f, 0.9f},  // Urano - azul-esverdeado
        {0.0f, 0.0f, 0.8f}   // Netuno - azul escuro
    };
    
    // Desenhar órbita para cada planeta (exceto o Sol)
    for (int i = 1; i < objectCount; i++) {
        float radius = orbitalRadii[i];
        
        // Definir cor da órbita
        glColor3f(orbitColors[i][0], orbitColors[i][1], orbitColors[i][2]);
        
        // Desenhar círculo no plano XZ
        glBegin(GL_LINE_LOOP);
        for (int j = 0; j < 360; j += 5) {  // incremento de 5 graus para suavidade
            float angle = j * M_PI / 180.0f;
            float x = radius * cos(angle);
            float z = radius * sin(angle);
            glVertex3f(x, 0.0f, z);
        }
        glEnd();
    }
    
    // Restaurar cor original
    glColor4fv(currentColor);
    
    // Restaurar estados
    if (lightEnabled) {
        glEnable(GL_LIGHTING);
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Sistema Solar");
    
    init();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMotionFunc(mouseMotion);
    glutPassiveMotionFunc(mouseMotion);
    glutEntryFunc(mouseEntry);
    
    glutMainLoop();
    return 0;
}
