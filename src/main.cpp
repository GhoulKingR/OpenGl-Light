#define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <expected>

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
    unsigned int shaderID = 0;
    Shader(unsigned int ID) : shaderID(ID) {}

public:
    ~Shader() {
        if (shaderID > 0) {
            glDeleteProgram(shaderID);
        }
    }

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader &) = delete;
    Shader(Shader &&sh) {
        shaderID = sh.shaderID;
        sh.shaderID = 0;
    }

    static std::expected<Shader, std::runtime_error> init() {
        // vertex shader
        const char *vertexShaderCode = ""
            "#version 330 core\n"
            "\n"
            "layout (location=0) in vec3 aPos;\n"
            "layout (location=1) in vec3 aNormal;\n"
            "\n"
            "uniform mat4 projection;\n"
            "uniform mat4 view;\n"
            "uniform mat4 model;\n"
            "uniform vec3 lightPos;\n"
            "uniform vec3 viewPos;\n"
            "\n"
            "out vec3 Normal;\n"
            "out vec3 light;\n"
            "out vec3 viewFrag;\n"
            "\n"
            "void main() {\n"
            "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
            "   Normal = mat3(transpose(inverse(model))) * aNormal;\n"
            "   light = lightPos - (model * vec4(aPos, 1.0)).xyz;\n"
            "   viewFrag = viewPos - (model * vec4(aPos, 1.0)).xyz;\n"
            "}\n";
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
        glCompileShader(vertexShader);

        // error handling
        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            return std::unexpected<std::runtime_error>(
                std::format("VERTEX SHADER ERROR :: {}", infoLog));
        }

        // fragment shader
        const char *fragmentShaderCode = ""
            "#version 330 core\n"
            "\n"
            "in vec3 Normal;\n"
            "in vec3 light;\n"
            "in vec3 viewFrag;\n"
            "\n"
            "uniform vec3 aColor;\n"
            "uniform vec3 lightColor;\n"
            "\n"
            "out vec4 FragColor;\n"
            "\n"
            "void main() {\n"
            "   vec3 lights = vec3(0.0, 0.0, 0.0);\n"
            "   float ambientStrength = 0.2;\n"
            "   float specularStrength = 0.5;\n"
            "   lights += lightColor * ambientStrength;  // ambient light\n"
            "   \n"
            "   float NdotL = dot(normalize(Normal), normalize(light));\n"
            "   if (NdotL > 0) {\n"
            "       // diffuse lighting\n"
            "       lights += max(NdotL, 0.0);  // diffuse light\n"
            "       \n"
            "       // specular lighting\n"
            "       vec3 reflectDir = reflect(-normalize(light), Normal);\n"
            "       float viewHit = dot(reflectDir, normalize(viewFrag));\n"
            "       float spec = pow(max(viewHit, 0.0), 16.0);\n"
            "       lights += specularStrength * spec * lightColor; // specular light\n"
            "   }\n"
            "   \n"
            "   FragColor = vec4(aColor * lights, 1.0);\n"
            "}\n";
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderCode, NULL);
        glCompileShader(fragmentShader);

        // error handling
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glDeleteShader(vertexShader);
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            return std::unexpected<std::runtime_error>(
                std::format("FRAGMENT SHADER ERROR :: {}", infoLog));
        }

        // shader program
        unsigned int shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        // error handling
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            return std::unexpected<std::runtime_error>(
                std::format("SHADER PROGRAM ERROR :: {}", infoLog));
        }

        // cleanup
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return Shader(shaderProgram);
    }

    void setFloat(const char *name, float data) {
        glUniform1f(glGetUniformLocation(shaderID, name), data);
    }

    void setMat4f(const char *name, const glm::mat4 &data) {
        glUniformMatrix4fv(glGetUniformLocation(shaderID, name), 1, GL_FALSE, glm::value_ptr(data));
    }

    void setVec3f(const char *name, const glm::vec3 &data) {
        glUniform3f(glGetUniformLocation(shaderID, name), data.x, data.y, data.z);
    }

    void use() {
        glUseProgram(shaderID);
        assert(glGetError() == GL_NO_ERROR);
    }
};

int screenWidth = 800;
int screenHeight = 600;

void mainActivity(Shader &shader, GLFWwindow &window) {
    // // initialize and setup main cube
    float data[] = {
        // vertices             // normals          // labels
        // front face
        -1.0,  1.0,  1.0,        0.0,  0.0,  1.0,     // ltf
        -1.0, -1.0,  1.0,        0.0,  0.0,  1.0,     // lbf
         1.0, -1.0,  1.0,        0.0,  0.0,  1.0,     // rbf
        -1.0,  1.0,  1.0,        0.0,  0.0,  1.0,     // ltf
         1.0, -1.0,  1.0,        0.0,  0.0,  1.0,     // rbf
         1.0,  1.0,  1.0,        0.0,  0.0,  1.0,     // rtf

        // top face
        -1.0,  1.0,  1.0,        0.0,  1.0,  0.0,     // ltf
         1.0,  1.0,  1.0,        0.0,  1.0,  0.0,     // rtf
        -1.0,  1.0, -1.0,        0.0,  1.0,  0.0,     // ltb
         1.0,  1.0,  1.0,        0.0,  1.0,  0.0,     // rtf
        -1.0,  1.0, -1.0,        0.0,  1.0,  0.0,     // ltb
         1.0,  1.0, -1.0,        0.0,  1.0,  0.0,     // rtb

        // back face
        -1.0,  1.0, -1.0,        0.0,  0.0, -1.0,     // ltb
        -1.0, -1.0, -1.0,        0.0,  0.0, -1.0,     // lbb
         1.0, -1.0, -1.0,        0.0,  0.0, -1.0,     // rbb
        -1.0,  1.0, -1.0,        0.0,  0.0, -1.0,     // ltb
         1.0, -1.0, -1.0,        0.0,  0.0, -1.0,     // rbb
         1.0,  1.0, -1.0,        0.0,  0.0, -1.0,     // rtb

        // bottom face
        -1.0, -1.0,  1.0,        0.0, -1.0,  0.0,     // lbf
         1.0, -1.0,  1.0,        0.0, -1.0,  0.0,     // rbf
        -1.0, -1.0, -1.0,        0.0, -1.0,  0.0,     // lbb
         1.0, -1.0,  1.0,        0.0, -1.0,  0.0,     // rbf
        -1.0, -1.0, -1.0,        0.0, -1.0,  0.0,     // lbb
         1.0, -1.0, -1.0,        0.0, -1.0,  0.0,     // rbb

        // left face
        -1.0,  1.0,  1.0,       -1.0,  0.0,  0.0,     // ltf
        -1.0, -1.0,  1.0,       -1.0,  0.0,  0.0,     // lbf
        -1.0, -1.0, -1.0,       -1.0,  0.0,  0.0,     // lbb
        -1.0, -1.0, -1.0,       -1.0,  0.0,  0.0,     // lbb
        -1.0,  1.0,  1.0,       -1.0,  0.0,  0.0,     // ltf
        -1.0,  1.0, -1.0,       -1.0,  0.0,  0.0,     // ltb

        // right face
         1.0,  1.0,  1.0,        1.0,  0.0,  0.0,     // rtf
         1.0, -1.0,  1.0,        1.0,  0.0,  0.0,     // rbf
         1.0, -1.0, -1.0,        1.0,  0.0,  0.0,     // rbb
         1.0, -1.0, -1.0,        1.0,  0.0,  0.0,     // rbb
         1.0,  1.0,  1.0,        1.0,  0.0,  0.0,     // rtf
         1.0,  1.0, -1.0,        1.0,  0.0,  0.0,     // rtb
    };

    // init and copy vertex data to buffers
    unsigned int VBO, VAO;
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

    // aPos (shader)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);

    // aNormal (shader)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // set uniforms
    shader.use();
    glm::vec3 cameraPos(3.0f, 2.0f, 5.0f);
    shader.setVec3f("viewPos", cameraPos);
    shader.setMat4f("view", glm::lookAt(
        cameraPos,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    ));
    shader.setMat4f("model", glm::identity<glm::mat4>());
    shader.setVec3f("aColor", glm::vec3(0.7, 0.5, 0.2));
    shader.setVec3f("lightColor", glm::vec3(1.0, 1.0, 1.0));
    // shader.setVec3f("lightPos", glm::vec3(2.0, 1.5, 3.0));
    shader.setVec3f("lightPos", glm::vec3(-3.0, 2.0, -5.0));
    // shader.setVec3f("lightPos", glm::vec3(-0.5, 0.0, 1.5));

    const int count = sizeof(data) / (2 * 3 * sizeof(float));
    printf("Count: %d\n", count);
    
    // float bufRead[9] = {0};
    // glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bufRead), bufRead);
    // printf("Read data: {\n");
    // for (int i = 0; i < 3; i++) {
    //     printf("\t%f, %f, %f\n", bufRead[i * 3 + 0], bufRead[i * 3 + 1], bufRead[i * 3 + 2]);
    // }
    // printf("}\n");

    // render stuffs
    while (!glfwWindowShouldClose(&window)) {
        assert(glGetError() == GL_NO_ERROR);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspectRatio = (float)screenWidth / (float)screenHeight;
        shader.setMat4f("projection", glm::perspective(glm::radians(45.0f), aspectRatio, 1.0f, 10.0f));

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, count);

        glfwPollEvents();
        glfwSwapBuffers(&window);
    }

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

int main(void) {
    glfwSetErrorCallback([](int error, const char *description) {
        fprintf(stderr, "Error: %s\n", description);
    });

    // init glfw window
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow *window = glfwCreateWindow(screenWidth, screenHeight, "OpenGL lighting", NULL, NULL);

    assert(window != NULL && "Failed to initialize GLFW window");
    glfwMakeContextCurrent(window);

    // initialize GLAD
    assert(
        gladLoadGLLoader((GLADloadproc) glfwGetProcAddress) &&
        "Failed to initialize GLAD"
    );

    // framebuffer change callback
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height){
        glViewport(0, 0, width, height);
        screenHeight = height;
        screenWidth = width;
    });

    auto shaderProgram = Shader::init();
    if (!shaderProgram.has_value()) {
        fprintf(stderr, "%s\n", shaderProgram.error().what());
        return EXIT_FAILURE;
    }
    
    mainActivity(shaderProgram.value(), *window);

    return EXIT_SUCCESS;
}
