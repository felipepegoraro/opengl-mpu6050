#include <fcntl.h>
#include <termios.h> // https://www.man7.org/linux/man-pages/man0/termios.h.0p.html
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace std;

// typedef struct Acc {
//     float x;
//     float y;
//     float z;
// } Acc;

typedef struct AngEuler {
    float roll;
    float pitch;
    float yaw;
} AngEuler;

int setupSerial(void){
    int serial_port = open("/dev/ttyUSB0", O_RDWR);
    if (serial_port < 0) {
        perror("erro ao abrir porta serial");
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(serial_port, &tty) != 0) {
        perror("erro ao obter atributos da porta serial");
        close(serial_port);
        return -1;
    }

    cfsetispeed(&tty, B57600);
    cfsetospeed(&tty, B57600);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~ICANON;
    tty.c_iflag &= ~ECHO;
    tty.c_iflag &= ~ISIG;

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0){
        perror("erro ao configurar atributos da porta serial");
        close(serial_port);
        return -1;
    }

    return serial_port;
}

void readAngles(int serial_port,/* Acc *a*/ AngEuler *ang){
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));

    int n = read(serial_port, buffer, sizeof(buffer) - 1);
    if (n < 0){
        perror("erro na leitura da porta serial");
        return;
    }

    if (n > 0){
        sscanf(buffer,
               "Roll(%f) Pitch(%f) Yaw(%f)",
               &ang->roll, &ang->pitch, &ang->yaw
        );
    }
}

GLuint renderingProgram;
GLuint vao, vbo, ibo;
float cameraX, cameraY, cameraZ;
GLuint mvLoc, projLoc;
int width, height;
float aspect;
glm::mat4 pMat, vMat, mMat, mvMat;

string readShaderFile(const char *filePath) {
    string content;
    ifstream fileStream(filePath, ios::in);
    string line = "";
    while (getline(fileStream, line)) {
        content.append(line + "\n");
    }
    fileStream.close();
    return content;
}

GLuint compileShader(GLenum shaderType, const char *shaderPath) {
    GLuint shader = glCreateShader(shaderType);
    string shaderSource = readShaderFile(shaderPath);
    const char *source = shaderSource.c_str();
    
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint compileStatus;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus != GL_TRUE) {
        cout 
            << (shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment") 
            << " shader: erro na criacao" 
            << endl;

        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0) {
            char *log = (char *)malloc(logLength);
            glGetShaderInfoLog(shader, logLength, NULL, log);
            cout << "shader log: " << log << endl;
            free(log);
        }
    }
    
    return shader;
}

GLuint createShaderProgram(const char *vertexPath, const char *fragmentPath) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexPath);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentPath);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

    if (linkStatus != GL_TRUE) {
        cout << "Program linking failed." << endl;
        GLint logLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0) {
            char *log = (char *)malloc(logLength);
            glGetProgramInfoLog(program, logLength, NULL, log);
            cout << "programa info log: " << log << endl;
            free(log);
        }
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

void setupVertices(void) {
    GLfloat cubePositions[] = {
        //  x      y      z     u     v
        -1.0f, -1.0f,  1.0f, 0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f, 0.0f, 1.0f 
    };

    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        0, 3, 7, 7, 4, 0,
        1, 2, 6, 6, 5, 1,
        3, 2, 6, 6, 7, 3,
        0, 1, 5, 5, 4, 0 
    };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubePositions), cubePositions, GL_STATIC_DRAW);

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void init(GLFWwindow *window){
    renderingProgram = createShaderProgram("./vertex.glsl", "./frag.glsl");

    glfwGetFramebufferSize(window, &width, &height);
    aspect = (float)width / (float)height;
    pMat = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 1000.0f);
    cameraX = 0.0f;
    cameraY = 0.0f;
    cameraZ = 12.0f;

    setupVertices();
}

void display(GLFWwindow *window, AngEuler ang) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(renderingProgram);

    projLoc = glGetUniformLocation(renderingProgram, "proj_matrix");
    mvLoc = glGetUniformLocation(renderingProgram, "mv_matrix");

    vMat = glm::translate(glm::mat4(1.0f), glm::vec3(-cameraX, -cameraY, -cameraZ));

    // usar quaterniões pra evitar gimball lock
    // glm::quat quaternion = glm::quat(glm::vec3(
    //     glm::radians(ang.pitch),
    //     glm::radians(ang.yaw),
    //     glm::radians(ang.roll)
    // ));
    // glm::mat4 rotationMatrix = glm::toMat4(quaternion);
    // mvMat = vMat * rotationMatrix;

    glm::mat4 rotationMatrix = glm::rotate(
        glm::mat4(1.0f),
        glm::radians(ang.roll),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(ang.pitch), glm::vec3(0.0f, 1.0f, 0.0f));
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(ang.yaw), glm::vec3(0.0f, 0.0f, 1.0f));
    mvMat = vMat * rotationMatrix;

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
    glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));

    glBindVertexArray(vao);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}


int main(void){
    if (!glfwInit()) exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // no i3wm: for_window [title="(?i)^OpenGL:.*"] floating enable
    GLFWwindow *window = glfwCreateWindow(800, 800, "OpenGL: MPU6050 Cube Rotation", NULL, NULL);
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glewInit();
    glfwSwapInterval(1);

    init(window);

    int serial_port = setupSerial();
    if (serial_port < 0) return -1;

    // Acc acc;
    AngEuler ang;

    while (!glfwWindowShouldClose(window))
    {
        readAngles(serial_port, /*&acc*/ &ang);
        display(window, ang);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);

    glfwDestroyWindow(window);
    glfwTerminate();
    close(serial_port);
    return 0;
}

