#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

#include <cmath>
#include <list>
#include <complex>
#include <vector>

using namespace std;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
void iluminacion(Shader& shader);

// Tamaño de la pantalla
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 1000;

// CÁMARA
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;


// TIEMPO
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// ILUMINACIÓN
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
bool circuloLampara = true;
int estadoAnt = GLFW_RELEASE;
// posiciones de las lámparas
glm::vec3 pointLightPositions[] = {
    glm::vec3(0.7f,  0.2f,  2.0f),
    glm::vec3(2.3f, -3.3f, -4.0f),
    glm::vec3(-4.0f,  2.0f, -12.0f),
    glm::vec3(0.0f,  0.0f, -3.0f)
};
// colores de las lámparas
glm::vec3 pointLightColors[] = {
    glm::vec3(236.0 / 255.0, 239.0 / 255.0, 248.0 / 255.0),
    glm::vec3(157.0 / 255.0, 190.0 / 255.0, 212.0 / 255.0),
    glm::vec3(209.0 / 255.0, 216.0 / 255.0, 226.0 / 255.0),
    glm::vec3(79.0 / 255.0, 156.0 / 255.0, 200.0 / 255.0)
};

// Declaración de constantes a utilizar a lo largo de este programa
const int IMAGE_SIZE_X = SCR_WIDTH;
const int IMAGE_SIZE_Y = SCR_HEIGHT;
const int NUM_SUBDIVISONES = 7;
const double goldenRatio = (1 + sqrt(5)) / 2;
const double pi = 3.1415926535897932384626433832795028841971;

// Estructura que guarda la información de los triángulos a dibujar
struct triangulo {
    int color;
    complex<double> A;
    complex<double> B;
    complex<double> C;
};

// Método para subdividir todos los triángulos de manera que el resultante sea un
// tipo de estructura como la de Penrose. Recibe una lista de apuntadores a los
// triángulos ya creados y regresa otra lista con la subdivisión de los triángulos.
list<struct triangulo*> subdividir(list<struct triangulo*> triangulos) {
    list<struct triangulo*> resultado = {};
    list<struct triangulo*> ::iterator it;
    // Iteramos sobre la lista que nos pasaron como argumento
    for (it = triangulos.begin(); it != triangulos.end(); ++it) {
        // Subdividimos al triángulo según el tipo de color que tenga
        if ((*it)->color == 0) {
            complex<double> P;
            struct triangulo* t1 = (struct triangulo*)malloc(sizeof(struct triangulo));
            struct triangulo* t2 = (struct triangulo*)malloc(sizeof(struct triangulo));
            P = (*it)->A + ((*it)->B - (*it)->A) / goldenRatio;

            t1->color = 0;
            t1->A = (*it)->C;
            t1->B = P;
            t1->C = (*it)->B;

            t2->color = 1;
            t2->A = P;
            t2->B = (*it)->C;
            t2->C = (*it)->A;

            resultado.push_back(t1);
            resultado.push_back(t2);
        }
        else {
            complex<double> Q;
            complex<double> R;
            struct triangulo* t1 = (struct triangulo*)malloc(sizeof(struct triangulo));
            struct triangulo* t2 = (struct triangulo*)malloc(sizeof(struct triangulo));
            struct triangulo* t3 = (struct triangulo*)malloc(sizeof(struct triangulo));

            Q = (*it)->B + ((*it)->A - (*it)->B) / goldenRatio;
            R = (*it)->B + ((*it)->C - (*it)->B) / goldenRatio;

            t1->color = 1;
            t1->A = R;
            t1->B = (*it)->C;
            t1->C = (*it)->A;

            t2->color = 1;
            t2->A = Q;
            t2->B = R;
            t2->C = (*it)->B;

            t3->color = 0;
            t3->A = R;
            t3->B = Q;
            t3->C = (*it)->A;

            resultado.push_back(t1);
            resultado.push_back(t2);
            resultado.push_back(t3);
        }
    }
    return resultado;
}

int main()
{
    // #########################################################################################  TESELACIÓN DE PENROSE    
    list<struct triangulo*> triangulos = {};
    complex<double> A(0, 0);

    for (int j = 0; j < 10; j++) {
        struct triangulo* t = (struct triangulo*)malloc(sizeof(struct triangulo));
        t->color = 0;
        t->A = A;
        t->B = polar(1.0, ((2 * j - 1) * pi) / 10.0);
        t->C = polar(1.0, ((2 * j + 1) * pi) / 10.0);
        if (j % 2 == 0) {
            complex<double> aux = t->B;
            t->B = t->C;
            t->C = aux;
        }
        triangulos.push_back(t);        
    }

    // Subdividimos los triángulos las veces que indique la constante NUM_SUBDIVISIONES.
    for (int j = 0; j < NUM_SUBDIVISONES; j++) {
        triangulos = subdividir(triangulos);
    }

    // Guardamos los triángulos en formato de vértices para OpenGL.
    list<struct triangulo*> ::iterator it;
    vector<float> vert_ceros;
    vector<float> vert_unos;    
    for (it = triangulos.begin(); it != triangulos.end(); ++it) {
        if ((*it)->color == 0) {    
            // Parte delantera
            vert_ceros.push_back((float)(*it)->A.real()); vert_ceros.push_back((float)(*it)->A.imag()); vert_ceros.push_back(0.5f); // Posición            
            vert_ceros.push_back(0.0f); vert_ceros.push_back(0.0f); vert_ceros.push_back(1.0f);                                     // Vector normal
            vert_ceros.push_back((float)(*it)->B.real()); vert_ceros.push_back((float)(*it)->B.imag()); vert_ceros.push_back(0.5f); // Posición           
            vert_ceros.push_back(0.0f); vert_ceros.push_back(0.0f); vert_ceros.push_back(1.0f);                                     // Vector normal
            vert_ceros.push_back((float)(*it)->C.real()); vert_ceros.push_back((float)(*it)->C.imag()); vert_ceros.push_back(0.5f); // Posición           
            vert_ceros.push_back(0.0f); vert_ceros.push_back(0.0f); vert_ceros.push_back(1.0f);                                     // Vector normal

            // Parte trasera
            vert_ceros.push_back((float)(*it)->A.real()); vert_ceros.push_back((float)(*it)->A.imag()); vert_ceros.push_back(-0.5f); // Posición           
            vert_ceros.push_back(0.0f); vert_ceros.push_back(0.0f); vert_ceros.push_back(-1.0f);                                     // Vector normal
            vert_ceros.push_back((float)(*it)->B.real()); vert_ceros.push_back((float)(*it)->B.imag()); vert_ceros.push_back(-0.5f); // Posición            
            vert_ceros.push_back(0.0f); vert_ceros.push_back(0.0f); vert_ceros.push_back(-1.0f);                                     // Vector normal
            vert_ceros.push_back((float)(*it)->C.real()); vert_ceros.push_back((float)(*it)->C.imag()); vert_ceros.push_back(-0.5f); // Posición
            vert_ceros.push_back(0.0f); vert_ceros.push_back(0.0f); vert_ceros.push_back(-1.0f);                                     // Vector normal
        }
        else {            
            // Parte delantera
            vert_unos.push_back((float)(*it)->A.real()); vert_unos.push_back((float)(*it)->A.imag()); vert_unos.push_back(0.5f); // Posición            
            vert_unos.push_back(0.0f); vert_unos.push_back(0.0f); vert_unos.push_back(1.0f);                                     // Vector normal
            vert_unos.push_back((float)(*it)->B.real()); vert_unos.push_back((float)(*it)->B.imag()); vert_unos.push_back(0.5f); // Posición           
            vert_unos.push_back(0.0f); vert_unos.push_back(0.0f); vert_unos.push_back(1.0f);                                     // Vector normal
            vert_unos.push_back((float)(*it)->C.real()); vert_unos.push_back((float)(*it)->C.imag()); vert_unos.push_back(0.5f); // Posición           
            vert_unos.push_back(0.0f); vert_unos.push_back(0.0f); vert_unos.push_back(1.0f);                                     // Vector normal

            // Parte trasera
            vert_unos.push_back((float)(*it)->A.real()); vert_unos.push_back((float)(*it)->A.imag()); vert_unos.push_back(-0.5f); // Posición           
            vert_unos.push_back(0.0f); vert_unos.push_back(0.0f); vert_unos.push_back(-1.0f);                                     // Vector normal
            vert_unos.push_back((float)(*it)->B.real()); vert_unos.push_back((float)(*it)->B.imag()); vert_unos.push_back(-0.5f); // Posición            
            vert_unos.push_back(0.0f); vert_unos.push_back(0.0f); vert_unos.push_back(-1.0f);                                     // Vector normal
            vert_unos.push_back((float)(*it)->C.real()); vert_unos.push_back((float)(*it)->C.imag()); vert_unos.push_back(-0.5f); // Posición
            vert_unos.push_back(0.0f); vert_unos.push_back(0.0f); vert_unos.push_back(-1.0f);                                     // Vector normal
        }
    }    

    // #########################################################################################  MENÚ DE USUARIO
    cout << "       MANUAL DE USUARIO\n";
    cout << "-------------------------------------\n";
    cout << "Para controlar la cámara, utilice:\n";
    cout << "   -W: adentro\n";
    cout << "   -A: izquierda\n";
    cout << "   -S: afuera\n";
    cout << "   -D: derecha\n";
    cout << "\nAlternativamente, puede mover la bola del ratón hacia \narriba en lugar de W o hacia abajo en lugar de S.\n";

    cout << "\nPara controlar la lámpara, utilice:\n";
    cout << "   -Up arrow: arriba\n";
    cout << "   -Left arrow: izquierda\n";
    cout << "   -Down arrow: abajo\n";
    cout << "   -Right arrow: derecha\n";
    cout << "   -RePág: adentro\n";
    cout << "   -AvPág: afuera\n";

    cout << "\nPor último, use Espacio para pausar la luz en movimiento.";
       
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: creación de la ventana  
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Proyecto 2", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }    

    // voltear la textura en el eje-y antes de cargar el modelo
    stbi_set_flip_vertically_on_load(true);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // construimos y compilamos los shaders    
    Shader ourShader("shaders/proyecto2.vs", "shaders/proyecto2.fs");
    Shader lightCubeShader("shaders/light_cube.vs", "shaders/light_cube.fs");
    Shader shader_blend("shaders/blending.vs", "shaders/blending.fs");
    Shader mochilaShader("shaders/multiple_lights.vs", "shaders/multiple_lights.fs");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };
    float verticesOrilla[] = {
         // Vértices                            // Vector normal
         0.9510565163,  0.3090169944,  0.5,     1.0,            0.0,            0.0, //A
         0.9510565163,  0.3090169944, -0.5,     1.0,            0.0,            0.0, //B
         0.9510565163, -0.3090169944,  0.5,     1.0,            0.0,            0.0, //C
         0.9510565163,  0.3090169944, -0.5,     1.0,            0.0,            0.0, //B
         0.9510565163, -0.3090169944,  0.5,     1.0,            0.0,            0.0, //C
         0.9510565163, -0.3090169944, -0.5,     1.0,            0.0,            0.0, //D

         0.5877852523,  0.8090169944,  0.5,     0.809016994375, 0.587785252292, 0.0, //E
         0.5877852523,  0.8090169944, -0.5,     0.809016994375, 0.587785252292, 0.0, //F
         0.9510565163,  0.3090169944,  0.5,     0.809016994375, 0.587785252292, 0.0, //A
         0.5877852523,  0.8090169944, -0.5,     0.809016994375, 0.587785252292, 0.0, //F
         0.9510565163,  0.3090169944,  0.5,     0.809016994375, 0.587785252292, 0.0, //A
         0.9510565163,  0.3090169944, -0.5,     0.809016994375, 0.587785252292, 0.0, //B

         0.0000000000,  1.0000000000,  0.5,     0.309016994375, 0.951056516295, 0.0, //G
         0.0000000000,  1.0000000000, -0.5,     0.309016994375, 0.951056516295, 0.0,//H
         0.5877852523,  0.8090169944,  0.5,     0.309016994375, 0.951056516295, 0.0, //E
         0.0000000000,  1.0000000000, -0.5,     0.309016994375, 0.951056516295, 0.0, //H
         0.5877852523,  0.8090169944,  0.5,     0.309016994375, 0.951056516295, 0.0, //E
         0.5877852523,  0.8090169944, -0.5,     0.309016994375, 0.951056516295, 0.0, //F

        -0.5877852523,  0.8090169944,  0.5,    -0.309016994375, 0.951056516295, 0.0, //I
        -0.5877852523,  0.8090169944, -0.5,    -0.309016994375, 0.951056516295, 0.0, //J
         0.0000000000,  1.0000000000,  0.5,    -0.309016994375, 0.951056516295, 0.0, //G
        -0.5877852523,  0.8090169944, -0.5,    -0.309016994375, 0.951056516295, 0.0, //J
         0.0000000000,  1.0000000000,  0.5,    -0.309016994375, 0.951056516295, 0.0, //G
         0.0000000000,  1.0000000000, -0.5,    -0.309016994375, 0.951056516295, 0.0, //H

        -0.9510565163,  0.3090169944,  0.5,    -0.809016994375, 0.587785252292, 0.0, //K
        -0.9510565163,  0.3090169944, -0.5,    -0.809016994375, 0.587785252292, 0.0, //L
        -0.5877852523,  0.8090169944,  0.5,    -0.809016994375, 0.587785252292, 0.0, //I
        -0.9510565163,  0.3090169944, -0.5,    -0.809016994375, 0.587785252292, 0.0, //L
        -0.5877852523,  0.8090169944,  0.5,    -0.809016994375, 0.587785252292, 0.0, //I
        -0.5877852523,  0.8090169944, -0.5,    -0.809016994375, 0.587785252292, 0.0, //J

        -0.9510565163, -0.3090169944,  0.5,    -1.0,            0.0,            0.0, //M
        -0.9510565163, -0.3090169944, -0.5,    -1.0,            0.0,            0.0, //N
        -0.9510565163,  0.3090169944,  0.5,    -1.0,            0.0,            0.0, //K
        -0.9510565163, -0.3090169944, -0.5,    -1.0,            0.0,            0.0, //N
        -0.9510565163,  0.3090169944,  0.5,    -1.0,            0.0,            0.0, //K
        -0.9510565163,  0.3090169944, -0.5,    -1.0,            0.0,            0.0, //L

        -0.5877852523, -0.8090169944,  0.5,    -0.809016994375,-0.587785252292, 0.0, //O
        -0.5877852523, -0.8090169944, -0.5,    -0.809016994375,-0.587785252292, 0.0, //P
        -0.9510565163, -0.3090169944,  0.5,    -0.809016994375,-0.587785252292, 0.0, //M
        -0.5877852523, -0.8090169944, -0.5,    -0.809016994375,-0.587785252292, 0.0, //P
        -0.9510565163, -0.3090169944,  0.5,    -0.809016994375,-0.587785252292, 0.0, //M
        -0.9510565163, -0.3090169944, -0.5,    -0.809016994375,-0.587785252292, 0.0, //N

        -0.0000000000, -1.0000000000,  0.5,    -0.309016994375,-0.951056516295, 0.0, //Q
        -0.0000000000, -1.0000000000, -0.5,    -0.309016994375,-0.951056516295, 0.0, //R
        -0.5877852523, -0.8090169944,  0.5,    -0.309016994375,-0.951056516295, 0.0, //O
        -0.0000000000, -1.0000000000, -0.5,    -0.309016994375,-0.951056516295, 0.0, //R
        -0.5877852523, -0.8090169944,  0.5,    -0.309016994375,-0.951056516295, 0.0, //O
        -0.5877852523, -0.8090169944, -0.5,    -0.309016994375,-0.951056516295, 0.0, //P

         0.5877852523, -0.8090169944,  0.5,    0.309016994375, -0.951056516295, 0.0, //S
         0.5877852523, -0.8090169944, -0.5,    0.309016994375, -0.951056516295, 0.0, //T
        -0.0000000000, -1.0000000000,  0.5,    0.309016994375, -0.951056516295, 0.0, //Q
         0.5877852523, -0.8090169944, -0.5,    0.309016994375, -0.951056516295, 0.0, //T
        -0.0000000000, -1.0000000000,  0.5,    0.309016994375, -0.951056516295, 0.0, //Q
        -0.0000000000, -1.0000000000, -0.5,    0.309016994375, -0.951056516295, 0.0, //R

         0.9510565163, -0.3090169944,  0.5,    0.809016994375, -0.587785252292, 0.0, //C
         0.9510565163, -0.3090169944, -0.5,    0.809016994375, -0.587785252292, 0.0, //D
         0.5877852523, -0.8090169944,  0.5,    0.809016994375, -0.587785252292, 0.0, //S
         0.9510565163, -0.3090169944, -0.5,    0.809016994375, -0.587785252292, 0.0, //D
         0.5877852523, -0.8090169944,  0.5,    0.809016994375, -0.587785252292, 0.0, //S
         0.5877852523, -0.8090169944, -0.5,    0.809016994375, -0.587785252292, 0.0  //T
    };
    float cubeVertices[] = {
        // positions          // texture Coords
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    float planeVertices[] = {
        // positions          // texture Coords 
         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,

         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
         5.0f, -0.5f, -5.0f,  2.0f, 2.0f
    };
    float transparentVertices[] = {
        // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
        0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
        0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
        1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

        0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
        1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
        1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    // load model  
    Model ourModel("texturas/backpack/backpack.obj");

    // #########################################################################################  VAOs y VBOs
    unsigned int VBOs[3], VAOs[3];
    glGenVertexArrays(3, VAOs); 
    glGenBuffers(3, VBOs);    
    // Triángulos tipo cero de la teselación    
    glBindVertexArray(VAOs[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, vert_ceros.size() * sizeof(float), &vert_ceros[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Triángulos tipo uno de la teselación    
    glBindVertexArray(VAOs[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
    glBufferData(GL_ARRAY_BUFFER, vert_unos.size() * sizeof(float), &vert_unos[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Orilas de la teselación
    glBindVertexArray(VAOs[2]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesOrilla), verticesOrilla, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // first, configure the cube's VAO (and VBO)
    unsigned int VBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // ###### VAO's de los objetos que usan texturas
    // cube VAO
    unsigned int cubeBlendVAO, cubeBlendVBO;
    glGenVertexArrays(1, &cubeBlendVAO);
    glGenBuffers(1, &cubeBlendVBO);
    glBindVertexArray(cubeBlendVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeBlendVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    // plane VAO
    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    // transparent VAO
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    // load textures    
    unsigned int cubeTexture = loadTexture("texturas/marble.jpg");
    unsigned int floorTexture = loadTexture("texturas/metal.png");
    unsigned int transparentTexture = loadTexture("texturas/window.png");

    // transparent window locations    
    vector<glm::vec3> windows {
        glm::vec3(-1.5f, 0.0f, -0.48f),
        glm::vec3(1.5f, 0.0f, 0.51f),
        glm::vec3(0.0f, 0.0f, 0.7f),
        glm::vec3(-0.3f, 0.0f, -2.3f),
        glm::vec3(0.5f, 0.0f, -0.6f)
    };

    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic        
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);                                
        
        glClearColor(185.0 / 255.0, 200.0 / 255.0, 215.0 / 255.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Mover la luz en una esfera
        if (circuloLampara) {
            float c = 18.0;
            float t = sin(0.1 * glfwGetTime()) * (pi / 2) + (pi / 2);
            float r = 2;

            pointLightPositions[0].x = r * sin(t) * sin(c * t) + 10;
            pointLightPositions[0].y = r * cos(t);
            pointLightPositions[0].z = r * sin(t) * cos(c * t);
        }        
                
        ourShader.use();                           
        ourShader.setVec3("viewPos", camera.Position);        
        iluminacion(ourShader);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // world transformation
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(1.0f, 0.3f, 0.5f));
        /*if (glfwGetTime() <= 10) {
            model = glm::translate(model, glm::vec3(0.0f, glfwGetTime() - 9, -10.0));
        }
        else {
            model = glm::translate(model, glm::vec3(0.0, 1.0f, -10.0f));
        }*/
        model = glm::translate(model, glm::vec3(0.0, 1.0f, -10.0f));
        ourShader.setMat4("model", model);

        // #########################################################################################  RENDER TESELACIÓN
        // ------------------------------- Render triángulos tipo cero (azul obscuro)                
        ourShader.setVec3("material.diffuse", 0.043f, 0.145f, 0.271f);
        ourShader.setVec3("material.specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("material.shininess", 32.0f);
                
        glBindVertexArray(VAOs[0]);        
        glDrawArrays(GL_TRIANGLES, 0, vert_ceros.size());    

        // ------------------------------- Render orillas
        glBindVertexArray(VAOs[2]);
        glDrawArrays(GL_TRIANGLES, 0, sizeof(verticesOrilla));

        // ------------------------------- Render triángulos tipo uno (azul claro)        
        ourShader.setVec3("material.diffuse", 0.698f, 0.761f, 0.929f);
        ourShader.setVec3("material.specular", 0.1f, 0.1f, 0.1f);
        ourShader.setFloat("material.shininess", 16.0f);
                
        glBindVertexArray(VAOs[1]);
        glDrawArrays(GL_TRIANGLES, 0, vert_unos.size());        

        // #########################################################################################  RENDER DE LAS LÁMPARAS
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        model = glm::mat4(1.0f);        
        glBindVertexArray(lightCubeVAO);
        for (unsigned int i = 0; i < 4; i++) {
            model = glm::mat4(1.0f);
            if (glfwGetTime() < 10) {
                model = glm::translate(model, glm::vec3(pointLightPositions[i].x, glfwGetTime() - pointLightPositions[i].y - 10.0, pointLightPositions[i].z));
            }
            else {
                model = glm::translate(model, pointLightPositions[i]);
            }
            model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
            lightCubeShader.setMat4("model", model);
            lightCubeShader.setVec3("ourColor", pointLightColors[i]);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }        

        // #########################################################################################  OBJETOS TRASLÚCIDOS
        // ordenar los objetos antes de renderizarlos
        std::map<float, glm::vec3> sorted;
        for (unsigned int i = 0; i < windows.size(); i++) {
            float distance = glm::length(camera.Position - windows[i]);
            sorted[distance] = windows[i];
        }        
        // render objetos
        shader_blend.use();        
        shader_blend.setMat4("projection", projection);
        shader_blend.setMat4("view", view);
        // cubes
        glBindVertexArray(cubeBlendVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cubeTexture);
        model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
        shader_blend.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
        shader_blend.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        // floor
        glBindVertexArray(planeVAO);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -5.0f, 0.0f));
        model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));
        shader_blend.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // windows (from furthest to nearest)
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        for (std::map<float, glm::vec3>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); ++it)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, it->second);
            shader_blend.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        mochilaShader.use();        
        mochilaShader.setFloat("material.shininess", 32.0f);
        iluminacion(mochilaShader);

        // LA MOCHILA        
        mochilaShader.setMat4("projection", projection);
        mochilaShader.setMat4("view", view);
        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(10.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
        mochilaShader.setMat4("model", model);
        ourModel.Draw(mochilaShader);

        // glfw: swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(2, VAOs);
    glDeleteBuffers(2, VBOs);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// Método para pasarle las características de la iluminación a un shader.
void iluminacion(Shader &shader) {    
    shader.setVec3("viewPos", camera.Position);
    // directional light
    shader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
    shader.setVec3("dirLight.diffuse", 0.8f, 0.8f, 1.0f);
    shader.setVec3("dirLight.specular", 0.8f, 0.8f, 0.8f);
    shader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
    // point light 1
    shader.setVec3("pointLights[0].position", pointLightPositions[0]);
    shader.setVec3("pointLights[0].ambient", pointLightColors[0].x * 0.1, pointLightColors[0].y * 0.1, pointLightColors[0].z * 0.1);
    shader.setVec3("pointLights[0].diffuse", pointLightColors[0].x, pointLightColors[0].y, pointLightColors[0].z);
    shader.setVec3("pointLights[0].specular", pointLightColors[0].x, pointLightColors[0].y, pointLightColors[0].z);
    shader.setFloat("pointLights[0].constant", 1.0f);
    shader.setFloat("pointLights[0].linear", 0.09);
    shader.setFloat("pointLights[0].quadratic", 0.032);
    // point light 2
    shader.setVec3("pointLights[1].position", pointLightPositions[1]);
    shader.setVec3("pointLights[1].ambient", pointLightColors[1].x * 0.1, pointLightColors[1].y * 0.1, pointLightColors[1].z * 0.1);
    shader.setVec3("pointLights[1].diffuse", pointLightColors[1].x, pointLightColors[1].y, pointLightColors[1].z);
    shader.setVec3("pointLights[1].specular", pointLightColors[1].x, pointLightColors[1].y, pointLightColors[1].z);
    shader.setFloat("pointLights[1].constant", 1.0f);
    shader.setFloat("pointLights[1].linear", 0.09);
    shader.setFloat("pointLights[1].quadratic", 0.032);
    // point light 3
    shader.setVec3("pointLights[2].position", pointLightPositions[2]);
    shader.setVec3("pointLights[2].ambient", pointLightColors[2].x * 0.1, pointLightColors[2].y * 0.1, pointLightColors[2].z * 0.1);
    shader.setVec3("pointLights[2].diffuse", pointLightColors[2].x, pointLightColors[2].y, pointLightColors[2].z);
    shader.setVec3("pointLights[2].specular", pointLightColors[2].x, pointLightColors[2].y, pointLightColors[2].z);
    shader.setFloat("pointLights[2].constant", 1.0f);
    shader.setFloat("pointLights[2].linear", 0.09);
    shader.setFloat("pointLights[2].quadratic", 0.032);
    // point light 4
    shader.setVec3("pointLights[3].position", pointLightPositions[3]);
    shader.setVec3("pointLights[3].ambient", pointLightColors[3].x * 0.1, pointLightColors[3].y * 0.1, pointLightColors[3].z * 0.1);
    shader.setVec3("pointLights[3].diffuse", pointLightColors[3].x, pointLightColors[3].y, pointLightColors[3].z);
    shader.setVec3("pointLights[3].specular", pointLightColors[3].x, pointLightColors[3].y, pointLightColors[3].z);
    shader.setFloat("pointLights[3].constant", 1.0f);
    shader.setFloat("pointLights[3].linear", 0.09);
    shader.setFloat("pointLights[3].quadratic", 0.032);
    // spotLight
    shader.setVec3("spotLight.position", camera.Position);
    shader.setVec3("spotLight.direction", camera.Front);
    shader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
    shader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
    shader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
    shader.setFloat("spotLight.constant", 1.0f);
    shader.setFloat("spotLight.linear", 0.09);
    shader.setFloat("spotLight.quadratic", 0.032);
    shader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
    shader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
}

// GFLW: input del usuario
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    
    // Mover la lámpara
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        pointLightPositions[1].x += 0.01;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        pointLightPositions[1].x -= 0.01;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        pointLightPositions[1].y += 0.01;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        pointLightPositions[1].y -= 0.01;
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
        pointLightPositions[1].z += 0.01;
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
        pointLightPositions[1].z -= 0.01;

    // Para detectar cuándo hay que mover la lámpara y cuándo no
    int estadoNuevo = glfwGetKey(window, GLFW_KEY_SPACE);
    if (estadoNuevo == GLFW_RELEASE && estadoAnt == GLFW_PRESS)
        circuloLampara = !circuloLampara;
    estadoAnt = estadoNuevo;
}

// GLFW: cambio del tamaño de la ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (width >= height) {
        glViewport(0, 0, width, width);
    }
    else {
        glViewport(0, 0, height, height);
    }
}

// GLFW: movimiento del mouse
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// GLFW: para detectar el scroll del mouse
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

// Función para cargar una textura en 2D
unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}