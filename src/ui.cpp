#include "ui.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <cstring>

// ── 5×7 bitmap font (ASCII 32–126) ────────────────────────────────────────────
// Each char encoded as 5 bytes, each byte = one column of 7 bits (bit0=top)
static const unsigned char s_font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // 32 space
    {0x00,0x00,0x5F,0x00,0x00}, // 33 !
    {0x00,0x07,0x00,0x07,0x00}, // 34 "
    {0x14,0x7F,0x14,0x7F,0x14}, // 35 #
    {0x24,0x2A,0x7F,0x2A,0x12}, // 36 $
    {0x23,0x13,0x08,0x64,0x62}, // 37 %
    {0x36,0x49,0x55,0x22,0x50}, // 38 &
    {0x00,0x05,0x03,0x00,0x00}, // 39 '
    {0x00,0x1C,0x22,0x41,0x00}, // 40 (
    {0x00,0x41,0x22,0x1C,0x00}, // 41 )
    {0x14,0x08,0x3E,0x08,0x14}, // 42 *
    {0x08,0x08,0x3E,0x08,0x08}, // 43 +
    {0x00,0x50,0x30,0x00,0x00}, // 44 ,
    {0x08,0x08,0x08,0x08,0x08}, // 45 -
    {0x00,0x60,0x60,0x00,0x00}, // 46 .
    {0x20,0x10,0x08,0x04,0x02}, // 47 /
    {0x3E,0x51,0x49,0x45,0x3E}, // 48 0
    {0x00,0x42,0x7F,0x40,0x00}, // 49 1
    {0x42,0x61,0x51,0x49,0x46}, // 50 2
    {0x21,0x41,0x45,0x4B,0x31}, // 51 3
    {0x18,0x14,0x12,0x7F,0x10}, // 52 4
    {0x27,0x45,0x45,0x45,0x39}, // 53 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 54 6
    {0x01,0x71,0x09,0x05,0x03}, // 55 7
    {0x36,0x49,0x49,0x49,0x36}, // 56 8
    {0x06,0x49,0x49,0x29,0x1E}, // 57 9
    {0x00,0x36,0x36,0x00,0x00}, // 58 :
    {0x00,0x56,0x36,0x00,0x00}, // 59 ;
    {0x08,0x14,0x22,0x41,0x00}, // 60 <
    {0x14,0x14,0x14,0x14,0x14}, // 61 =
    {0x00,0x41,0x22,0x14,0x08}, // 62 >
    {0x02,0x01,0x51,0x09,0x06}, // 63 ?
    {0x32,0x49,0x79,0x41,0x3E}, // 64 @
    {0x7E,0x11,0x11,0x11,0x7E}, // 65 A
    {0x7F,0x49,0x49,0x49,0x36}, // 66 B
    {0x3E,0x41,0x41,0x41,0x22}, // 67 C
    {0x7F,0x41,0x41,0x22,0x1C}, // 68 D
    {0x7F,0x49,0x49,0x49,0x41}, // 69 E
    {0x7F,0x09,0x09,0x09,0x01}, // 70 F
    {0x3E,0x41,0x49,0x49,0x7A}, // 71 G
    {0x7F,0x08,0x08,0x08,0x7F}, // 72 H
    {0x00,0x41,0x7F,0x41,0x00}, // 73 I
    {0x20,0x40,0x41,0x3F,0x01}, // 74 J
    {0x7F,0x08,0x14,0x22,0x41}, // 75 K
    {0x7F,0x40,0x40,0x40,0x40}, // 76 L
    {0x7F,0x02,0x04,0x02,0x7F}, // 77 M
    {0x7F,0x04,0x08,0x10,0x7F}, // 78 N
    {0x3E,0x41,0x41,0x41,0x3E}, // 79 O
    {0x7F,0x09,0x09,0x09,0x06}, // 80 P
    {0x3E,0x41,0x51,0x21,0x5E}, // 81 Q
    {0x7F,0x09,0x19,0x29,0x46}, // 82 R
    {0x46,0x49,0x49,0x49,0x31}, // 83 S
    {0x01,0x01,0x7F,0x01,0x01}, // 84 T
    {0x3F,0x40,0x40,0x40,0x3F}, // 85 U
    {0x1F,0x20,0x40,0x20,0x1F}, // 86 V
    {0x3F,0x40,0x38,0x40,0x3F}, // 87 W
    {0x63,0x14,0x08,0x14,0x63}, // 88 X
    {0x07,0x08,0x70,0x08,0x07}, // 89 Y
    {0x61,0x51,0x49,0x45,0x43}, // 90 Z
    {0x00,0x7F,0x41,0x41,0x00}, // 91 [
    {0x02,0x04,0x08,0x10,0x20}, // 92 
    {0x00,0x41,0x41,0x7F,0x00}, // 93 ]
    {0x04,0x02,0x01,0x02,0x04}, // 94 ^
    {0x40,0x40,0x40,0x40,0x40}, // 95 _
    {0x00,0x01,0x02,0x04,0x00}, // 96 `
    {0x20,0x54,0x54,0x54,0x78}, // 97 a
    {0x7F,0x48,0x44,0x44,0x38}, // 98 b
    {0x38,0x44,0x44,0x44,0x20}, // 99 c
    {0x38,0x44,0x44,0x48,0x7F}, // 100 d
    {0x38,0x54,0x54,0x54,0x18}, // 101 e
    {0x08,0x7E,0x09,0x01,0x02}, // 102 f
    {0x0C,0x52,0x52,0x52,0x3E}, // 103 g
    {0x7F,0x08,0x04,0x04,0x78}, // 104 h
    {0x00,0x44,0x7D,0x40,0x00}, // 105 i
    {0x20,0x40,0x44,0x3D,0x00}, // 106 j
    {0x7F,0x10,0x28,0x44,0x00}, // 107 k
    {0x00,0x41,0x7F,0x40,0x00}, // 108 l
    {0x7C,0x04,0x18,0x04,0x78}, // 109 m
    {0x7C,0x08,0x04,0x04,0x78}, // 110 n
    {0x38,0x44,0x44,0x44,0x38}, // 111 o
    {0x7C,0x14,0x14,0x14,0x08}, // 112 p
    {0x08,0x14,0x14,0x18,0x7C}, // 113 q
    {0x7C,0x08,0x04,0x04,0x08}, // 114 r
    {0x48,0x54,0x54,0x54,0x20}, // 115 s
    {0x04,0x3F,0x44,0x40,0x20}, // 116 t
    {0x3C,0x40,0x40,0x20,0x7C}, // 117 u
    {0x1C,0x20,0x40,0x20,0x1C}, // 118 v
    {0x3C,0x40,0x30,0x40,0x3C}, // 119 w
    {0x44,0x28,0x10,0x28,0x44}, // 120 x
    {0x0C,0x50,0x50,0x50,0x3C}, // 121 y
    {0x44,0x64,0x54,0x4C,0x44}, // 122 z
    {0x00,0x08,0x36,0x41,0x00}, // 123 {
    {0x00,0x00,0x7F,0x00,0x00}, // 124 |
    {0x00,0x41,0x36,0x08,0x00}, // 125 }
    {0x10,0x08,0x08,0x10,0x08}, // 126 ~
};

bool UI::charPixel(char c, int col, int row) const {
    int idx = (unsigned char)c - 32;
    if (idx < 0 || idx >= (int)(sizeof(s_font5x7)/5)) return false;
    if (col < 0 || col >= 5 || row < 0 || row >= 7) return false;
    return (s_font5x7[idx][col] >> row) & 1;
}

void UI::initQuad() {
    float v[] = {0,0,0,0, 1,0,1,0, 1,1,1,1, 0,1,0,1};
    unsigned int idx[] = {0,1,2,2,3,0};
    glGenVertexArrays(1,&quadVAO);
    glGenBuffers(1,&quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER,quadVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STATIC_DRAW);
    unsigned int ebo; glGenBuffers(1,&ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(idx),idx,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glBindVertexArray(0);
}

void UI::init(int w, int h) { screenW=w; screenH=h; initQuad(); }
void UI::shutdown() { glDeleteVertexArrays(1,&quadVAO); glDeleteBuffers(1,&quadVBO); }

// ── Internal helpers ──────────────────────────────────────────────────────────
void UI::drawFilledRect(float x, float y, float w, float h,
                         glm::vec4 color, unsigned int sh, const glm::mat4& ortho)
{
    glUseProgram(sh);
    glm::mat4 m = glm::translate(glm::mat4(1.f),{x,y,0.f});
    m = glm::scale(m,{w,h,1.f});
    glUniformMatrix4fv(glGetUniformLocation(sh,"projection"),1,GL_FALSE,glm::value_ptr(ortho*m));
    glUniform4fv(glGetUniformLocation(sh,"color"),1,glm::value_ptr(color));
    glUniform1i(glGetUniformLocation(sh,"useTexture"),0);
    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
    glBindVertexArray(0);
}

void UI::drawCharBlock(char c, float x, float y, float sz,
                        glm::vec4 col, unsigned int sh, const glm::mat4& ortho)
{
    for (int col_=0;col_<5;col_++)
        for (int row=0;row<7;row++)
            if (charPixel(c,col_,row))
                drawFilledRect(x+col_*sz, y+row*sz, sz-0.5f, sz-0.5f, col, sh, ortho);
}

void UI::drawText(const std::string& text, float x, float y,
                   float scale, glm::vec4 color,
                   unsigned int sh, const glm::mat4& ortho)
{
    float cx = x;
    for (char c : text) {
        if (c == '\n') { cx=x; y+=8*scale; continue; }
        drawCharBlock(c, cx, y, scale, color, sh, ortho);
        cx += 6*scale;
    }
}

// ── Public draw calls ─────────────────────────────────────────────────────────
void UI::drawCrosshair(unsigned int sh, const glm::mat4& ortho) {
    float cx=screenW/2.f, cy=screenH/2.f;
    drawFilledRect(cx-10,cy-1,20,2,{1,1,1,0.9f},sh,ortho);
    drawFilledRect(cx-1,cy-10,2,20,{1,1,1,0.9f},sh,ortho);
}

void UI::drawInteractPrompt(const std::string& msg, unsigned int sh, const glm::mat4& ortho) {
    float scale = 2.0f;
    float w = msg.size() * (6.f * scale) + 10.f; // 6 pixels per char
    float x = (screenW-w)/2.f, y = screenH*0.65f;
    drawFilledRect(x-5,y-5,w+10,8.f*scale + 10.f,{0,0,0,0.6f},sh,ortho);
    drawText(msg, x, y, scale, {1,0.9f,0.4f,1.f},sh,ortho);
}

void UI::drawInventory(const Inventory& inv, unsigned int sh, const glm::mat4& ortho) {
    int n = inv.size();
    if (n==0) return;
    float slotW=48, slotH=48, gap=6;
    float totalW = n*(slotW+gap);
    float startX = (screenW-totalW)/2.f;
    float y = screenH-62;

    for (int i=0;i<n;i++) {
        float x = startX + i*(slotW+gap);
        drawFilledRect(x,y,slotW,slotH,{0.1f,0.1f,0.15f,0.8f},sh,ortho);
        drawFilledRect(x+1,y+1,slotW-2,slotH-2,{0.25f,0.20f,0.35f,0.8f},sh,ortho);
        const auto& item = inv.items()[i];
        std::string label = item.name;
        // center text roughly inside the 48x48 box, scaled down to fit
        float scale = label.size() > 5 ? 1.0f : 1.5f;
        float textW = label.size() * (6.f * scale);
        float tx = x + (slotW - textW)/2.f;
        drawText(label, tx, y+20, scale, {1,0.9f,0.5f,1.f},sh,ortho);
    }
}

void UI::drawCodePad(const std::string& input, unsigned int sh, const glm::mat4& ortho) {
    float pw=280,ph=200;
    float px=(screenW-pw)/2.f, py=(screenH-ph)/2.f;
    drawFilledRect(px,py,pw,ph,{0.05f,0.05f,0.1f,0.92f},sh,ortho);
    drawFilledRect(px+2,py+2,pw-4,ph-4,{0.12f,0.08f,0.18f,0.9f},sh,ortho);
    drawText("CODE PAD", px+80,py+12, 2.5f, {0.4f,0.8f,1.f,1.f},sh,ortho);
    drawText("Enter 4-digit code:", px+20,py+50, 2.f, {0.8f,0.8f,0.8f,1.f},sh,ortho);

    // Display current input as boxes
    for (int i=0;i<4;i++) {
        float bx=px+40+i*55, by=py+90;
        drawFilledRect(bx,by,45,45,{0.2f,0.2f,0.3f,1.f},sh,ortho);
        if (i<(int)input.size()) {
            std::string d(1,input[i]);
            drawText(d, bx+13,by+10, 4.f, {1.f,0.9f,0.2f,1.f},sh,ortho);
        }
    }
    drawText("[0-9] Type  [Enter] Confirm  [Esc] Cancel",
             px+8,py+155, 1.5f, {0.5f,0.5f,0.5f,1.f},sh,ortho);
}

void UI::drawSequencePanel(int litIndex, int total, unsigned int sh, const glm::mat4& ortho) {
    float pw=320, ph=120;
    float px=(screenW-pw)/2.f, py=(screenH-ph)/2.f;
    drawFilledRect(px,py,pw,ph,{0.05f,0.05f,0.1f,0.9f},sh,ortho);
    drawText("WATCH THE SEQUENCE", px+30,py+10, 2.f,{1.f,0.7f,0.1f,1.f},sh,ortho);
    for (int i=0;i<total;i++) {
        float bx=px+20+i*70, by=py+50;
        bool lit=(i==litIndex);
        drawFilledRect(bx,by,55,45,
            lit ? glm::vec4{0.2f,0.9f,0.2f,1.f} : glm::vec4{0.15f,0.15f,0.2f,1.f},
            sh,ortho);
        std::string n=std::to_string(i+1);
        drawText(n, bx+18,by+12, 3.f, lit?glm::vec4{0,0,0,1}:glm::vec4{0.5f,0.5f,0.5f,1},sh,ortho);
    }
}

void UI::drawHUD(float fps, float timerLeft, unsigned int sh, const glm::mat4& ortho) {
    // FPS
    std::string fpsStr = "FPS:" + std::to_string((int)fps);
    drawText(fpsStr, 8, 8, 1.8f, {0.4f,1.f,0.4f,0.8f}, sh, ortho);

    // Timer
    int minutes = (int)(timerLeft)/60;
    int seconds = (int)(timerLeft)%60;
    char buf[32];
    snprintf(buf,sizeof(buf),"TIME %02d:%02d",minutes,seconds);
    drawFilledRect(screenW-160,4,155,22,{0,0,0,0.5f},sh,ortho);
    drawText(buf, screenW-155, 6, 1.8f,
        timerLeft<60?glm::vec4{1,0.2f,0.2f,1}:glm::vec4{0.9f,0.9f,0.5f,1},sh,ortho);

    // Room name
    drawText("ENIGMA", (screenW-50*1.8f)/2.f, 6, 1.8f, {0.5f,0.4f,0.7f,0.7f},sh,ortho);
}

void UI::drawWinScreen(unsigned int sh, const glm::mat4& ortho) {
    drawFilledRect(0,0,(float)screenW,(float)screenH,{0,0.05f,0,0.82f},sh,ortho);
    drawText("YOU ESCAPED!", screenW/2.f-90, screenH/2.f-40, 4.f,{0.2f,1.f,0.3f,1.f},sh,ortho);
    drawText("Congratulations! Press R to restart.", screenW/2.f-160,screenH/2.f+30,2.f,{0.8f,0.8f,0.8f,1},sh,ortho);
}

void UI::drawLoseScreen(unsigned int sh, const glm::mat4& ortho) {
    drawFilledRect(0,0,(float)screenW,(float)screenH,{0.08f,0,0,0.85f},sh,ortho);
    drawText("TIME IS UP!", screenW/2.f-80, screenH/2.f-40, 4.f,{1.f,0.2f,0.2f,1.f},sh,ortho);
    drawText("You failed to escape. Press R to retry.", screenW/2.f-170,screenH/2.f+30,2.f,{0.8f,0.5f,0.5f,1},sh,ortho);
}

void UI::drawPauseMenu(unsigned int sh, const glm::mat4& ortho) {
    drawFilledRect(0,0,(float)screenW,(float)screenH,{0,0,0,0.65f},sh,ortho);
    drawText("PAUSED",       screenW/2.f-45,screenH/2.f-70,4.f,{1,1,1,1},sh,ortho);
    drawText("[Esc] Resume", screenW/2.f-70,screenH/2.f,   2.f,{0.8f,0.8f,0.5f,1},sh,ortho);
    drawText("[S]   Save",   screenW/2.f-70,screenH/2.f+30,2.f,{0.6f,0.9f,0.6f,1},sh,ortho);
    drawText("[L]   Load",   screenW/2.f-70,screenH/2.f+60,2.f,{0.6f,0.6f,0.9f,1},sh,ortho);
    drawText("[Q]   Quit",   screenW/2.f-70,screenH/2.f+90,2.f,{0.9f,0.4f,0.4f,1},sh,ortho);
}

void UI::drawMainMenu(unsigned int sh, const glm::mat4& ortho) {
    drawFilledRect(0,0,(float)screenW,(float)screenH,{0.02f,0.01f,0.04f,1.f},sh,ortho);
    // Title
    drawText("ENIGMA",  screenW/2.f-60, screenH/2.f-120, 6.f,{0.7f,0.3f,1.f,1.f},sh,ortho);
    drawText("A Mystery in the Dark", screenW/2.f-130,screenH/2.f-50,2.f,{0.5f,0.4f,0.6f,1},sh,ortho);
    drawText("[Enter] Start Game",    screenW/2.f-90, screenH/2.f+20,2.5f,{0.9f,0.8f,0.3f,1},sh,ortho);
    drawText("[L]     Load Save",     screenW/2.f-90, screenH/2.f+60,2.f, {0.4f,0.7f,0.4f,1},sh,ortho);
    drawText("[Esc]   Quit",          screenW/2.f-90, screenH/2.f+95,2.f, {0.7f,0.3f,0.3f,1},sh,ortho);
    drawText("WASD=Move  Mouse=Look  E=Interact  Ctrl=Crouch  Space=Jump",
             screenW/2.f-260, screenH-40, 1.8f, {0.4f,0.4f,0.5f,1},sh,ortho);
}

void UI::drawHintPanel(const std::string& hint, unsigned int sh, const glm::mat4& ortho) {
    if (hint.empty()) return;
    float pw=400, ph=60;
    float px=(screenW-pw)/2.f, py=screenH*0.15f;
    drawFilledRect(px,py,pw,ph,{0.05f,0.05f,0.1f,0.85f},sh,ortho);
    drawText("HINT: "+hint, px+10,py+15, 1.8f,{0.4f,0.9f,1.f,1.f},sh,ortho);
}
