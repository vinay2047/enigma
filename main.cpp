// EscapeRoom — main.cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

#include "shader.h"
#include "player.h"
#include "collision.h"
#include "lighting.h"
#include "inventory.h"
#include "puzzle.h"
#include "room.h"
#include "renderer.h"
#include "particle.h"
#include "ui.h"
#include "saveload.h"

// ── Constants ─────────────────────────────────────────────────────────────────
static const int SCR_W = 1280, SCR_H = 720;
static const float INTERACT_DIST = 5.0f;
static const float GAME_TIMER    = 600.f; // 10 minutes

// ── Global state ──────────────────────────────────────────────────────────────
static GameState  g_state       = GameState::MENU;
static Player     g_player;
static Inventory  g_inv;
static PuzzleManager g_puzzles;
static SceneManager  g_scene;
static Renderer   g_renderer;
static ParticleSystem g_particles;
static UI         g_ui;
static SaveData   g_save;

static float g_timer      = GAME_TIMER;
static float g_fps        = 0.f;
static float g_lastX      = SCR_W/2.f, g_lastY = SCR_H/2.f;
static bool  g_firstMouse = true;
static bool  g_mouseLocked= false;

// Code pad state
static std::string g_codeInput;
// Sequence state
static std::vector<int> g_seqInput;
static bool g_showingSeq = false;
static float g_seqTimer  = 0.f;
// Active puzzle id for UI
static int g_activePuzzle = -1;
// Interaction message
static std::string g_interactMsg;
static float g_msgTimer = 0.f;
// Hint
static std::string g_hint;
static float g_hintTimer = 0.f;

// ── Puzzle setup ──────────────────────────────────────────────────────────────
static void setupPuzzles()
{
    // 0: Hidden Button → opens drawer, spawns brass key
    {
        Puzzle p; p.type=PuzzleType::HIDDEN_BUTTON;
        p.hint="Something is hidden behind the painting...";
        p.onSolve=[&]{
            // Show hidden button, reveal key
            Room& r = g_scene.rooms[0];
            for(auto& o:r.objects){
                if(o.name=="HiddenButton") o.visible=true;
                if(o.name=="BrassKey")     o.visible=true;
            }
            g_hint="The painting swung open! Check behind it.";
            g_hintTimer=5.f;
        };
        g_puzzles.addPuzzle(p);
    }
    // 1: Key Lock (Entry Hall door)
    {
        Puzzle p; p.type=PuzzleType::KEY_LOCK;
        p.hint="You need the brass key for this door.";
        p.onSolve=[&]{
            Room& r=g_scene.rooms[0];
            for(auto& o:r.objects) if(o.name=="DoorToStudy"){ o.isOpen=true; }
            g_scene.changeRoom(1);
            g_player.position={-4.5f,0.f,0.f};
            g_hint="The door swings open. You enter the Study Room.";
            g_hintTimer=5.f;
        };
        g_puzzles.addPuzzle(p);
    }
    // 2: Light Sequence (Study Room panel)
    {
        Puzzle p; p.type=PuzzleType::LIGHT_SEQUENCE;
        p.sequence={0,2,1,3};
        p.hint="Watch the lights, then press them in the same order.";
        p.onSolve=[&]{
            Room& r=g_scene.rooms[1];
            for(auto& o:r.objects) if(o.name=="CodeNote") o.visible=true;
            g_hint="A note appeared on the desk!";
            g_hintTimer=5.f;
        };
        g_puzzles.addPuzzle(p);
    }
    // 3: Lever Sequence
    {
        Puzzle p; p.type=PuzzleType::LEVER_SEQUENCE;
        p.sequence={0,2,1}; // Left, Right, Middle
        p.leverPulled={false,false,false};
        p.hint="The book says: Left, Right, Middle.";
        p.onSolve=[&]{
            Room& r=g_scene.rooms[1];
            for(auto& o:r.objects) if(o.name=="PressurePlate") o.visible=true;
            g_hint="A pressure plate rises from the floor!";
            g_hintTimer=5.f;
        };
        g_puzzles.addPuzzle(p);
    }
    // 4: Pressure Plate (Study) - single step, stepping on it
    {
        Puzzle p; p.type=PuzzleType::PRESSURE_PLATE;
        p.sequence={0};
        p.hint="Step on the pressure plate.";
        p.onSolve=[&]{
            // Unlock code pad door
            g_hint="A mechanism clicks. The vault door code pad activates!";
            g_hintTimer=5.f;
        };
        g_puzzles.addPuzzle(p);
    }
    // 5: Code Pad (Vault door)
    {
        Puzzle p; p.type=PuzzleType::CODE_PAD;
        p.solution="4821";
        p.hint="The code is written on the note you found.";
        p.onSolve=[&]{
            Room& r=g_scene.rooms[1];
            for(auto& o:r.objects) if(o.name=="DoorToVault"){ o.isOpen=true; }
            g_scene.changeRoom(2);
            g_player.position={-4.f,0.f,0.f};
            g_hint="The vault door opens. Enter The Vault!";
            g_hintTimer=6.f;
        };
        g_puzzles.addPuzzle(p);
    }
    // 6: Mirror + Pressure plates in vault → final exit
    {
        Puzzle p; p.type=PuzzleType::MIRROR_LIGHT;
        p.requiredAngle=45.f;
        p.hint="Rotate the mirror [Q/E] to direct light at the sensor.";
        p.onSolve=[&]{
            Room& r=g_scene.rooms[2];
            for(auto& o:r.objects) if(o.name=="FinalDoor") o.isOpen=true;
            g_state=GameState::WIN;
        };
        g_puzzles.addPuzzle(p);
    }
}

// ── Pick nearest interactable object ─────────────────────────────────────────
static GameObject* pickObject(float maxDist)
{
    Room& room = g_scene.activeRoom();
    glm::vec3 eye = g_player.eyePos();
    glm::vec3 dir = g_player.front;
    float best = maxDist;
    GameObject* hit = nullptr;
    for(auto& obj : room.objects){
        if(!obj.visible) continue;
        if(obj.interaction==InteractionType::NONE) continue;
        float t = rayAABB(eye, dir, obj.aabb);
        if(t>0.f && t<best){ best=t; hit=&obj; }
    }
    return hit;
}

// ── Handle E-key interaction ──────────────────────────────────────────────────
static void doInteract()
{
    GameObject* obj = pickObject(INTERACT_DIST);
    if(!obj) {
        g_interactMsg = "Nothing to interact with nearby.";
        g_msgTimer = 2.f;
        return;
    }

    switch(obj->interaction)
    {
    case InteractionType::PICKUP:
        if(obj->givesItem!=ItemType::NONE){
            g_inv.addItem(obj->givesItem);
            obj->visible=false;
            obj->solid=false;
            g_interactMsg="Picked up: "+Inventory::nameOf(obj->givesItem);
            g_msgTimer=3.f;
        }
        break;

    case InteractionType::INSPECT:
        g_interactMsg=obj->interactMsg;
        g_msgTimer=4.f;
        break;

    case InteractionType::PUZZLE:
        {
            int pid=obj->puzzleId;
            Puzzle* pz=g_puzzles.get(pid);
            if(!pz||pz->isSolved()){
                g_interactMsg="Already solved."; g_msgTimer=2.f; return;
            }
            if(pz->type==PuzzleType::HIDDEN_BUTTON){
                if(obj->name=="Painting"){
                    // Reveal hidden button
                    Room& r=g_scene.rooms[0];
                    for(auto& o:r.objects) if(o.name=="HiddenButton") o.visible=true;
                    g_interactMsg="You move the painting aside...";
                    g_msgTimer=3.f;
                } else {
                    g_puzzles.interactHiddenButton(pid);
                }
            }
            else if(pz->type==PuzzleType::LIGHT_SEQUENCE){
                g_activePuzzle=pid;
                if(!g_showingSeq){
                    g_puzzles.startLightSequence(pid);
                    g_showingSeq=true; g_seqTimer=0.f;
                    g_state=GameState::SEQUENCE_UI;
                } else {
                    // player presses a button (index from name)
                    int idx = obj->name.back()-'0';
                    g_seqInput.push_back(idx);
                    if((int)g_seqInput.size()==(int)g_puzzles.get(pid)->sequence.size()){
                        g_puzzles.submitLightSequence(pid,g_seqInput);
                        g_seqInput.clear(); g_showingSeq=false;
                        g_state=GameState::PLAYING;
                    }
                }
            }
            else if(pz->type==PuzzleType::CODE_PAD){
                g_activePuzzle=pid;
                g_codeInput.clear();
                g_state=GameState::CODE_PAD_UI;
            }
            else if(pz->type==PuzzleType::LEVER_SEQUENCE){
                int idx=obj->name.back()-'0';
                bool solved=g_puzzles.interactLeverSequence(pid,idx);
                g_interactMsg=solved?"Correct sequence!":"Lever pulled.";
                g_msgTimer=2.f;
            }
            else if(pz->type==PuzzleType::PRESSURE_PLATE){
                g_puzzles.interactPressurePlate(pid,0);
            }
            else if(pz->type==PuzzleType::MIRROR_LIGHT){
                g_activePuzzle=pid;
                g_interactMsg="[Q] rotate left  [E] rotate right  [F] done";
                g_msgTimer=4.f;
            }
        }
        break;

    case InteractionType::USE_KEY:
        break;

    case InteractionType::DOOR:
        {
            int pid=obj->puzzleId;
            Puzzle* pz=g_puzzles.get(pid);
            if(pz && pz->type==PuzzleType::KEY_LOCK){
                if(!g_inv.hasItem(obj->requiredItem)){
                    g_interactMsg=obj->interactMsg; g_msgTimer=3.f;
                } else {
                    g_inv.removeItem(obj->requiredItem);
                    g_puzzles.interactKeyLock(pid,true);
                }
            } else if(obj->isOpen){
                g_interactMsg="The door is open."; g_msgTimer=1.5f;
            } else {
                g_interactMsg=obj->interactMsg; g_msgTimer=3.f;
            }
        }
        break;

    default: break;
    }
}

// ── Pressure plate auto-detection ─────────────────────────────────────────────
static void checkPressurePlates()
{
    Room& room=g_scene.activeRoom();
    glm::vec3 feet=g_player.position;
    for(auto& obj:room.objects){
        if(obj.interaction!=InteractionType::PUZZLE) continue;
        Puzzle* pz=g_puzzles.get(obj.puzzleId);
        if(!pz||pz->type!=PuzzleType::PRESSURE_PLATE||pz->isSolved()) continue;
        if(obj.aabb.containsPoint(feet)){
            int idx=obj.name.back()-'0';
            g_puzzles.interactPressurePlate(obj.puzzleId,idx);
        }
    }
}

// ── GLFW callbacks ─────────────────────────────────────────────────────────────
static void framebufferCallback(GLFWwindow*, int w, int h){
    glViewport(0,0,w,h);
}

static void mouseCallback(GLFWwindow* win, double xpos, double ypos)
{
    if(g_state!=GameState::PLAYING) return;
    if(g_firstMouse){ g_lastX=(float)xpos; g_lastY=(float)ypos; g_firstMouse=false; }
    float dx=(float)xpos-g_lastX, dy=(float)ypos-g_lastY;
    g_lastX=(float)xpos; g_lastY=(float)ypos;
    g_player.processMouse(dx,dy);
}

static void keyCallback(GLFWwindow* win, int key, int, int action, int)
{
    if(action==GLFW_PRESS){
        // Code pad input
        if(g_state==GameState::CODE_PAD_UI){
            if(key>=GLFW_KEY_0 && key<=GLFW_KEY_9 && g_codeInput.size()<4)
                g_codeInput+=(char)('0'+(key-GLFW_KEY_0));
            if(key==GLFW_KEY_BACKSPACE && !g_codeInput.empty())
                g_codeInput.pop_back();
            if(key==GLFW_KEY_ENTER){
                bool ok=g_puzzles.interactCodePad(g_activePuzzle,g_codeInput);
                g_interactMsg=ok?"Code accepted!":"Wrong code. Try again.";
                g_msgTimer=3.f;
                if(ok){ g_state=GameState::PLAYING; g_codeInput.clear(); }
                else g_codeInput.clear();
            }
            if(key==GLFW_KEY_ESCAPE){ g_state=GameState::PLAYING; g_codeInput.clear(); }
            return;
        }
        switch(key){
        case GLFW_KEY_ESCAPE:
            if(g_state==GameState::PLAYING){ g_state=GameState::PAUSED; glfwSetInputMode(win,GLFW_CURSOR,GLFW_CURSOR_NORMAL); }
            else if(g_state==GameState::PAUSED){ g_state=GameState::PLAYING; glfwSetInputMode(win,GLFW_CURSOR,GLFW_CURSOR_DISABLED); }
            else if(g_state==GameState::SEQUENCE_UI){ g_state=GameState::PLAYING; g_showingSeq=false; }
            break;
        case GLFW_KEY_ENTER:
            if(g_state==GameState::MENU){
                g_state=GameState::PLAYING;
                glfwSetInputMode(win,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
                g_firstMouse=true;
            }
            break;
        case GLFW_KEY_E:
            if(g_state==GameState::PLAYING) doInteract();
            break;
        case GLFW_KEY_H:
            if(g_state==GameState::PLAYING){
                // Show hint for nearest puzzle
                GameObject* o=pickObject(INTERACT_DIST);
                if(o && o->puzzleId>=0){
                    Puzzle* pz=g_puzzles.get(o->puzzleId);
                    if(pz){ g_hint=pz->hint; g_hintTimer=5.f; }
                }
            }
            break;
        case GLFW_KEY_S:
            if(g_state==GameState::PAUSED){
                SaveLoad::save("escape_save.dat",g_save);
                g_interactMsg="Game saved!"; g_msgTimer=2.f;
            }
            break;
        case GLFW_KEY_L:
            if(g_state==GameState::PAUSED||g_state==GameState::MENU){
                if(SaveLoad::load("escape_save.dat",g_save)){
                    g_player.position={g_save.playerX,g_save.playerY,g_save.playerZ};
                    g_scene.changeRoom(g_save.currentRoom);
                    g_timer=g_save.timerLeft;
                    g_state=GameState::PLAYING;
                    glfwSetInputMode(win,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
                }
            }
            break;
        case GLFW_KEY_R:
            if(g_state==GameState::WIN||g_state==GameState::LOSE){
                // restart
                g_state=GameState::MENU;
                g_timer=GAME_TIMER;
                g_player.position={0,0,2.5f};
                g_scene.currentRoom=0;
                g_inv=Inventory{};
                glfwSetInputMode(win,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
            }
            break;
        case GLFW_KEY_Q:
            if(g_state==GameState::PAUSED) glfwSetWindowShouldClose(win,true);
            // Mirror rotate
            if(g_state==GameState::PLAYING && g_activePuzzle==6)
                g_puzzles.interactMirror(6,-5.f);
            break;
        default: break;
        }
    }
}

// ── Collect solid AABBs for collision ─────────────────────────────────────────
static std::vector<AABB> getRoomWalls()
{
    std::vector<AABB> walls;
    for(const auto& obj: g_scene.activeRoom().objects)
        if(obj.solid && obj.visible) walls.push_back(obj.aabb);
    return walls;
}

// ── Main ──────────────────────────────────────────────────────────────────────
int main()
{
    // Init GLFW
    if(!glfwInit()){ std::cerr<<"GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES,4);

    GLFWwindow* win=glfwCreateWindow(SCR_W,SCR_H,"Escape Room",nullptr,nullptr);
    if(!win){ std::cerr<<"Window failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win,framebufferCallback);
    glfwSetCursorPosCallback(win,mouseCallback);
    glfwSetKeyCallback(win,keyCallback);
    glfwSwapInterval(1);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cerr<<"GLAD failed\n"; return -1;
    }
    glEnable(GL_MULTISAMPLE);
    glViewport(0,0,SCR_W,SCR_H);

    // Init systems
    g_renderer.init(SCR_W,SCR_H);
    g_ui.init(SCR_W,SCR_H);
    g_particles.init();
    g_scene.buildScene();
    setupPuzzles();

    g_player.position={0.f,0.f,2.5f};
    g_player.yaw=-90.f;

    // Emit ambient particles in entry hall
    g_particles.emit({0,1.5f,0},80,{0.6f,0.6f,0.7f},0.05f,8.f);

    float lastTime=(float)glfwGetTime();

    // ── Game loop ─────────────────────────────────────────────────────────────
    while(!glfwWindowShouldClose(win))
    {
        float now=(float)glfwGetTime();
        float dt=std::min(now-lastTime,0.05f);
        lastTime=now;
        g_fps = 1.f/dt;

        glfwPollEvents();

        // ── Update ─────────────────────────────────────────────────────────
        if(g_state==GameState::PLAYING)
        {
            bool W=glfwGetKey(win,GLFW_KEY_W)==GLFW_PRESS;
            bool S=glfwGetKey(win,GLFW_KEY_S)==GLFW_PRESS;
            bool A=glfwGetKey(win,GLFW_KEY_A)==GLFW_PRESS;
            bool D=glfwGetKey(win,GLFW_KEY_D)==GLFW_PRESS;
            bool JP=glfwGetKey(win,GLFW_KEY_SPACE)==GLFW_PRESS;
            bool CR=glfwGetKey(win,GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS;

            glm::vec3 prevPos=g_player.position;
            g_player.processKeyboard(W,S,A,D,JP,CR,dt);
            g_player.update(dt);

            // Collision
            auto walls=getRoomWalls();
            glm::vec3 half={g_player.extent.w,g_player.extent.h*0.5f,g_player.extent.w};
            g_player.position=resolveCollision(g_player.position,half,walls.data(),(int)walls.size());

            // Pressure plates
            checkPressurePlates();

            // Lighting flicker
            g_scene.activeRoom().lighting.update(dt);

            // Particles
            g_particles.update(dt);
            if((int)g_particles.particles.size()<80)
                g_particles.emit(g_player.position+glm::vec3{0,1.f,0},3);

            // Timer
            g_timer-=dt;
            if(g_timer<=0.f){ g_timer=0.f; g_state=GameState::LOSE; }

            // Sequence auto-advance
            if(g_showingSeq && g_activePuzzle>=0){
                g_puzzles.updateLightSequence(g_activePuzzle,dt);
                if(g_puzzles.lightSequenceCurrentStep(g_activePuzzle)==-1){
                    g_showingSeq=false;  // player's turn — stay in SEQUENCE_UI
                }
            }

            // Update prompt timers
            if(g_msgTimer>0.f) g_msgTimer-=dt;
            if(g_hintTimer>0.f) g_hintTimer-=dt;

            // Save data live
            g_save.playerX=g_player.position.x;
            g_save.playerY=g_player.position.y;
            g_save.playerZ=g_player.position.z;
            g_save.currentRoom=g_scene.currentRoom;
            g_save.timerLeft=g_timer;
            g_save.hasKeyBrass=g_inv.hasItem(ItemType::KEY_BRASS);
            g_save.hasKeyIron =g_inv.hasItem(ItemType::KEY_IRON);
            g_save.hasCodeNote=g_inv.hasItem(ItemType::CODE_NOTE);
            g_save.hasBookClue=g_inv.hasItem(ItemType::BOOK_CLUE);

            // Door animations
            for(auto& obj: g_scene.activeRoom().objects)
                if(obj.isDoor) obj.updateDoorAnim(dt);
        }

        // ── Render ─────────────────────────────────────────────────────────
        glClearColor(0.01f,0.01f,0.02f,1.f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glm::mat4 view=g_player.getViewMatrix();
        glm::mat4 proj=glm::perspective(glm::radians(70.f),(float)SCR_W/SCR_H,0.05f,100.f);
        glm::mat4 ortho=glm::ortho(0.f,(float)SCR_W,(float)SCR_H,0.f,-1.f,1.f);

        if(g_state!=GameState::MENU){
            g_renderer.renderRoom(g_scene.activeRoom(),view,proj,g_player.eyePos());
            g_particles.render(g_renderer.particleShader.ID,view,proj);
        }

        // ── 2D UI ──────────────────────────────────────────────────────────
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

        unsigned int shID=g_renderer.uiShader.ID;

        switch(g_state){
        case GameState::MENU:
            g_ui.drawMainMenu(shID,ortho);
            break;
        case GameState::PLAYING:
        case GameState::SEQUENCE_UI:
            g_ui.drawCrosshair(shID,ortho);
            g_ui.drawHUD(g_fps,g_timer,shID,ortho);
            g_ui.drawInventory(g_inv,shID,ortho);
            if(g_msgTimer>0.f) g_ui.drawInteractPrompt(g_interactMsg,shID,ortho);
            if(g_hintTimer>0.f) g_ui.drawHintPanel(g_hint,shID,ortho);
            // Sequence overlay
            if(g_state==GameState::SEQUENCE_UI && g_activePuzzle>=0){
                int lit=g_puzzles.lightSequenceCurrentStep(g_activePuzzle);
                g_ui.drawSequencePanel(lit,4,shID,ortho);
            }
            // Interaction prompt if looking at object
            if(g_msgTimer<=0.f){
                GameObject* obj=pickObject(INTERACT_DIST);
                if(obj) g_ui.drawInteractPrompt("[E] "+obj->interactMsg,shID,ortho);
            }
            break;
        case GameState::CODE_PAD_UI:
            g_ui.drawCrosshair(shID,ortho);
            g_ui.drawCodePad(g_codeInput,shID,ortho);
            break;
        case GameState::PAUSED:
            g_ui.drawPauseMenu(shID,ortho);
            break;
        case GameState::WIN:
            g_ui.drawWinScreen(shID,ortho);
            break;
        case GameState::LOSE:
            g_ui.drawLoseScreen(shID,ortho);
            break;
        default: break;
        }

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(win);
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    g_renderer.shutdown();
    g_particles.shutdown();
    g_ui.shutdown();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
