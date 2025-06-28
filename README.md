# Jogo Isométrico - Coleta de Moedas

## Descrição

Este é um jogo isométrico simples em C++ com OpenGL, onde o objetivo é **coletar todas as moedas espalhadas pelo mapa** sem pisar na lava. O personagem pode se mover em **8 direções** e interagir com diferentes tipos de terrenos.

---

## Como Jogar

- Use as teclas para mover o personagem:
  - `W`, `A`, `S`, `D`: cima, esquerda, baixo, direita
  - `Q`, `E`, `Z`, `X`: diagonais (noroeste, nordeste, sudoeste, sudeste)
- O jogo termina quando:
  - Você **coleta todas as moedas** → Vitória 🎉
  - Você **pisa em lava** → Derrota 💀
- O **tile rosa** se transforma em **terra** quando pisado.

---

## Tipos de Tile

| Tile Index | Aparência | Comportamento                     |
| ---------- | --------- | --------------------------------- |
| 0          | Terra     | Caminhável                        |
| 1          | Lava      | Caminhável, mas letal (game over) |
| 6          | Rosa      | Caminhável, vira terra ao pisar   |
| Outros     | Ignorados | Não renderizados                  |

---

## Arquivo `map.txt`

O arquivo `map.txt` define o mapa do jogo, incluindo:

1. Caminho do tileset
2. Quantidade de tiles no tileset + dimensões de cada tile
3. Dimensões do mapa (largura x altura)
4. Matriz de **tiles** (índices dos tiles por linha)
5. Matriz de **itens** (0 = vazio, 1 = moeda)

### Exemplo de `map.txt`

tilesetIso.png
7 64 32
15 15
2 2 2 2 2 2 6 6 6 2 2 2 2 2 2
2 2 2 2 6 2 2 2 2 2 6 2 2 2 2
2 2 2 2 2 6 4 4 4 6 2 2 2 2 2
2 2 2 6 4 2 2 6 2 2 2 4 6 2 2
2 2 6 4 2 2 6 2 6 2 2 2 4 6 2
2 6 4 2 2 6 2 2 2 2 6 2 2 4 6
6 4 2 2 6 2 2 6 2 2 2 2 2 2 4
2 2 2 6 2 2 2 2 6 2 2 2 2 2 2
2 2 2 2 6 2 2 2 2 6 2 2 2 2 2
2 2 4 2 2 6 2 2 2 2 6 2 2 4 2
2 2 2 4 2 2 6 2 2 2 2 6 4 2 2
2 2 2 2 4 2 2 6 2 2 2 4 2 2 2
2 2 2 2 2 4 4 4 6 2 2 2 2 2 2
2 2 2 2 2 2 6 2 2 6 2 2 2 2 2
2 2 2 2 2 2 2 2 6 2 2 2 2 2 2
0 0 0 0 0 0 0 1 0 0 0 0 0 0 0
0 0 0 0 0 1 0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0 1 0 0 0 0 0
0 0 0 0 1 0 0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 1 0 0 0 0 0 0
0 0 0 1 0 0 0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 1 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0 0 1 0 0 0 0
0 0 0 0 0 0 0 0 0 0 0 1 0 0 0
0 0 0 0 0 0 0 0 0 0 0 0 1 0 0
0 0 0 0 0 0 0 0 0 0 0 0 0 1 0
0 0 0 0 0 0 0 0 0 0 0 0 0 0 1
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0

- O índice `2` será tratado como **terra (tile 0)**
- O índice `4` será tratado como **lava (tile 1)**
- O índice `6` será tratado como **rosa (transforma-se em terra após pisar)**

## Compilação e Execução

Certifique-se de ter as bibliotecas necessárias (GLFW, GLAD, stb_image, GLM) configuradas corretamente.

```bash
g++ main.cpp -o jogo -lglfw -lGL -ldl -lX11 -lpthread
./jogo
```
