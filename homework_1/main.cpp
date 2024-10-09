/**
* Author: Sage Cronen-Townsend
* Assignment: Simple 2D Scene
* Date due: 2023-09-20, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/ 

#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };  // use enums instead of bools

constexpr int WINDOW_WIDTH = 640,        // in pixels?
              WINDOW_HEIGHT = 480;

constexpr int VIEWPORT_X      = 0,              // viewport position/dimensions
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr float BG_RED     = 1.0f,
                BG_GREEN   = 1.0f,
                BG_BLUE    = 1.0f,
                BG_OPACITY = 1.0f;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",   //filepaths
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECONDS= 1000.0;
int timer = 0;

constexpr char SPRITE1[] = "sprite1.png";
constexpr char SPRITE2[] = "sprite2.png";

constexpr glm::vec3 INIT_POS_SPRITE1    = glm::vec3(0.0f, 0.0f, 0.0f),
                    INIT_POS_SPRITE2 = glm::vec3(-0.0f, 0.0f, 0.0f);
glm::vec3           SCALE       = glm::vec3(1.0f, 1.0f, 0.0f);

GLuint sprite1_texture_id,
       sprite2_texture_id;

constexpr GLint NUM_TEXTURES = 1,       // to be generated
                LEVEL_OF_DETAIL = 0,    // mipmap reduction image level
                TEXTURE_BORDER = 0;     // this value MUST be zero

AppStatus g_app_status = RUNNING;
SDL_Window* g_display_window;
ShaderProgram g_shader_program;

glm::mat4 g_view_matrix,        // pose of camera
          g_projection_matrix,  // camera characteristics
          sprite1_matrix,
          sprite2_matrix;

//TODO: decide on transformations and assign appropriate constants here
constexpr float ROT_INCREMENT = 1.0f;
constexpr float TRANS_INCREMENT = 0.1f;

glm::vec3 rotation_sprite1 = glm::vec3(0.0f, 0.0f, 0.0f),
          rotation_sprite2 = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 translation_sprite1 = glm::vec3(1.5f, 0.0f, 0.0f),
          translation_sprite2 = glm::vec3(-1.5f, 0.0f, 0.0f);
glm::vec3 offset = glm::vec3(-2.0f, 0.0f, 0.0f);

float previous_ticks = 0.0f;

GLuint load_texture(const char* filepath) {
    int width, height, num_components;
    
    // load image file
    unsigned char* image = stbi_load(filepath, &width, &height,                                            &num_components, STBI_rgb_alpha);
    
    if (image == NULL) {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    // generate + bind texture ID to image
    GLuint textureID;                           // declaration
    glGenTextures(NUM_TEXTURES, &textureID);    // assignment
    glBindTexture(GL_TEXTURE_2D, textureID);
    // bind texture ID with raw image data, send image to graphics card
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA,
                 width, height, TEXTURE_BORDER, GL_RGBA,
                 GL_UNSIGNED_BYTE, image);
    
    // set texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    stbi_image_free(image);  // release file from memory
    
    return textureID;
}


void initialize() {
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Sage's 2D Scene",
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH,
                                        WINDOW_HEIGHT,
                                        SDL_WINDOW_OPENGL);
    
    if (g_display_window == nullptr) {
        std::cerr << "ERROR: SDL Window could not be created." << std::endl;
        g_app_status = TERMINATED;
        SDL_Quit();
        exit(1);
    }
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT); // camera
    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH); // load shaders
    sprite1_texture_id = load_texture(SPRITE1);
    sprite2_texture_id = load_texture(SPRITE2);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    sprite1_matrix = glm::mat4(1.0f);
    sprite2_matrix = glm::mat4(1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    glUseProgram(g_shader_program.get_program_id()); // each object has its own ID
    
    glEnable(GL_BLEND); // enable blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
}

void process_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            g_app_status = TERMINATED;
        }
    }
}

void update () {
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECONDS;  // get curr num of g_ticks
    float delta_time = ticks - previous_ticks;
    previous_ticks = ticks;
    
    // movement logic
    rotation_sprite1.z += ROT_INCREMENT * delta_time;
    rotation_sprite2.z -= 2 * ROT_INCREMENT * delta_time;
    
    glm::vec3 translation_factors1 = glm::vec3(3 * TRANS_INCREMENT, TRANS_INCREMENT, 0.0f);
    glm::vec3 translation_factors2 = glm::vec3(-3 * TRANS_INCREMENT, TRANS_INCREMENT, 0.0f);
    
    offset += translation_factors2 * delta_time;
    
    translation_sprite1 += translation_factors1 * delta_time;
    translation_sprite2 = translation_sprite1 + offset;
    
    SCALE += 0.1f * delta_time;
    
    // reset model matrices
    sprite1_matrix = glm::mat4(1.0f);
    sprite2_matrix = glm::mat4(1.0f);
    
    // apply transformations
    sprite1_matrix = glm::translate(sprite1_matrix, INIT_POS_SPRITE1);
    sprite1_matrix = glm::translate(sprite1_matrix, translation_sprite1);
    sprite1_matrix = glm::rotate(sprite1_matrix,
                                 rotation_sprite1.z,
                                 glm::vec3(0.0f, 0.0f, 1.0f));
    sprite1_matrix = glm::scale(sprite1_matrix, SCALE);
    
    sprite2_matrix = glm::translate(sprite2_matrix, INIT_POS_SPRITE2);
    sprite2_matrix = glm::translate(sprite2_matrix, translation_sprite2);
    sprite2_matrix = glm::rotate(sprite2_matrix,
                                 rotation_sprite2.z,
                                 glm::vec3(0.0f, 0.0f, 1.0f));
    sprite2_matrix = glm::scale(sprite2_matrix, SCALE);
}

void draw_object(glm::mat4 &model_matrix, GLuint &model_texture_id) {
    g_shader_program.set_model_matrix(model_matrix);
    glBindTexture(GL_TEXTURE_2D, model_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    // vertices
    float vertices[] = {-0.5f, -0.5f,       // triangle 1
                        0.5f, -0.5f,
                        0.5f, 0.5f,
                        -0.5f, -0.5f,       // triangle 2
                        0.5f, 0.5f,
                        -0.5f, 0.5f };
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(),
                          2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    
    // textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };
    
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(),
                          2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    // bind texture
    draw_object(sprite1_matrix, sprite1_texture_id);
    draw_object(sprite2_matrix, sprite2_texture_id);
    
    // disable two attribute arrays for the two triangles now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    initialize();
    
    while (g_app_status == RUNNING) {
        process_input ();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
