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

#ifdef GL_VERSION_1_1
static GLuint earthTexName;
static GLuint sunTexName;
#endif

// Constante gravitacional (valor ajustado para a simulação)
const double G = 6.67430e-11;  // Constante gravitacional em m^3 kg^-1 s^-2

// Variáveis da janela
int windowWidth = 800;
int windowHeight = 600;
bool fullscreen = false;

// Variáveis de posição da câmera
float cameraX = 0.0f;
float cameraY = 10.0f;
float cameraZ = -30.0f;
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
    double posX, posY, posZ;     // Posição
    double velX, velY, velZ;     // Velocidade
    double accX, accY, accZ;     // Aceleração
    double mass;                 // Massa em kg
    float radius;                // Raio em unidades GL
    GLuint texture;              // Textura do objeto
    float r, g, b;               // Cor do objeto (para backup se não tiver textura)
    bool fixed;                  // Se o objeto está fixo no espaço (não se move pela gravidade)
} CelestialObject;

// Array de objetos celestes
#define MAX_OBJECTS 2  // Apenas Sol e Terra
CelestialObject objects[MAX_OBJECTS];
int objectCount = 0;

// Variáveis de física
double timeStep = 0.01;     // Fator de escala de tempo
double simulationScale = 1.0e9;  // Escala da simulação: 1 unidade GL = 1 bilhão de metros
// Fator para amplificar a força gravitacional na simulação visual
double gravitationalFactor = 100.0;  // 9.0 é um valor alto para tornar o efeito visível

// Flags de estado
int lightEnabled = 1;  // Iluminação habilitada por padrão
bool simulationPaused = false;

// Propriedades de iluminação
GLfloat lightAmbient[] = { 0.5f, 0.5f, 0.5f, 1.0f };  // Luz ambiente
GLfloat lightDiffuse[] = { 1.0f, 1.0f, 0.8f, 1.0f };  // Luz difusa amarelada para o sol
GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Componente especular branca
GLfloat lightPosition[4];                            // Posição da luz, será atualizada durante o rendering

// Protótipos de funções
void updateCamera();
void calculateCameraVectors();
void updatePhysics();
void loadEarthTexture();
void loadSunTexture();
void setupLighting();
void toggleFullscreen();
void resizeWindow(int width, int height);
void addCelestialObject(double posX, double posY, double posZ, 
                       double velX, double velY, double velZ,
                       double mass, float radius, GLuint texture,
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
        printf("Textura carregada com sucesso: %s\n", filename);
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
void addCelestialObject(double posX, double posY, double posZ, 
                        double velX, double velY, double velZ,
                        double mass, float radius, GLuint texture,
                        float r, float g, float b, bool fixed) {
    if (objectCount < MAX_OBJECTS) {
        CelestialObject obj = {
            .posX = posX, .posY = posY, .posZ = posZ,
            .velX = velX, .velY = velY, .velZ = velZ,
            .accX = 0.0, .accY = 0.0, .accZ = 0.0,
            .mass = mass,
            .radius = radius,
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
    // Resetar acelerações para objetos não fixos
    for (int i = 0; i < objectCount; i++) {
        if (!objects[i].fixed) {
            objects[i].accX = 0.0;
            objects[i].accY = 0.0;
            objects[i].accZ = 0.0;
        }
    }
    
    // Calcular forças gravitacionais entre todos os pares de objetos
    for (int i = 0; i < objectCount; i++) {
        if (objects[i].fixed) continue; // Objetos fixos não são afetados pela gravidade
        
        for (int j = 0; j < objectCount; j++) {
            if (i == j) continue; // Pular auto-interação
            
            // Calcular vetor distância entre os objetos em unidades GL
            double dx = objects[j].posX - objects[i].posX;
            double dy = objects[j].posY - objects[i].posY;
            double dz = objects[j].posZ - objects[i].posZ;
            
            // Distância ao quadrado em unidades GL
            double distSq = dx*dx + dy*dy + dz*dz;
            
            // Evitar divisão por zero ou forças muito grandes quando muito próximos
            if (distSq < 0.1) { // Usar um valor fixo pequeno para evitar explosões
                continue;
            }
            
            // Calcular a distância real
            double dist = sqrt(distSq);
            
            // Abordagem simplificada: aplicar força diretamente proporcional a 1/r²
            // Usar um valor muito maior para a constante gravitacional na simulação
            double forceFactor = gravitationalFactor / (distSq);
            
            // Componentes normalizados da aceleração
            double accX = dx / dist * forceFactor;
            double accY = dy / dist * forceFactor;
            double accZ = dz / dist * forceFactor;
            
            // Adicionar à aceleração total do objeto i
            objects[i].accX += accX;
            objects[i].accY += accY;
            objects[i].accZ += accZ;
        }
    }
}

// Atualizar a física de todos os objetos
void updatePhysics() {
    if (simulationPaused) return;
    
    // Atualizar forças gravitacionais entre objetos
    updateGravitationalForces();
    
    // Atualizar velocidades e posições usando as acelerações calculadas
    for (int i = 0; i < objectCount; i++) {
        if (objects[i].fixed) continue; // Objetos fixos não se movem
        
        // Atualizar velocidade com base na aceleração (v = v + a*t)
        objects[i].velX += objects[i].accX * timeStep;
        objects[i].velY += objects[i].accY * timeStep;
        objects[i].velZ += objects[i].accZ * timeStep;
        
        // Atualizar posição com base na velocidade (p = p + v*t)
        objects[i].posX += objects[i].velX * timeStep;
        objects[i].posY += objects[i].velY * timeStep;
        objects[i].posZ += objects[i].velZ * timeStep;
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
    
    // Configurar iluminação
    setupLighting();
    
    // Limpar o array de objetos celestes
    objectCount = 0;
    
    // === Criar apenas o Sol e a Terra para a simulação gravitacional ===
    
    // Valores usados para a simulação
    double sunMass = 1.989e30;           // Massa do Sol em kg
    double earthMass = 5.972e24;         // Massa da Terra em kg
    double earthDist = 10.0;             // Distância Terra-Sol em unidades GL
    double earthOrbitalSpeed = 2.0;      // Velocidade orbital inicial da Terra (aumentada)
    
    // Sol (fixo no centro)
    addCelestialObject(
        0.0, 0.0, 0.0,             // posição
        0.0, 0.0, 0.0,             // velocidade (fixo)
        sunMass,                   // massa em kg
        2.0,                       // raio visual
        sunTexName,                // textura
        1.0f, 1.0f, 0.0f,          // cor amarela
        true                       // fixo, não se move
    );
    
    // Terra (com velocidade inicial tangencial para órbita circular)
    addCelestialObject(
        earthDist, 0.0, 0.0,       // posição inicial
        0.0, earthOrbitalSpeed, 0.0, // velocidade inicial (tangencial para órbita no eixo Y)
        earthMass,                 // massa em kg
        1.0,                       // raio visual
        earthTexName,              // textura
        0.0f, 0.5f, 1.0f,          // cor azul
        false                      // não fixo, se move pela gravidade
    );
    
    // Imprimir instruções
    printf("\n--- Controles do Sistema Solar com Gravidade ---\n");
    printf("WASD: Movimento da câmera\n");
    printf("R/G: Subir/descer\n");
    printf("Mouse: Olhar ao redor\n");
    printf("M: Alternar controle do mouse\n");
    printf("L: Alternar iluminação\n");
    printf("F: Alternar tela cheia\n");
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
        case 'l': // Alternar iluminação
        case 'L':
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
            timeStep *= 1.2;
            printf("Velocidade da simulação: %.5f\n", timeStep);
            break;
        case ',': // Diminuir velocidade da simulação
            timeStep /= 1.2;
            if (timeStep < 0.0001) timeStep = 0.0001; // Mínimo de 0.0001
            printf("Velocidade da simulação: %.5f\n", timeStep);
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