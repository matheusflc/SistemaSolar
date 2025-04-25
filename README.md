# Sistema Solar

Este é um projeto de simulação de Sistema Solar usando OpenGL.

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

- Use as teclas W, A, S, D para mover a câmera
- R: Mover para cima
- F: Mover para baixo
- Barra de espaço: Resetar a simulação física
- ESC: Sair do programa
- Movimentos do mouse: Olhar ao redor

## Observações

Certifique-se de que o arquivo de textura `texture_image.jpg` esteja presente no diretório do projeto para que as texturas sejam carregadas corretamente. 