#pragma once
#include <string>
#include <glm/glm.hpp>
#include "inventory.h"
#include "puzzle.h"

enum class GameState {
    MENU,
    PLAYING,
    PAUSED,
    INSPECT,
    CODE_PAD_UI,
    SEQUENCE_UI,
    WIN,
    LOSE
};

// Text line for the built-in bitmap font quad renderer
struct TextLine {
    std::string text;
    float x, y, scale;
    glm::vec4 color {1.f};
};

class UI {
public:
    void init(int w, int h);
    void shutdown();

    // Draws crosshair
    void drawCrosshair(unsigned int uiShader, const glm::mat4& ortho);

    // "Press E to …" prompt
    void drawInteractPrompt(const std::string& msg,
                            unsigned int uiShader, const glm::mat4& ortho);

    // Bottom inventory strip
    void drawInventory(const Inventory& inv,
                       unsigned int uiShader, const glm::mat4& ortho);

    // Overlay for code pad input
    void drawCodePad(const std::string& input,
                     unsigned int uiShader, const glm::mat4& ortho);

    // Sequence display (light up index, -1 = none)
    void drawSequencePanel(int litIndex, int total,
                           unsigned int uiShader, const glm::mat4& ortho);

    // FPS / Timer
    void drawHUD(float fps, float timerLeft,
                 unsigned int uiShader, const glm::mat4& ortho);

    // Full-screen overlays
    void drawWinScreen (unsigned int uiShader, const glm::mat4& ortho);
    void drawLoseScreen(unsigned int uiShader, const glm::mat4& ortho);
    void drawPauseMenu (unsigned int uiShader, const glm::mat4& ortho);
    void drawMainMenu  (unsigned int uiShader, const glm::mat4& ortho);
    void drawHintPanel (const std::string& hint,
                        unsigned int uiShader, const glm::mat4& ortho);

    // Simple text rendering via quads (each char = small colored rect)
    void drawText(const std::string& text, float x, float y,
                  float scale, glm::vec4 color,
                  unsigned int uiShader, const glm::mat4& ortho);

    int screenW {1280}, screenH {720};

private:
    unsigned int quadVAO {0}, quadVBO {0};
    void initQuad();
    void drawFilledRect(float x, float y, float w, float h,
                        glm::vec4 color,
                        unsigned int uiShader, const glm::mat4& ortho);
    void drawCharBlock(char c, float x, float y, float sz,
                       glm::vec4 col,
                       unsigned int uiShader, const glm::mat4& ortho);
    // 5x7 bitmap font data
    bool charPixel(char c, int col, int row) const;
};
