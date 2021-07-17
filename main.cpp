#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "SDL_mixer.h"

#include "Util.h"
#include "Entity.h"
#include "Map.h"
#include "Scene.h"
#include "Level1.h"
#include "Level2.h"
#include "Level3.h"
#include "MainMenu.h"

SDL_Window* displayWindow;
bool gameIsRunning = true;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

Scene* currentScene;
Scene* sceneList[4];

int lives = 3;
bool loseLife;

bool gameOver = false;
bool successful = false;

Mix_Music* music;
Mix_Chunk* error;

void SwitchToScene(Scene* scene) {
    currentScene = scene;
    currentScene->Initialize();
}


void Initialize() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    displayWindow = SDL_CreateWindow("Platformer!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(0, 0, 640, 480);

    program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");

    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    music = Mix_LoadMUS("newer_wave_music.mp3");
    Mix_PlayMusic(music, -1);
    Mix_VolumeMusic(MIX_MAX_VOLUME / 2);

    error = Mix_LoadWAV("error.wav");

    viewMatrix = glm::mat4(1.0f);
    modelMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);

    glUseProgram(program.programID);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    sceneList[0] = new MainMenu();
    sceneList[1] = new Level1();
    sceneList[2] = new Level2();
    sceneList[3] = new Level3();
    SwitchToScene(sceneList[0]);

}

void ProcessInput() {

    currentScene->state.player->movement = glm::vec3(0);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                gameIsRunning = false;
                break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
                case SDLK_RETURN:
                    if (currentScene->state.nextScene == 0)
                        currentScene->state.nextScene = 1;
                    break;

                case SDLK_LEFT:
                    // Move the player left
                    break;

                case SDLK_RIGHT:
                    // Move the player right
                    break;

                case SDLK_SPACE:
                    if (currentScene->state.player->collidedBottom) {
                        currentScene->state.player->jump = true;
                    }
                    break;

            }
            break; // SDL_KEYDOWN
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_LEFT]) {
        currentScene->state.player->movement.x = -1.0f;
        currentScene->state.player->animIndices = currentScene->state.player->animLeft;
    }
    else if (keys[SDL_SCANCODE_RIGHT]) {
        currentScene->state.player->movement.x = 1.0f;
        currentScene->state.player->animIndices = currentScene->state.player->animRight;
    }


    if (glm::length(currentScene->state.player->movement) > 1.0f) {
        currentScene->state.player->movement = glm::normalize(currentScene->state.player->movement);
    }

}

#define FIXED_TIMESTEP 0.0166666f
float lastTicks = 0;
float accumulator = 0.0f;

void Update() {

    if (gameOver) {
        SwitchToScene(currentScene);
        currentScene->state.player->isActive = false;
    }

    float ticks = (float)SDL_GetTicks() / 1000.0f; //first we get how much time has passed
    float deltaTime = ticks - lastTicks;
    lastTicks = ticks;

    deltaTime += accumulator; 
    if (deltaTime < FIXED_TIMESTEP) {
        accumulator = deltaTime;
        return;
    }

    while (deltaTime >= FIXED_TIMESTEP) {
        // Update. Notice it's FIXED_TIMESTEP. Not deltaTime
        currentScene->Update(FIXED_TIMESTEP);

        deltaTime -= FIXED_TIMESTEP;
    }

    accumulator = deltaTime;

    viewMatrix = glm::mat4(1.0f); 
    if (currentScene->state.player->position.x > 5 && currentScene->state.player->position.x < 14) {
        viewMatrix = glm::translate(viewMatrix, glm::vec3(-currentScene->state.player->position.x, 3.75, 0));
    }
    else if(currentScene->state.player->position.x >= 14){
        viewMatrix = glm::translate(viewMatrix, glm::vec3(-14, 3.75, 0));
    }
    else if (currentScene->state.player->position.x <= 5){
        viewMatrix = glm::translate(viewMatrix, glm::vec3(-5, 3.75, 0));
    }



    if (currentScene->state.player->loseLife) {
        lives -= 1;
        Mix_PlayChannel(-1, error, 0);
        SwitchToScene(currentScene);
    }
    
    if (lives <= 0) {
        gameOver = true;
        successful = false;
    }
    else {
        gameOver = currentScene->state.gameOver;
        successful = currentScene->state.sucessful;
    }
}

void Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    program.SetViewMatrix(viewMatrix);
    
    currentScene->Render(&program);

    GLuint font2 = Util::LoadTexture("font2.png");

    Util::DrawText(&program, font2, "Lives: " + std::to_string(lives), .35, -0.15f, glm::vec3(.75, -.25, 0));

    if (gameOver) {
        if (successful) {
            Util::DrawText(&program, font2, "You Win!", 1.5, -0.75f, glm::vec3(2.5, -3, 0));
        }
        else {
            Util::DrawText(&program, font2, "You Lose :(", 1.5, -0.75f, glm::vec3(1.5, -3, 0));
        }
    }

    SDL_GL_SwapWindow(displayWindow);
}


void Shutdown() {

    Mix_FreeChunk(error);
    Mix_FreeMusic(music);

    SDL_Quit();
}

int main(int argc, char* argv[]) {
    Initialize();

    while (gameIsRunning) {
        ProcessInput();
        Update();

        if (currentScene->state.nextScene >= 0) SwitchToScene(sceneList[currentScene->state.nextScene]);

        Render();
    }

    Shutdown();
    return 0;
}

