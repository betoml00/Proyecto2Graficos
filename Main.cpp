#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>


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

// Tamaño de la pantalla
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 1000;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;


// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
bool circuloLampara = true;
int estadoAnt = GLFW_RELEASE;
// positions of the point lights
glm::vec3 pointLightPositions[] = {
    glm::vec3(0.7f,  0.2f,  2.0f),
    glm::vec3(2.3f, -3.3f, -4.0f),
    glm::vec3(-4.0f,  2.0f, -12.0f),
    glm::vec3(0.0f,  0.0f, -3.0f)
};

// #############################################################################
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
// Podría optimizarse liberando la memoria de los triángulos originales, pero no
// dio tiempo.
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

// Este método no tiene una aplicación real en el código; sin embargo, lo utilicé para asegurarme
// de que los valores que estaba generando el algoritmo fueran los correctos.
void imprimeTriangulos(list<struct triangulo*> triangulos) {
    list<struct triangulo*> ::iterator it;
    for (it = triangulos.begin(); it != triangulos.end(); ++it) {
        printf("%f, %f, 0.0 \n%f, %f, 0.0 \n%f, %f, 0.0\n", (*it)->A.real(), (*it)->A.imag(),
            (*it)->B.real(), (*it)->B.imag(), (*it)->C.real(), (*it)->C.imag());
    }
}
// #########################################################################################

// Imprime a consola el manual de usuario
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

// Método principal
int main()
{
    // Parte para calcular lo de Penrose
    // Empezamos con 10 triángulos alrededor del origen.
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
        // Para imprimir las orillas. Lo dejo por cualquier cosa.
        // printf("(%.10f, %.10f)\n", t->B.real(), t->B.imag());        
        // printf("(%.10f, %.10f)\n", t->C.real(), t->C.imag());
    }

    // Subdividimos los triángulos las veces que indice la constante NUM_SUBDIVISIONES.
    for (int j = 0; j < NUM_SUBDIVISONES; j++) {
        triangulos = subdividir(triangulos);
    }

    // Guardamos los triángulos en formato de vértices para OpenGL.
    list<struct triangulo*> ::iterator it;
    vector<float> vert_ceros;
    vector<float> vert_unos;    
    for (it = triangulos.begin(); it != triangulos.end(); ++it) {
        if ((*it)->color == 0) {            
            vert_ceros.push_back((float)(*it)->A.real());
            vert_ceros.push_back((float)(*it)->A.imag());
            vert_ceros.push_back(0.5f);
            // Vector normal:
            vert_ceros.push_back(0.0f);
            vert_ceros.push_back(0.0f);
            vert_ceros.push_back(1.0f);

            vert_ceros.push_back((float)(*it)->B.real());
            vert_ceros.push_back((float)(*it)->B.imag());
            vert_ceros.push_back(0.5f);
            // Vector normal:
            vert_ceros.push_back(0.0f);
            vert_ceros.push_back(0.0f);
            vert_ceros.push_back(1.0f);

            vert_ceros.push_back((float)(*it)->C.real());
            vert_ceros.push_back((float)(*it)->C.imag());
            vert_ceros.push_back(0.5f);
            // Vector normal:
            vert_ceros.push_back(0.0f);
            vert_ceros.push_back(0.0f);            
            vert_ceros.push_back(1.0f);

            //------------- Parte 2
            vert_ceros.push_back((float)(*it)->A.real());
            vert_ceros.push_back((float)(*it)->A.imag());
            vert_ceros.push_back(-0.5f);
            // Vector normal:
            vert_ceros.push_back(0.0f);
            vert_ceros.push_back(0.0f);
            vert_ceros.push_back(-1.0f);

            vert_ceros.push_back((float)(*it)->B.real());
            vert_ceros.push_back((float)(*it)->B.imag());
            vert_ceros.push_back(-0.5f);
            // Vector normal:
            vert_ceros.push_back(0.0f);
            vert_ceros.push_back(0.0f);
            vert_ceros.push_back(-1.0f);

            vert_ceros.push_back((float)(*it)->C.real());
            vert_ceros.push_back((float)(*it)->C.imag());
            vert_ceros.push_back(-0.5f);
            // Vector normal:
            vert_ceros.push_back(0.0f);
            vert_ceros.push_back(0.0f);            
            vert_ceros.push_back(-1.0f);
        }
        else {            
            vert_unos.push_back((float)(*it)->A.real());
            vert_unos.push_back((float)(*it)->A.imag());            
            vert_unos.push_back(0.5f);
            // Vector normal:
            vert_unos.push_back(0.0f);
            vert_unos.push_back(0.0f);
            vert_unos.push_back(1.0f);

            vert_unos.push_back((float)(*it)->B.real());
            vert_unos.push_back((float)(*it)->B.imag());            
            vert_unos.push_back(0.5f);
            // Vector normal:
            vert_unos.push_back(0.0f);
            vert_unos.push_back(0.0f);
            vert_unos.push_back(1.0f);

            vert_unos.push_back((float)(*it)->C.real());
            vert_unos.push_back((float)(*it)->C.imag());            
            vert_unos.push_back(0.5f);
            // Vector normal:
            vert_unos.push_back(0.0f);
            vert_unos.push_back(0.0f);
            vert_unos.push_back(1.0f);

            // Parte 2
            vert_unos.push_back((float)(*it)->A.real());
            vert_unos.push_back((float)(*it)->A.imag());
            vert_unos.push_back(-0.5f);
            // Vector normal:
            vert_unos.push_back(0.0f);
            vert_unos.push_back(0.0f);
            vert_unos.push_back(-1.0f);

            vert_unos.push_back((float)(*it)->B.real());
            vert_unos.push_back((float)(*it)->B.imag());
            vert_unos.push_back(-0.5f);
            // Vector normal:
            vert_unos.push_back(0.0f);
            vert_unos.push_back(0.0f);
            vert_unos.push_back(-1.0f);

            vert_unos.push_back((float)(*it)->C.real());
            vert_unos.push_back((float)(*it)->C.imag());
            vert_unos.push_back(-0.5f);
            // Vector normal:
            vert_unos.push_back(0.0f);
            vert_unos.push_back(0.0f);            
            vert_unos.push_back(-1.0f);
        }
    }
    // #######################################################################################

    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw creación de la ventana
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Proyecto 2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }    

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // construimos y compilamos los shaders
    // ------------------------------------
    Shader ourShader("shaders/proyecto2.vs", "shaders/proyecto2.fs");
    Shader lightCubeShader("shaders/light_cube.vs", "shaders/light_cube.fs");

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

    unsigned int VBOs[3], VAOs[3];
    glGenVertexArrays(3, VAOs); 
    glGenBuffers(3, VBOs);    
    // Triángulos tipo cero de la teselación
    // --------------------
    glBindVertexArray(VAOs[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, vert_ceros.size() * sizeof(float), &vert_ceros[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Triángulos tipo uno de la teselación
    // ---------------------
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

    // Para dibujar únicamente los bordes
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);                                

        //glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClearColor(185.0 / 255.0, 200.0 / 255.0, 215.0 / 255.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // change the light's position values over time (can be done anywhere in the render loop actually, but try to do it at least before using the light source positions)
        // Mover la luz en una esfera - https://en.wikipedia.org/wiki/Spiral#Spherical_spirals
        if (circuloLampara) {
            float c = 18.0;
            float t = sin(0.1 * glfwGetTime()) * (pi / 2) + (pi / 2);
            float r = 2;

            pointLightPositions[0].x = r * sin(t) * sin(c * t);
            pointLightPositions[0].y = r * cos(t);
            pointLightPositions[0].z = r * sin(t) * cos(c * t);
        }        
        
        // Pasar la ubicación de la luz y de dónde se está viendo
        ourShader.use();                   
        // ourShader.setVec3("light.position", lightPos);
        ourShader.setVec3("viewPos", camera.Position);        

        /*
           Here we set all the uniforms for the 5/6 types of lights we have. We have to set them manually and index
           the proper PointLight struct in the array to set each uniform variable. This can be done more code-friendly
           by defining light types as classes and set their values in there, or by using a more efficient uniform approach
           by using 'Uniform buffer objects', but that is something we'll discuss in the 'Advanced GLSL' tutorial.
        */
        glm::vec3 pointLightColors[] = {
            glm::vec3(236.0/255.0, 239.0 / 255.0, 248.0 / 255.0),
            glm::vec3(157.0 / 255.0, 190.0 / 255.0, 212.0 / 255.0),
            glm::vec3(209.0 / 255.0, 216.0 / 255.0, 226.0 / 255.0),            
            glm::vec3(79.0 / 255.0, 156.0 / 255.0, 200.0 / 255.0)
        };
        // directional light
        ourShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        ourShader.setVec3("dirLight.diffuse", 0.8f, 0.8f, 1.0f);
        ourShader.setVec3("dirLight.specular", 0.8f, 0.8f, 0.8f);
        ourShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
        // point light 1
        ourShader.setVec3("pointLights[0].position", pointLightPositions[0]);
        ourShader.setVec3("pointLights[0].ambient", pointLightColors[0].x * 0.1, pointLightColors[0].y * 0.1, pointLightColors[0].z * 0.1);
        ourShader.setVec3("pointLights[0].diffuse", pointLightColors[0].x, pointLightColors[0].y, pointLightColors[0].z);
        ourShader.setVec3("pointLights[0].specular", pointLightColors[0].x, pointLightColors[0].y, pointLightColors[0].z);
        ourShader.setFloat("pointLights[0].constant", 1.0f);
        ourShader.setFloat("pointLights[0].linear", 0.09);
        ourShader.setFloat("pointLights[0].quadratic", 0.032);
        // point light 2
        ourShader.setVec3("pointLights[1].position", pointLightPositions[1]);
        ourShader.setVec3("pointLights[1].ambient", pointLightColors[1].x * 0.1, pointLightColors[1].y * 0.1, pointLightColors[1].z * 0.1);
        ourShader.setVec3("pointLights[1].diffuse", pointLightColors[1].x, pointLightColors[1].y, pointLightColors[1].z);
        ourShader.setVec3("pointLights[1].specular", pointLightColors[1].x, pointLightColors[1].y, pointLightColors[1].z);
        ourShader.setFloat("pointLights[1].constant", 1.0f);
        ourShader.setFloat("pointLights[1].linear", 0.09);
        ourShader.setFloat("pointLights[1].quadratic", 0.032);
        // point light 3
        ourShader.setVec3("pointLights[2].position", pointLightPositions[2]);
        ourShader.setVec3("pointLights[2].ambient", pointLightColors[2].x * 0.1, pointLightColors[2].y * 0.1, pointLightColors[2].z * 0.1);
        ourShader.setVec3("pointLights[2].diffuse", pointLightColors[2].x, pointLightColors[2].y, pointLightColors[2].z);
        ourShader.setVec3("pointLights[2].specular", pointLightColors[2].x, pointLightColors[2].y, pointLightColors[2].z);
        ourShader.setFloat("pointLights[2].constant", 1.0f);
        ourShader.setFloat("pointLights[2].linear", 0.09);
        ourShader.setFloat("pointLights[2].quadratic", 0.032);
        // point light 4
        ourShader.setVec3("pointLights[3].position", pointLightPositions[3]);
        ourShader.setVec3("pointLights[3].ambient", pointLightColors[3].x * 0.1, pointLightColors[3].y * 0.1, pointLightColors[3].z * 0.1);
        ourShader.setVec3("pointLights[3].diffuse", pointLightColors[3].x, pointLightColors[3].y, pointLightColors[3].z);
        ourShader.setVec3("pointLights[3].specular", pointLightColors[3].x, pointLightColors[3].y, pointLightColors[3].z);
        ourShader.setFloat("pointLights[3].constant", 1.0f);
        ourShader.setFloat("pointLights[3].linear", 0.09);
        ourShader.setFloat("pointLights[3].quadratic", 0.032);
        // spotLight
        ourShader.setVec3("spotLight.position", camera.Position);
        ourShader.setVec3("spotLight.direction", camera.Front);
        ourShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        ourShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        ourShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("spotLight.constant", 1.0f);
        ourShader.setFloat("spotLight.linear", 0.09);
        ourShader.setFloat("spotLight.quadratic", 0.032);
        ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        ourShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // world transformation
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(1.0f, 0.3f, 0.5f));
        if (glfwGetTime() <= 10) {
            model = glm::translate(model, glm::vec3(10 - glfwGetTime(), 0.0f, 0.0f));
        }           
        ourShader.setMat4("model", model);

        // ------------------------------- Render triángulos tipo cero (azul obscuro)
        // material properties        
        ourShader.setVec3("material.diffuse", 0.043f, 0.145f, 0.271f);
        ourShader.setVec3("material.specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("material.shininess", 32.0f);
                
        glBindVertexArray(VAOs[0]);        
        glDrawArrays(GL_TRIANGLES, 0, vert_ceros.size());    

        // ------------------------------- Render orillas
        glBindVertexArray(VAOs[2]);
        glDrawArrays(GL_TRIANGLES, 0, sizeof(verticesOrilla));

        // ------------------------------- Render triángulos tipo uno (azul claro)
        // material properties
        ourShader.setVec3("material.diffuse", 0.698f, 0.761f, 0.929f);
        ourShader.setVec3("material.specular", 0.1f, 0.1f, 0.1f);
        ourShader.setFloat("material.shininess", 16.0f);
                
        glBindVertexArray(VAOs[1]);
        glDrawArrays(GL_TRIANGLES, 0, vert_unos.size());        

        // also draw the lamp object(s)
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);

        // we now draw as many light bulbs as we have point lights.
        glBindVertexArray(lightCubeVAO);
        for (unsigned int i = 0; i < 4; i++)
        {
            model = glm::mat4(1.0f);
            if (glfwGetTime() < 10) {
                model = glm::translate(model, glm::vec3(10 - glfwGetTime() - pointLightPositions[i].x, pointLightPositions[i].y, 10 - glfwGetTime() - pointLightPositions[i].z));
            }
            else {
                model = glm::translate(model, pointLightPositions[i]);
            }
            model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
            lightCubeShader.setMat4("model", model);
            lightCubeShader.setVec3("ourColor", pointLightColors[i]);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

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

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
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

    int estadoNuevo = glfwGetKey(window, GLFW_KEY_SPACE);
    if (estadoNuevo == GLFW_RELEASE && estadoAnt == GLFW_PRESS)
        circuloLampara = !circuloLampara;
    estadoAnt = estadoNuevo;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    if (width >= height) {
        glViewport(0, 0, width, width);
    }
    else {
        glViewport(0, 0, height, height);
    }
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
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

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}
