# Sistema Solar

Este é um projeto de simulação de Sistema Solar usando OpenGL, apresentando todos os planetas do sistema solar orbitando ao redor do Sol.

## Características

- Simulação visual de todos os planetas do sistema solar
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

O projeto inclui um script de compilação simples chamado `run.sh`. Para compilar e executar o programa:

1. Primeiro, dê permissão de execução ao script:
   ```bash
   chmod +x run.sh
   ```

2. Em seguida, compile e execute com:
   ```bash
   ./run.sh SistemaSolar.c
   ```

## Controles

- **W, A, S, D**: Mover a câmera horizontalmente
- **R**: Mover câmera para cima
- **G**: Mover câmera para baixo
- **M**: Alternar controle do mouse
- **L**: Alternar iluminação
- **F**: Alternar tela cheia
- **[/]**: Diminuir/aumentar largura da janela
- **-/+**: Diminuir/aumentar altura da janela
- **,/.**: Diminuir/aumentar velocidade da simulação
- **P**: Pausar/Continuar simulação
- **ESC**: Sair do programa
- **Mouse**: Olhar ao redor (quando ativado)

## Texturas

O programa utiliza texturas para todos os planetas, armazenadas na pasta `texturas/`. Estas texturas são carregadas automaticamente durante a inicialização do programa.

## Observações

Este projeto é uma simulação simplificada que prioriza o visual em vez da precisão astronômica completa. As órbitas são aproximadamente circulares e as velocidades são proporcionais, mas não são uma simulação gravitacional precisa. 