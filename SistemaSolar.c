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
static GLuint groundTexName;
#endif

// Variáveis de posição da câmera
float cameraX = 0.0f;
float cameraY = 0.0f;
float cameraZ = -3.6f;
float cameraSpeed = 0.1f;

// Variáveis de rotação da câmera
float cameraYaw = 0.0f;   // Rotação em torno do eixo Y (esquerda-direita)
float cameraPitch = 0.0f; // Rotação em torno do eixo X (cima-baixo)
int lastMouseX = -1;
int lastMouseY = -1;
float mouseSensitivity = 0.2f;

// Vetores da câmera para movimento
float forwardX, forwardY, forwardZ;  // Vetor para frente
float rightX, rightY, rightZ;        // Vetor para direita
float upX, upY, upZ;                 // Vetor para cima

// Variáveis de física
float sphereY = 4.0f;         // Altura inicial da esfera - aumentada para acomodar o raio maior
float sphereVelocity = 0.0f;  // Velocidade inicial
float gravity = 0.005f;       // Aceleração da gravidade
float groundY = -1.5f;        // Posição do chão
float restitution = 0.7f;     // Fator de quique (0-1), 1 = quique perfeito
float stopThreshold = 0.001f; // Limite de velocidade para parar de quicar
bool stopped = false;         // Se a esfera parou
float sphereRadius = 2.0f;    // Raio da esfera - adicionado como variável

// Protótipos de funções
void updateCamera();
void calculateCameraVectors();
void updatePhysics();
void drawGround();
void loadGroundTexture();

void loadTexture(const char* filename) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (data) {
        glGenTextures(1, &texName);
        glBindTexture(GL_TEXTURE_2D, texName);
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

void loadGroundTexture() {
    int width, height, nrChannels;
    unsigned char* data = stbi_load("texture_image.jpg", &width, &height, &nrChannels, 0);
    if (data) {
        glGenTextures(1, &groundTexName);
        glBindTexture(GL_TEXTURE_2D, groundTexName);
        // Configurar parâmetros de wrapping e filtragem da textura
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Carregar os dados da textura
        if (nrChannels == 3) {
            gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
        } else if (nrChannels == 4) {
            gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        stbi_image_free(data);
    } else {
        fprintf(stderr, "Falha ao carregar textura do chão\n");
    }
}

void init(void) {
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    // Carregar a textura do chão
    loadGroundTexture();
}

// Calcular vetores de direção da câmera com base em yaw e pitch
void calculateCameraVectors() {
    // Calcular o novo vetor Forward - corrigir as direções aqui
    forwardX = -sin(cameraYaw * M_PI / 180.0f) * cos(cameraPitch * M_PI / 180.0f);  // Adicionado negativo
    forwardY = sin(cameraPitch * M_PI / 180.0f);  // Removido negativo
    forwardZ = cos(cameraYaw * M_PI / 180.0f) * cos(cameraPitch * M_PI / 180.0f);  // Removido negativo
    
    // Calcular o vetor Right
    rightX = -cos(cameraYaw * M_PI / 180.0f);  // Adicionado negativo
    rightY = 0.0f;
    rightZ = -sin(cameraYaw * M_PI / 180.0f);  // Adicionado negativo
    
    // Calcular o vetor Up usando produto vetorial
    upX = forwardY * rightZ - forwardZ * rightY;  // Fórmula alterada
    upY = forwardZ * rightX - forwardX * rightZ;  // Fórmula alterada
    upZ = forwardX * rightY - forwardY * rightX;  // Fórmula alterada
    
    // Normalizar vetores (não é estritamente necessário para este caso simples)
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

// Atualizar simulação física para a esfera
void updatePhysics() {
    if (!stopped) {
        // Aplicar gravidade
        sphereVelocity -= gravity;
        
        // Atualizar posição
        sphereY += sphereVelocity;
        
        // Verificar colisão com o chão
        if (sphereY - sphereRadius <= groundY) { // Usar sphereRadius em vez de 1.0f codificado
            // Colisão detectada, quicar
            sphereY = groundY + sphereRadius; // Colocar esfera no chão com raio correto
            
            // Aplicar quique (inverter velocidade com amortecimento)
            sphereVelocity = -sphereVelocity * restitution;
            
            // Verificar se a esfera deve parar de quicar
            if (fabs(sphereVelocity) < stopThreshold) {
                sphereVelocity = 0.0f;
                sphereY = groundY + sphereRadius; // Usar sphereRadius
                stopped = true;
            }
        }
    }
}

void drawGround() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, groundTexName);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    glColor3f(0.8f, 0.8f, 0.8f); // Chão cinza claro
    
    // Desenhar plano do chão (quad grande)
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-10.0f, groundY, -10.0f);
    glTexCoord2f(10.0f, 0.0f); glVertex3f(-10.0f, groundY, 10.0f);
    glTexCoord2f(10.0f, 10.0f); glVertex3f(10.0f, groundY, 10.0f);
    glTexCoord2f(0.0f, 10.0f); glVertex3f(10.0f, groundY, -10.0f);
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
}

void display(void) {
    // Atualizar física
    updatePhysics();
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Desenhar chão
    drawGround();
    
    // Desenhar esfera
    glEnable(GL_TEXTURE_2D);
    loadTexture("texture_image.jpg");
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor3f(1.0, 0.0, 0.0);
    
    // Posicionar e desenhar a esfera
    glPushMatrix();
    glTranslatef(0.0f, sphereY, 0.0f);
    GLUquadric* quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);
    gluSphere(quad, sphereRadius, 32, 32);
    gluDeleteQuadric(quad);
    glPopMatrix();
    
    glFlush();
    glDisable(GL_TEXTURE_2D);
    
    // Solicitar redesenho para animação
    glutPostRedisplay();
}

void reshape(int w, int h) {
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat)w / (GLfloat)h, 1.0, 30.0);
    
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
        case 'f': // Mover para baixo em relação ao mundo
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
        case ' ': // Resetar física da esfera quando a barra de espaço é pressionada
            sphereY = 4.0f; // Aumentado para acomodar o raio maior
            sphereVelocity = 0.0f;
            stopped = false;
            break;
        default:
            break;
    }
}

void mouseMotion(int x, int y) {
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
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600); // Janela maior
    glutInitWindowPosition(100, 100);
    glutCreateWindow(argv[0]);
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMotionFunc(mouseMotion);     // Rastrear movimento do mouse enquanto botão pressionado
    glutPassiveMotionFunc(mouseMotion); // Rastrear movimento do mouse quando nenhum botão pressionado
    glutEntryFunc(mouseEntry);       // Rastrear quando o mouse entra/sai da janela
    glutMainLoop();
    return 0;
}
