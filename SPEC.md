# Especificação da Implementação

## Integrantes da dupla

- **Aluno 1 - Nome**: Elias Furtado Helfer
- **Aluno 1 - Cartão UFRGS**: 00577752

- **Aluno 2 - Nome**: João Vítor Leffa Lummertz
- **Aluno 2 - Cartão UFRGS**: 00577893

## Detalhes do que será implementado

- **Título do trabalho**: Meia Vida 3
- **Parágrafo curto descrevendo o que será implementado**: Uma recriação da introdução do jogo  *Half-Life Source*, mostrando as cenas iniciais e primeiras interações do jogador com o mapa e controles, da chegada do protagonista ao laboratório até o acidente que desencadeia o resto da história do jogo.

## Especificação visual

### Vídeo - Link

https://www.youtube.com/watch?v=yTY85Per-8U

### Vídeo - Timestamp

- **Timestamp inicial**: 15:37
- **Timestamp final**: 16:00

### Imagens

> [!IMPORTANT]
> - Coloque aqui **três imagens** capturadas do vídeo acima, que você
>   irá usar como ilustração para as explicações que vêm abaixo.

<mark>`<preencher>`</mark>

## Especificação textual

Para cada um dos requisitos abaixo (detalhados no [Enunciado do Trabalho final - Moodle](https://moodle.ufrgs.br/mod/assign/view.php?id=6018620)), escreva um parágrafo **curto** explicando como este requisito será atendido, apontando itens específicos do vídeo/imagens que você incluiu acima que atendem estes requisitos.

### Malhas poligonais complexas
Mapa, NPC's, crowbar e objetos decorativos.

### Transformações geométricas controladas pelo usuário
Além do controle da câmera, uso do crowbar para quebrar vidro, ação de empurrar um carrinho no experimento, e interação com o elevador.

### Diferentes tipos de câmeras
Câmera em primeira pessoa e look-at centrada no jogador.

### Instâncias de objetos
NPC's cientistas, portas e computadores.

### Testes de intersecção
Teste de intersecção com paredes e objetos decorativos, intersecção da arma do jogador com vidros quebráveis, e intersecção do jogador com o carrinho empurrável.

### Modelos de Iluminação em todos os objetos
Modelo de iluminação de Phong utilizado em todos os objetos.

### Mapeamento de texturas em todos os objetos
Tem texturas.

### Movimentação com curva Bézier cúbica
Movimentação do bonde que carrega o jogador no inicio do jogo.

### Animações baseadas no tempo ($\Delta t$)
Movimentação do jogador e NPC's.

## Limitações esperadas
Não iremos implementar:
- Inimigos;
- Partículas;
