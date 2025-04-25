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

// Constante gravitacional
const float G = 6.67430e-11;  // Constante gravitacional em m^3 kg^-1 s^-2

// Variáveis da janela
int windowWidth = 800;
int windowHeight = 600;
bool fullscreen = false;

// Variáveis de posição da câmera
float cameraX = 0.0f;
float cameraY = 30.0f;
float cameraZ = -60.0f;        // Afastei a câmera para mostrar os objetos
float cameraSpeed = 1.0f;

// Variáveis de rotação da câmera
float cameraYaw = 0.0f;   // Rotação em torno do eixo Y (esquerda-direita)
float cameraPitch = 0.0f; // Rotação em torno do eixo X (cima-baixo)
int lastMouseX = -1;
int lastMouseY = -1;
float mouseSensitivity = 0.2f;

// Variáveis para captura do mouse
bool mouseActive = true; // Se o movimento do mouse está ativo

// Vetores da câmera para movimento
float forwardX, forwardY, forwardZ;  // Vetor para frente
float rightX, rightY, rightZ;        // Vetor para direita
float upX, upY, upZ;                 // Vetor para cima

// Definição do tipo de objeto celeste
typedef struct {
    float posX, posY, posZ;     // Posição
    float velX, velY, velZ;     // Velocidade
    float accX, accY, accZ;     // Aceleração
    float mass;                // Massa em kg
    float radius;              // Raio em unidades GL
    float rotationAngle;       // Ângulo de rotação em torno do próprio eixo
    float rotationSpeed;       // Velocidade de rotação
    GLuint texture;            // Textura do objeto
    float r, g, b;             // Cor do objeto (para backup se não tiver textura)
    bool fixed;                // Se o objeto está fixo no espaço (não se move pela gravidade)
} CelestialObject;

// Array de objetos celestes
#define MAX_OBJECTS 10
CelestialObject objects[MAX_OBJECTS];
int objectCount = 0;

// Variáveis de física
float timeStep = 1.0f;     // Fator de escala de tempo para ajustar velocidade da simulação
float scale = 1000000.0f;   // Escala para converter unidades astronômicas em unidades GL
float rotationAngles[MAX_OBJECTS]; // Ângulos de rotação para cada planeta

// Flags de estado
int lightEnabled = 1;  // Iluminação habilitada por padrão
bool simulationPaused = false;

// Propriedades de iluminação
GLfloat lightAmbient[] = { 0.5f, 0.5f, 0.5f, 1.0f };  // Luz ambiente aumentada
GLfloat lightDiffuse[] = { 1.0f, 1.0f, 0.8f, 1.0f };  // Luz difusa amarelada para o sol
GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Componente especular branca
GLfloat lightPosition[4];                            // Posição da luz, será atualizada durante o rendering

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
                       float r, float g, float b, bool fixed);
void updateGravitationalForces();

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
                       float r, float g, float b, bool fixed) {
    if (objectCount < MAX_OBJECTS) {
        CelestialObject obj = {
            .posX = posX, .posY = posY, .posZ = posZ,
            .velX = velX, .velY = velY, .velZ = velZ,
            .accX = 0.0f, .accY = 0.0f, .accZ = 0.0f,
            .mass = mass,
            .radius = radius,
            .rotationAngle = 0.0f,
            .rotationSpeed = 1.0f, // velocidade padrão de rotação
            .texture = texture,
            .r = r, .g = g, .b = b,
            .fixed = fixed
        };
        objects[objectCount++] = obj;
    } else {
        printf("Erro: Número máximo de objetos atingido.\n");
    }
}

// Atualizar forças gravitacionais entre todos os objetos
void updateGravitationalForces() {
    // Resetar acelerações
    for (int i = 0; i < objectCount; i++) {
        if (!objects[i].fixed) {
            objects[i].accX = 0;
            objects[i].accY = 0;
            objects[i].accZ = 0;
        }
    }
    
    // Calcular forças gravitacionais entre todos os pares de objetos
    for (int i = 0; i < objectCount; i++) {
        if (objects[i].fixed) continue; // Objetos fixos não se movem
        
        for (int j = 0; j < objectCount; j++) {
            if (i == j) continue; // Pular auto-interação
            
            // Calcular vetor distância entre os objetos
            float dx = objects[j].posX - objects[i].posX;
            float dy = objects[j].posY - objects[i].posY;
            float dz = objects[j].posZ - objects[i].posZ;
            
            // Distância ao quadrado
            float distSq = dx*dx + dy*dy + dz*dz;
            
            // Evitar divisão por zero ou forças muito grandes quando muito próximos
            if (distSq < (objects[i].radius + objects[j].radius) * (objects[i].radius + objects[j].radius)) {
                // Colisão ou objetos muito próximos, podemos implementar depois uma colisão elástica
                continue;
            }
            
            // Calcular a força da gravidade: F = G * (m1 * m2) / r^2
            float force = G * objects[i].mass * objects[j].mass / distSq;
            
            // Calcular a distância real
            float dist = sqrt(distSq);
            
            // Componentes da aceleração (F = m*a, então a = F/m)
            // Normalizar o vetor distância para obter a direção
            float ax = force * dx / (dist * objects[i].mass);
            float ay = force * dy / (dist * objects[i].mass);
            float az = force * dz / (dist * objects[i].mass);
            
            // Adicionar à aceleração total
            objects[i].accX += ax * timeStep * scale;
            objects[i].accY += ay * timeStep * scale;
            objects[i].accZ += az * timeStep * scale;
        }
    }
}

// Atualizar a física de todos os objetos
void updatePhysics() {
    if (simulationPaused) return;
    
    // Velocidades orbitais relativas (do mais rápido ao mais lento)
    const float orbitalSpeeds[] = {
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
    
    // Distâncias dos planetas ao Sol (definidas na função init)
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
    loadMoonTexture();
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
        true                  // fixo, não se move
    );
    
    // Mercúrio
    addCelestialObject(
        5.0f, 0.0f, 0.0f,     // posição (a 5 unidades do Sol)
        0.0f, 0.0f, 0.0f,     // velocidade (será calculada na física)
        3.3011e23,            // massa em kg
        0.4f,                 // raio visual
        mercuryTexName,       // textura
        0.7f, 0.7f, 0.7f,     // cor cinza (backup)
        true                  // fixo inicialmente
    );
    
    // Vênus
    addCelestialObject(
        7.0f, 0.0f, 0.0f,     // posição (a 7 unidades do Sol)
        0.0f, 0.0f, 0.0f,     // velocidade (será calculada na física)
        4.8675e24,            // massa em kg
        0.9f,                 // raio visual
        venusTexName,         // textura
        0.9f, 0.7f, 0.0f,     // cor laranja-amarelada (backup)
        true                  // fixo inicialmente
    );
    
    // Terra
    addCelestialObject(
        10.0f, 0.0f, 0.0f,    // posição (a 10 unidades do Sol)
        0.0f, 0.0f, 0.0f,     // velocidade (será calculada na física)
        5.972e24,             // massa da Terra em kg
        1.0f,                 // raio
        earthTexName,         // textura
        0.0f, 0.5f, 1.0f,     // cor azul (backup)
        true                  // fixo inicialmente
    );
    
    // Marte
    addCelestialObject(
        15.0f, 0.0f, 0.0f,    // posição (a 15 unidades do Sol)
        0.0f, 0.0f, 0.0f,     // velocidade (será calculada na física)
        6.4171e23,            // massa em kg
        0.5f,                 // raio visual
        marsTexName,          // textura
        1.0f, 0.3f, 0.0f,     // cor vermelha (backup)
        true                  // fixo inicialmente
    );
    
    // Júpiter
    addCelestialObject(
        25.0f, 0.0f, 0.0f,    // posição (a 25 unidades do Sol)
        0.0f, 0.0f, 0.0f,     // velocidade (será calculada na física)
        1.8982e27,            // massa em kg
        2.0f,                 // raio visual
        jupiterTexName,       // textura
        0.9f, 0.7f, 0.5f,     // cor bege (backup)
        true                  // fixo inicialmente
    );
    
    // Saturno
    addCelestialObject(
        35.0f, 0.0f, 0.0f,    // posição (a 35 unidades do Sol)
        0.0f, 0.0f, 0.0f,     // velocidade (será calculada na física)
        5.6834e26,            // massa em kg
        1.8f,                 // raio visual
        saturnTexName,        // textura
        0.9f, 0.8f, 0.5f,     // cor amarelada (backup)
        true                  // fixo inicialmente
    );
    
    // Urano
    addCelestialObject(
        45.0f, 0.0f, 0.0f,    // posição (a 45 unidades do Sol)
        0.0f, 0.0f, 0.0f,     // velocidade (será calculada na física)
        8.6810e25,            // massa em kg
        1.5f,                 // raio visual
        uranusTexName,        // textura
        0.5f, 0.8f, 0.9f,     // cor azul-esverdeada (backup)
        true                  // fixo inicialmente
    );
    
    // Netuno
    addCelestialObject(
        55.0f, 0.0f, 0.0f,    // posição (a 55 unidades do Sol)
        0.0f, 0.0f, 0.0f,     // velocidade (será calculada na física)
        1.02413e26,           // massa em kg
        1.4f,                 // raio visual
        neptuneTexName,       // textura
        0.0f, 0.0f, 0.8f,     // cor azul escuro (backup)
        true                  // fixo inicialmente
    );
    
    // Imprimir instruções
    printf("\n--- Controles do Sistema Solar ---\n");
    printf("WASD: Movimento da câmera\n");
    printf("R/G: Subir/descer\n");
    printf("Mouse: Olhar ao redor\n");
    printf("M: Alternar controle do mouse\n");
    printf("L: Alternar iluminação\n");
    printf("F: Alternar tela cheia\n");
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
    // Atualizar física
    updatePhysics();
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Atualizar posição da luz para estar no centro do sol
    if (objectCount > 0) {
        lightPosition[0] = objects[0].posX;
        lightPosition[1] = objects[0].posY;
        lightPosition[2] = objects[0].posZ;
        lightPosition[3] = 1.0f;
        glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    }
    
    // Configurar iluminação global
    if (lightEnabled) {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
    } else {
        glDisable(GL_LIGHTING);
        glDisable(GL_LIGHT0);
    }
    
    // Desenhar cada objeto celeste
    for (int i = 0; i < objectCount; i++) {
        CelestialObject* obj = &objects[i];
        
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
        if (obj->texture > 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, obj->texture);
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        } else {
            glDisable(GL_TEXTURE_2D);
        }
        
        // Definir cor (será usada como fator de multiplicação para a textura)
        glColor3f(obj->r, obj->g, obj->b);
        
        // Posicionar e desenhar o objeto
        glPushMatrix();
        glTranslatef(obj->posX, obj->posY, obj->posZ);
        
        // Rotação do objeto em torno do próprio eixo
        glRotatef(obj->rotationAngle, 0.0f, 1.0f, 0.0f);
        
        // Criar uma esfera para o objeto
        GLUquadric* quadric = gluNewQuadric();
        gluQuadricTexture(quadric, GL_TRUE);
        gluQuadricNormals(quadric, GLU_SMOOTH);
        gluSphere(quadric, obj->radius, 32, 32);
        gluDeleteQuadric(quadric);
        
        glPopMatrix();
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
        default:
            break;
    }
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

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    // Usar double buffering para animação mais suave
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Sistema Solar Gravitacional");
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
