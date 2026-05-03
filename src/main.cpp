#define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cassert>
#include <chrono>
#include <cstddef>
#include <print>
#include <cstdlib>
#include <format>
#include <expected>
#include <array>
#include <stdexcept>
#include <utility>

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
    unsigned int shaderID = 0;
    Shader(unsigned int ID) : shaderID(ID) {}

public:
    ~Shader() {
        if (shaderID > 0) {
            // std::println("Shader {} deleted", shaderID);
            glDeleteProgram(shaderID);
        }
    }

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader &) = delete;
    Shader(Shader &&sh) {
        shaderID = sh.shaderID;
        sh.shaderID = 0;
    }

    static std::expected<Shader, std::runtime_error> init(const char *vertexShaderSource, const char *fragmentShaderSource) {
        if (vertexShaderSource == NULL || fragmentShaderSource == NULL) {
            return std::unexpected<std::runtime_error>("vertex and fragment shader sources should not be null");
        }

        // vertex shader
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
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
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
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
        // std::println("Using program: {}", shaderID);
        glUseProgram(shaderID);
        // std::println("Error: {}", glGetError());
        assert(glGetError() == GL_NO_ERROR);
    }
};

std::expected<std::array<Shader, 2>, std::runtime_error> initShaders() {
    const char *vs1 = ""
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
    const char *fs1 = ""
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
    auto shader1 = Shader::init(vs1, fs1);
    if (!shader1.has_value()) {
        return std::unexpected<std::runtime_error>(shader1.error());
    }

    const char *vs2 = ""
        "#version 330 core\n"
        "\n"
        "layout (location=0) in vec3 aPos;\n"
        "\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "uniform mat4 model;\n"
        "\n"
        "void main() {\n"
        "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "}\n";
    const char *fs2 = ""
        "#version 330 core\n"
        "\n"
        "uniform vec3 lightColor;\n"
        "out vec4 FragColor;\n"
        "\n"
        "void main() {\n"
        "   FragColor = vec4(lightColor, 1.0);\n"
        "}\n";

    auto shader2 = Shader::init(vs2, fs2);
    if (!shader2.has_value()) {
        return std::unexpected<std::runtime_error>(shader2.error());
    }

    return std::array<Shader, 2>{
        std::move(shader1.value()),
        std::move(shader2.value())
    };
}

int screenWidth = 800;
int screenHeight = 600;

struct Models {
    std::array<unsigned int, 2> VBO, VAO;
    std::array<Shader, 2> &shaders;
    std::array<unsigned int, 2> counts;

    ~Models() {
        glDeleteBuffers(1, &(VBO[0]));
        glDeleteVertexArrays(1, &(VAO[0]));
        glDeleteBuffers(1, &(VBO[1]));
        glDeleteVertexArrays(1, &(VAO[1]));
    }

    static Models init(std::array<Shader, 2> &shaders) {
        // initialize and setup main cube
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

        // initialize and setup light cube
        float lightData[] = {
            // front face
            -0.1,  0.1,  0.1,
            -0.1, -0.1,  0.1,
             0.1, -0.1,  0.1,
            -0.1,  0.1,  0.1,
             0.1, -0.1,  0.1,
             0.1,  0.1,  0.1,

            // top face
            -0.1,  0.1,  0.1,
             0.1,  0.1,  0.1,
            -0.1,  0.1, -0.1,
             0.1,  0.1,  0.1,
            -0.1,  0.1, -0.1,
             0.1,  0.1, -0.1,

            // back face
            -0.1,  0.1, -0.1,
            -0.1, -0.1, -0.1,
             0.1, -0.1, -0.1,
            -0.1,  0.1, -0.1,
             0.1, -0.1, -0.1,
             0.1,  0.1, -0.1,

            // bottom face
            -0.1, -0.1,  0.1,
             0.1, -0.1,  0.1,
            -0.1, -0.1, -0.1,
             0.1, -0.1,  0.1,
            -0.1, -0.1, -0.1,
             0.1, -0.1, -0.1,

            // left face
            -0.1,  0.1,  0.1,
            -0.1, -0.1,  0.1,
            -0.1, -0.1, -0.1,
            -0.1, -0.1, -0.1,
            -0.1,  0.1,  0.1,
            -0.1,  0.1, -0.1,

            // right face
             0.1,  0.1,  0.1,
             0.1, -0.1,  0.1,
             0.1, -0.1, -0.1,
             0.1, -0.1, -0.1,
             0.1,  0.1,  0.1,
             0.1,  0.1, -0.1,
        };

        unsigned int VBO2, VAO2;
        glGenBuffers(1, &VBO2);
        glGenVertexArrays(1, &VAO2);

        glBindVertexArray(VAO2);
        glBindBuffer(GL_ARRAY_BUFFER, VBO2);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lightData), lightData, GL_STATIC_DRAW);

        // aPos (shader)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
        glEnableVertexAttribArray(0);

        return Models {
            .VBO = { VBO, VBO2 },
            .VAO = { VAO, VAO2 },
            .shaders = shaders,
            .counts = {
                sizeof(data) / (2 * 3 * sizeof(float)),
                sizeof(lightData) / (3 * sizeof(float))
            }
        };
    }
};

void mainActivity(std::array<Shader, 2> &shaders, GLFWwindow &window) {
    auto models = Models::init(shaders);
    glm::vec3 cameraPos(3.0f, 2.0f, 5.0f);

    // set uniforms
    models.shaders[0].use();
    models.shaders[0].setVec3f("viewPos", cameraPos);
    models.shaders[0].setMat4f("view", glm::lookAt(
        cameraPos,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    ));
    models.shaders[0].setMat4f("model", glm::identity<glm::mat4>());
    models.shaders[0].setVec3f("aColor", glm::vec3(0.7, 0.5, 0.2));
    models.shaders[0].setVec3f("lightColor", glm::vec3(1.0, 1.0, 1.0));
    // shader.setVec3f("lightPos", glm::vec3(2.0, 1.5, 3.0));
    // shader.setVec3f("lightPos", glm::vec3(-3.0, 2.0, -5.0));
    // shader.setVec3f("lightPos", glm::vec3(-0.5, 0.0, 1.5));

    models.shaders[1].use();
    models.shaders[1].setMat4f("view", glm::lookAt(
        cameraPos,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    ));
    models.shaders[1].setVec3f("lightColor", glm::vec3(1.0, 1.0, 1.0));

    assert(glGetError() == GL_NO_ERROR);

    // std::println("Count: {}", models.counts[0]);
    // float bufRead[108] = {0};
    // glBindBuffer(GL_ARRAY_BUFFER, models.VBO[1]);
    // glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bufRead), bufRead);
    // std::println("Read data: {{");
    // for (int i = 0; i < 36; i++) {
    //     std::println("\t{}, {}, {},", bufRead[i * 3 + 0], bufRead[i * 3 + 1], bufRead[i * 3 + 2]);
    // }
    // std::println("}}");

    // render stuffs
    auto previousTime = std::chrono::steady_clock::now();
    glm::vec4 lightPos = glm::vec4(-2.0, 0.0, -2.0, 1.0);
    while (!glfwWindowShouldClose(&window)) {
        auto currentTime = std::chrono::steady_clock::now();
        assert(glGetError() == GL_NO_ERROR);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float deltaTime = std::chrono::duration<float>(currentTime - previousTime).count();
        const float aspectRatio = (float)screenWidth / (float)screenHeight;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 1.0f, 20.0f);
        glm::mat4 rotation = glm::identity<glm::mat4>();
        rotation = glm::rotate(rotation, glm::radians(30.0f * deltaTime), glm::vec3(0.0, 1.0, 0.0));
        lightPos = lightPos * rotation;

        // shader 1
        models.shaders[0].use();
        models.shaders[0].setMat4f("projection", projection);
        models.shaders[0].setVec3f("lightPos", glm::vec3(lightPos));
        glBindBuffer(GL_ARRAY_BUFFER, models.VBO[0]);
        glBindVertexArray(models.VAO[0]);
        glDrawArrays(GL_TRIANGLES, 0, models.counts[0]);

        // shader 2
        models.shaders[1].use();
        models.shaders[1].setMat4f("projection", projection);
        models.shaders[1].setMat4f("model",
                glm::translate(glm::identity<glm::mat4>(), glm::vec3(lightPos)));
        glBindBuffer(GL_ARRAY_BUFFER, models.VBO[1]);
        glBindVertexArray(models.VAO[1]);
        glDrawArrays(GL_TRIANGLES, 0, models.counts[1]);

        glfwPollEvents();
        glfwSwapBuffers(&window);
        previousTime = currentTime;
    }

}

int main(void) {
    glfwSetErrorCallback([](int error, const char *description) {
        std::println(stderr, "Error: {}\n", description);
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

    auto shaderProgram = initShaders();
    if (!shaderProgram.has_value()) {
        std::println(stderr, "{}", shaderProgram.error().what());
        return EXIT_FAILURE;
    }
    
    mainActivity(shaderProgram.value(), *window);

    return EXIT_SUCCESS;
}
