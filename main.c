#include <stdio.h>
#include <stdlib.h>
#include <string.h>        // Para usar strings
#include <unistd.h>

#ifdef WIN32
#include <windows.h>    // Apenas para Windows
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>     // Funções da OpenGL
#include <GL/glu.h>    // Funções da GLU
#include <GL/glut.h>   // Funções da FreeGLUT
#endif

// SOIL é a biblioteca para leitura das imagens
#include "SOIL.h"

// Um pixel RGB (24 bits)
typedef struct {
    unsigned char r, g, b;
} RGB;

// Uma imagem RGB
typedef struct {
    int width, height;
    RGB* img;
} Img;

//A energia de um pixel
typedef struct{
    int energy;//energia em si
    int iCameFromWidth;//de qual pixel veio a energia acumulada
} Energy;
// Protótipos
void load(char* name, Img* pic);
void uploadTexture();
int energy(int i);
void tiraUmaLinha(int widt);


// Funções da interface gráfica e OpenGL
void init();
void draw();
void keyboard(unsigned char key, int x, int y);

// Largura e altura da janela
int width, height;

// Identificadores de textura
GLuint tex[3];

// As 3 imagens
Img pic[3];

// Imagem selecionada (0,1,2)
int sel;

// Carrega uma imagem para a struct Img
void load(char* name, Img* pic)
{
    int chan;
    pic->img = (RGB*) SOIL_load_image(name, &pic->width, &pic->height, &chan, SOIL_LOAD_RGB);
    if(!pic->img)
    {
        printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
        exit(1);
    }
    printf("Load: %d x %d x %d\n", pic->width, pic->height, chan);
}

int main(int argc, char** argv)
{
    if(argc < 2) {
        printf("seamcarving [origem] [mascara]\n");
        printf("Origem é a imagem original, mascara é a máscara desejada\n");
        exit(1);
    }
    glutInit(&argc,argv);

    // Define do modo de operacao da GLUT
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    // pic[0] -> imagem original
    // pic[1] -> máscara desejada
    // pic[2] -> resultado do algoritmo

    // Carrega as duas imagens
    load(argv[1], &pic[0]);
    load(argv[2], &pic[1]);

    if(pic[0].width != pic[1].width || pic[0].height != pic[1].height) {
        printf("Imagem e máscara com dimensões diferentes!\n");
        exit(1);
    }
    // A largura e altura da janela são calculadas de acordo com a maior
    // dimensão de cada imagem
    width = pic[0].width;
    height = pic[0].height;

    // A largura e altura da imagem de saída são iguais às da imagem original (1)
    pic[2].width  = pic[1].width;
    pic[2].height = pic[1].height;

    // Especifica o tamanho inicial em pixels da janela GLUT
    glutInitWindowSize(width, height);

    // Cria a janela passando como argumento o titulo da mesma
    glutCreateWindow("Seam Carving");

    // Registra a funcao callback de redesenho da janela de visualizacao
    glutDisplayFunc(draw);

    // Registra a funcao callback para tratamento das teclas ASCII
    glutKeyboardFunc (keyboard);

    // Cria texturas em memória a partir dos pixels das imagens
    tex[0] = SOIL_create_OGL_texture((unsigned char*) pic[0].img, pic[0].width, pic[0].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
    tex[1] = SOIL_create_OGL_texture((unsigned char*) pic[1].img, pic[1].width, pic[1].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    // Exibe as dimensões na tela, para conferência
    printf("Origem  : %s %d x %d\n", argv[1], pic[0].width, pic[0].height);
    printf("Destino : %s %d x %d\n", argv[2], pic[1].width, pic[0].height);
    sel = 0; // pic1

    // Define a janela de visualizacao 2D
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0.0,width,height,0.0);
    glMatrixMode(GL_MODELVIEW);

    // Aloca memória para a imagem de saída
    pic[2].img = malloc(pic[1].width * pic[1].height * 3); // W x H x 3 bytes (RGB)
    // Pinta a imagem resultante de preto!
    memset(pic[2].img, 0, width*height*3);

    // Cria textura para a imagem de saída
    tex[2] = SOIL_create_OGL_texture((unsigned char*) pic[2].img, pic[2].width, pic[2].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
    for(int i = 0;i<pic[2].height*pic[2].width;i++){
        pic[2].img[i]=pic[0].img[i];
    }
    uploadTexture();
    // Entra no loop de eventos, não retorna
    glutMainLoop();
}

int energy(int i){
    int rx = pic[2].img[i+1].r - pic[2].img[i-1].r;//rPixelDireita-rPixelEsquerda
    int gx = pic[2].img[i+1].g - pic[2].img[i-1].g;//gPixelDireita-gPixelEsquerda
    int bx = pic[2].img[i+1].b - pic[2].img[i-1].b;//bPixelDireita-bPixelEsquerda

    int deltax = (rx*rx) + (gx*gx) + (bx*bx);

    int ry = pic[2].img[i-width].r - pic[2].img[i+width].r;//rPixelCima-rPixelBaixo
    int gy = pic[2].img[i-width].g - pic[2].img[i+width].g;//gPixelCima-gPixelBaixo
    int by = pic[2].img[i-width].b - pic[2].img[i+width].b;//bPixelCima-bPixelBaixo

    int deltay = (ry*ry) + (gy*gy) + (by*by);
    return (deltax + deltay);
}

void tiraUmaLinha(int widt){
    width=widt;
    height=pic[2].height;
    Energy energies[height][width];
    //preenche a matriz de energia com as energias dos pixeis do meio
    for(int i = 1;i<height-1;i++){
        for(int j = 1;j<width-1;j++){
            energies[i][j].energy=energy((i*pic[0].width)+j);//altura atual(i)*largura + largura atual(j) = pixel no vetor
        }
    }
    //preenche os pixeis da borda com 0
    for(int i = 0; i<width;i++){
        energies[0][i].energy = 0;
        energies[height-1][i].energy = 0;
    }
    for(int i = 0;i<height;i++){
        energies[i][0].energy = 0;
        energies[i][width-1].energy = 0;
    }
    //utiliza mascara nas energias
    for(int h = 0;h<height;h++){
        for(int w = 0;w<width;w++){
            unsigned char r1 = pic[1].img[(h*pic[0].width)+w].r;
            unsigned char g1 = pic[1].img[(h*pic[0].width)+w].g;
            unsigned char b1 = pic[1].img[(h*pic[0].width)+w].b;
            /*
            OBSERVACOES DAS MASCARAS (ESTAS OBSERVACOES PODEM ESTAR INCORRETAS)

            TIVE QUE BOTAR B1<50, PRINTEI A MASCARA COM OS PIXEIS PINTADOS DE VERMELHO
            QUANDO ENTRAM NO IF VERMELHO E DE VERDE QUANDO ENTRAM NO IF DE VERDE, ALGUNS 
            PIXEIS FICAVAM COM A COR ERRADA, EM VOLTA DOS FOCOS DE COR NÃO BRANCA, AO 
            ADICIONAR O B1<50 A MASCARA PRINTADA FICOU IGUAL A MASCARA ORIGINAL.

            TAMBEM CONSTATEI QUE AS MASCARAS FORNECIDAS PARA TESTES TEM PEQUENAS MARGENS DE ERRO
            PORQUE A GIRAFA NAO FICOU COMPLETAMENTE PINTADA DE VERMELHO, ENTAO SOBRA UM PEDACINHO
            DA COLUNA DELA NA IMAGEM COM OS PIXEIS REMOVIDOS. E NO PASSARO O RABO TAMBEM NAO ESTA 
            COMPLETAMENTE PINTADO
            */
            if(r1>g1 && b1<100){//é vermelho
                energies[h][w].energy=-999999;
                pic[1].img[(h*pic[0].width)+w].r=255;
                pic[1].img[(h*pic[0].width)+w].g=0;
                pic[1].img[(h*pic[0].width)+w].b=0;
            }
            else if(g1>r1 && b1<100){//é verde
                energies[h][w].energy=999999;//390150 eh o maior valor possivel da formula de energia           
                pic[1].img[(h*pic[0].width)+w].r=0;
                pic[1].img[(h*pic[0].width)+w].g=255;
                pic[1].img[(h*pic[0].width)+w].b=0;
            }
            else{//nao faz parte da mascara, entao diminui pra dar mais destaque para o verde
                energies[h][w].energy-=100000;
            }
        }
    }
    //calcula linhas de energias
    for(int h=1;h<height;h++){//começa em 1 porque a primeira linha nao tem energia acumulada, ja que eh a primeira
        for(int w=2;w<width-2;w++){//de 2 a width-2 para não pegar as energias 0 das bordas
            int energia = energies[h][w].energy;
            int energiaEsquerda= energia + energies[h-1][w-1].energy;
            int energiaMeio = energia + energies[h-1][w].energy;
            int energiaDireita = energia + energies[h-1][w+1].energy;
            if((energiaMeio<=energiaEsquerda) && (energiaMeio<=energiaDireita)){//meio eh menor ou se é igual a alguma empata como menor e eh escolhida
                energies[h][w].energy=energiaMeio;
                energies[h][w].iCameFromWidth=w;
            }
            else if(energiaDireita<=energiaEsquerda){//direita eh menor ou empata com esquerda em menor e eh escolhida
                energies[h][w].energy=energiaDireita;
                energies[h][w].iCameFromWidth=w+1;
            }
            else{//esquerda é menor sem empate
                energies[h][w].energy=energiaEsquerda;
                energies[h][w].iCameFromWidth=w-1;
            }
        }
    }
    //acha menor energia acumulada na ultima linha
    int menorIndice=2;
    int menorEnergia=energies[height-1][2].energy;
    for(int i = 2;i<width-2;i++){//lembrando que vai de 2 a width-2 pra nao pegar as bordas zeradas
        int atual = energies[height-1][i].energy;
        if(atual<menorEnergia){
            menorEnergia=atual;
            menorIndice=i;
        }
    }
    //remover linha
    int widthAtual=menorIndice;//width na matriz em que o pixel removido esta
    int pixelRemovido;
    for(int h=height-1;h>=0;h--){
        pixelRemovido=(h*pic[0].width)+widthAtual;
        pic[2].img[pixelRemovido].r=255;
        pic[2].img[pixelRemovido].g=0;
        pic[2].img[pixelRemovido].b=0;
        
        for(int i=pixelRemovido;i<(h*pic[0].width)+width;i++){
            //levando o pixel removido até o fim da linha para ignorar nas proximas execucoes com width--
            RGB aux = pic[2].img[i];
            pic[2].img[i]=pic[2].img[i+1];
            pic[2].img[i+1]=aux;
            //levando o pixel removido da mascara até o fim da linha para ignorar nas proximas execucoes com width--
            RGB aux2 = pic[1].img[i];
            pic[1].img[i]=pic[1].img[i+1];
            pic[1].img[i+1]=aux2;
        }
        
        widthAtual=energies[h][widthAtual].iCameFromWidth;
    }
    uploadTexture();
}

// Gerencia eventos de teclado
void keyboard(unsigned char key, int x, int y)
{
    if(key==27) {
      // ESC: libera memória e finaliza
      free(pic[0].img);
      free(pic[1].img);
      free(pic[2].img);
      exit(1);
    }
    if(key >= '1' && key <= '3')
        // 1-3: seleciona a imagem correspondente (origem, máscara e resultado)
        sel = key - '1';
    if(key == 's') {
        // Aplica o algoritmo e gera a saida em pic[2].img...
        // ...
        // ... (crie uma função para isso!)
        int LARGURA_A_REMOVER = 50;
        for(int i = 1;i<=LARGURA_A_REMOVER;i++){
            tiraUmaLinha(--width);
        }
        // Chame uploadTexture a cada vez que mudar
        // a imagem (pic[2])*/
    }
    glutPostRedisplay();
}

// Faz upload da imagem para a textura,
// de forma a exibi-la na tela
void uploadTexture()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
        pic[2].width, pic[2].height, 0,
        GL_RGB, GL_UNSIGNED_BYTE, pic[2].img);
    glDisable(GL_TEXTURE_2D);
}

// Callback de redesenho da tela
void draw()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Preto
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // Para outras cores, veja exemplos em /etc/X11/rgb.txt

    glColor3ub(255, 255, 255);  // branco

    // Ativa a textura corresponde à imagem desejada
    glBindTexture(GL_TEXTURE_2D, tex[sel]);
    // E desenha um retângulo que ocupa toda a tela
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);

    glTexCoord2f(0,0);
    glVertex2f(0,0);

    glTexCoord2f(1,0);
    glVertex2f(pic[sel].width,0);

    glTexCoord2f(1,1);
    glVertex2f(pic[sel].width, pic[sel].height);

    glTexCoord2f(0,1);
    glVertex2f(0,pic[sel].height);

    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Exibe a imagem
    glutSwapBuffers();
}
