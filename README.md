# Sistema Solar

Este é um projeto de simulação de Sistema Solar usando OpenGL, apresentando dois modos de simulação:
1. Simulação tradicional com todos os planetas do sistema solar orbitando ao redor do Sol
2. Simulação gravitacional física entre o Sol e a Terra

## Características

- Simulação visual de todos os planetas do sistema solar
- Implementação de física real com cálculos gravitacionais (no modo gravitacional)
- Texturas realistas obtidas de imagens da NASA
- Velocidades orbitais proporcionais às reais
- Iluminação realista vinda do Sol
- Controles interativos de câmera e velocidade de simulação

## Requisitos

Para compilar e executar este projeto, você precisa:

- Compilador GCC
- Bibliotecas de desenvolvimento OpenGL
- GLUT (OpenGL Utility Toolkit)
- Biblioteca STB Image (já incluída no projeto)

## Instalação de Dependências

Em sistemas baseados em Ubuntu/Debian, instale as dependências necessárias com:

```bash
sudo apt-get update
sudo apt-get install build-essential libgl1-mesa-dev freeglut3-dev
```

## Compilação e Execução

O projeto inclui dois scripts de compilação:

### Simulação Tradicional
```bash
chmod +x run.sh
./run.sh SistemaSolar.c
```

### Simulação Gravitacional
```bash
chmod +x run_gravity.sh
./run_gravity.sh
```

## Controles

- **W, A, S, D**: Mover a câmera horizontalmente
- **R**: Mover câmera para cima
- **G**: Mover câmera para baixo
- **M**: Alternar controle do mouse
- **L**: Alternar iluminação
- **F**: Alternar tela cheia
- **[/]**: Diminuir/aumentar largura da janela (apenas no modo tradicional)
- **-/+**: Diminuir/aumentar altura da janela (apenas no modo tradicional)
- **,/.**: Diminuir/aumentar velocidade da simulação
- **P**: Pausar/Continuar simulação
- **ESC**: Sair do programa
- **Mouse**: Olhar ao redor (quando ativado)

## Texturas

O programa utiliza texturas para todos os planetas, armazenadas na pasta `texturas/`. Estas texturas são carregadas automaticamente durante a inicialização do programa.

## Modos de Simulação

### Simulação Tradicional
Este modo apresenta todos os planetas do sistema solar com órbitas pré-definidas, oferecendo uma visualização esteticamente agradável de todo o sistema solar.

### Simulação Gravitacional
Este modo implementa cálculos gravitacionais reais entre o Sol e a Terra, demonstrando como a atração gravitacional cria uma órbita elíptica. A força gravitacional e outros parâmetros foram ajustados para criar uma visualização visualmente adequada, mantendo o princípio físico da Lei da Gravitação Universal de Newton.

## Observações

A simulação gravitacional demonstra princípios físicos reais, enquanto a simulação tradicional prioriza o visual em vez da precisão astronômica completa. As órbitas na simulação tradicional são aproximadamente circulares e as velocidades são proporcionais, mas não seguem cálculos gravitacionais precisos como na simulação gravitacional. 