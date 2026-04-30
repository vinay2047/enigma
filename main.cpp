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
#include <random>
#include <chrono>

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
static float g_winTimer = -1.f;

// ── Puzzle setup ──────────────────────────────────────────────────────────────
static void setupPuzzles()
{
    // Random engine seeded with current time
    std::mt19937 rng(static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count()));

    // 0: Hidden Button → opens drawer, spawns brass key
    {
        Puzzle p; p.type=PuzzleType::HIDDEN_BUTTON;
        p.hint="Something is hidden behind the painting...";
        p.onSolve=[&]{
            Room& r = g_scene.rooms[0];
            for(auto& o:r.objects){
                if(o.name=="Drawer"){
                    // Slide drawer forward (out of cabinet) by updating position
                    o.transform.position.z = -0.65f;   // was -0.95, now pulled out
                    glm::vec3 half = o.transform.scale * 0.5f;
                    o.aabb = AABB::fromCenter(o.transform.position, half);
                    o.interactMsg = "The drawer is open!";
                }
                // Reveal the key floating visibly in front of the open drawer
                if(o.name=="BrassKey") o.visible=true;
            }
            g_interactMsg="The drawer springs open! A brass key is inside.";
            g_msgTimer=5.f;
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
            g_interactMsg="The door swings open. You enter the Study Room.";
            g_msgTimer=5.f;
        };
        g_puzzles.addPuzzle(p);
    }
    // 2: Light Sequence (Study Room panel)
    {
        Puzzle p; p.type=PuzzleType::LIGHT_SEQUENCE;
        p.sequence={0,1,2,3};
        std::shuffle(p.sequence.begin(), p.sequence.end(), rng);
        p.hint="Watch the lights, then press them in the same order.";
        p.onSolve=[&]{
            Room& r=g_scene.rooms[1];
            for(auto& o:r.objects) if(o.name=="CodeNote") o.visible=true;
            g_interactMsg="A note appeared on the desk!";
            g_msgTimer=5.f;
        };
        g_puzzles.addPuzzle(p);
    }
    // 3: Lever Sequence
    {
        Puzzle p; p.type=PuzzleType::LEVER_SEQUENCE;
        p.sequence={0,1,2};
        std::shuffle(p.sequence.begin(), p.sequence.end(), rng);
        p.leverPulled={false,false,false};
        // Build hint from shuffled sequence
        static const char* leverNames[] = {"Left", "Middle", "Right"};
        std::string hintStr = "The book says: ";
        for (int i = 0; i < (int)p.sequence.size(); i++) {
            if (i > 0) hintStr += ", ";
            hintStr += leverNames[p.sequence[i]];
        }
        hintStr += ".";
        p.hint = hintStr;
        // Update the book in the study room with the new order
        Room& studyRoom = g_scene.rooms[1];
        for (auto& o : studyRoom.objects) {
            if (o.name == "BookClue") {
                o.interactMsg = "The book reads: '" + hintStr.substr(15) + "'";
            }
        }
        p.onSolve=[&]{
            Room& r=g_scene.rooms[1];
            for(auto& o:r.objects) if(o.name=="PressurePlate") o.visible=true;
            g_interactMsg="A pressure plate rises from the floor!";
            g_msgTimer=5.f;
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
            Room& r=g_scene.rooms[1];
            for (auto& o : r.objects) {
                if (o.name == "DoorToVault") {
                    o.interaction = InteractionType::PUZZLE;
                    o.interactMsg = "A keypad lock. Enter the 4-digit code.";
                }
            }
            g_interactMsg="A mechanism clicks. The vault door code pad activates!";
            g_msgTimer=5.f;
        };
        g_puzzles.addPuzzle(p);
    }
    // 5: Code Pad (Vault door)
    {
        Puzzle p; p.type=PuzzleType::CODE_PAD;
        // Generate random 4-digit code
        std::uniform_int_distribution<int> dist(1000, 9999);
        int code = dist(rng);
        p.solution = std::to_string(code);
        p.hint="The code is written on the note you found.";
        // Update the code note in the study room
        Room& studyRoom = g_scene.rooms[1];
        for (auto& o : studyRoom.objects) {
            if (o.name == "CodeNote") {
                o.interactMsg = "A scrap of paper with numbers: " + p.solution;
            }
        }
        p.onSolve=[&]{
            Room& r=g_scene.rooms[1];
            for(auto& o:r.objects) if(o.name=="DoorToVault"){ o.isOpen=true; }
            g_scene.changeRoom(2);
            g_player.position={-4.f,0.f,0.f};
            g_interactMsg="The vault door opens. Enter The Vault!";
            g_msgTimer=6.f;
        };
        g_puzzles.addPuzzle(p);
    }
    
    {
        Puzzle p; p.type=PuzzleType::LEVER_SEQUENCE;
        p.sequence={0,1,2,3}; 
        std::shuffle(p.sequence.begin(), p.sequence.end(), rng);
        p.leverPulled={false,false,false,false};
       
        static const char* dirNames[] = {"North", "East", "West", "South"};
        std::string hintStr = "The compass says: ";
        for (int i = 0; i < (int)p.sequence.size(); i++) {
            if (i > 0) hintStr += ", ";
            hintStr += dirNames[p.sequence[i]];
        }
        hintStr += ".";
        p.hint = hintStr;
        
        Room& vaultRoom = g_scene.rooms[2];
        for (auto& o : vaultRoom.objects) {
            if (o.name == "RiddleTablet") {
                o.interactMsg = "A compass is etched here. It says: 'Follow the winds: " + hintStr.substr(18) + "'";
            }
        }
        p.onSolve=[&]{
            Room& r=g_scene.rooms[2];
            for(auto& o:r.objects) {
                if(o.name=="IronKey") o.visible=true;
            }
            g_interactMsg="The buttons lock in place! A key appears on the table!";
            g_msgTimer=5.f;
        };
        g_puzzles.addPuzzle(p);
    }
    // 7: Key Lock (Vault Chest) — requires Iron Key
    {
        Puzzle p; p.type=PuzzleType::KEY_LOCK;
        p.hint="You need the iron key to open this chest.";
        p.onSolve=[&]{
            Room& r=g_scene.rooms[2];
            for(auto& o:r.objects) {
                if(o.name=="RedCrystal") o.visible=true;
                if(o.name=="LockedChest") {
                    o.isDoor=true; // enable animation update
                    o.isOpen=true;
                    o.interactMsg="The chest is open.";
                }
            }
            g_interactMsg="The chest creaks open! A crimson crystal glows inside!";
            g_msgTimer=5.f;
        };
        g_puzzles.addPuzzle(p);
    }
    // 8: Crystal Altar (Vault) — place red crystal to activate laser
    {
        Puzzle p; p.type=PuzzleType::HIDDEN_BUTTON;
        p.hint="Place the crystal from the chest upon the altar.";
        p.onSolve=[&]{
            Room& r=g_scene.rooms[2];
            g_inv.removeItem(ItemType::CRYSTAL_RED);
            for(auto& o:r.objects) {
                if(o.name=="CrystalSlot") o.material.color={0.9f,0.15f,0.1f};
                if(o.name=="LightEmitter" || o.name=="EmitterLens") o.visible=true;
                if(o.name=="LaserBeam") o.visible=true;
                if(o.name=="Mirror" || o.name=="MirrorStand") o.visible=true;
            }
            if(!r.lighting.lights.empty()) {
                r.lighting.lights[0].baseIntensity=4.f;
                r.lighting.lights[0].intensity=4.f;
                r.lighting.lights[0].color={0.5f,0.55f,0.8f};
            }
            g_interactMsg="The crystal blazes with power! A laser beam erupts from the wall!";
            g_msgTimer=5.f;
        };
        g_puzzles.addPuzzle(p);
    }
    // 9: Mirror Rotation (Vault) — aim reflected beam at photo sensor
    {
        Puzzle p; p.type=PuzzleType::MIRROR_LIGHT;
        p.requiredAngle=45.f;
        p.hint="Rotate the mirror [Q/E] to direct the laser at the sensor beside the door.";
        p.onSolve=[&]{
            Room& r=g_scene.rooms[2];
            for(auto& o:r.objects) {
                if(o.name=="FinalDoor") o.isOpen=true;
                if(o.name=="ReflectedBeam") o.visible=true;
            }
            g_interactMsg="The sensor activates! The vault door opens!";
            g_msgTimer=5.f;
            g_winTimer=4.f;
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
            g_interactMsg="Picked up: "+Inventory::nameOf(obj->givesItem)+". "+obj->interactMsg;
            g_msgTimer=5.f;
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
                    // Remove painting from wall — make it invisible
                    Room& r=g_scene.rooms[0];
                    for(auto& o:r.objects){
                        if(o.name=="Painting"){
                            o.visible=false;
                            o.solid=false;
                            o.interaction=InteractionType::NONE;
                        }
                        if(o.name=="HiddenButton") o.visible=true;
                    }
                    g_interactMsg="You lift the painting off the wall...";
                    g_msgTimer=3.f;
                } else if(obj->name=="Altar") {
                    // Check if player has the red crystal
                    if(g_inv.hasItem(ItemType::CRYSTAL_RED)) {
                        g_puzzles.interactHiddenButton(pid);
                    } else {
                        g_interactMsg="The altar's crystal slot is empty. Find the crystal.";
                        g_msgTimer=3.f;
                    }
                } else {
                    // Generic hidden button (Room 1)
                    g_puzzles.interactHiddenButton(pid);
                    g_interactMsg="*CLICK* Something shifted in the cabinet!";
                    g_msgTimer=3.f;
                }
            }
            else if(pz->type==PuzzleType::LIGHT_SEQUENCE){
                g_activePuzzle=pid;
                if(obj->name=="SeqStartBtn"){
                    if(!g_showingSeq){
                        g_puzzles.startLightSequence(pid);
                        g_showingSeq=true; g_seqTimer=0.f;
                        g_seqInput.clear();
                        g_state=GameState::SEQUENCE_UI;
                        g_interactMsg="Watch the lights carefully...";
                        g_msgTimer=3.f;
                    } else {
                        g_interactMsg="Sequence is already playing!";
                        g_msgTimer=2.f;
                    }
                } else if(obj->name.find("SeqBtn") != std::string::npos) {
                    if(g_showingSeq){
                        g_interactMsg="Wait for the sequence to finish!";
                        g_msgTimer=2.f;
                    } else {
                        int idx = obj->name.back()-'0';
                        g_seqInput.push_back(idx);
                        g_interactMsg="Button " + std::to_string(idx+1) + " pressed.";
                        g_msgTimer=1.5f;
                        if((int)g_seqInput.size()==(int)g_puzzles.get(pid)->sequence.size()){
                            bool solved = g_puzzles.submitLightSequence(pid,g_seqInput);
                            g_seqInput.clear();
                            if(solved){
                                g_interactMsg="Sequence accepted!";
                            } else {
                                g_interactMsg="Wrong sequence. Try again.";
                            }
                            g_msgTimer=3.f;
                        }
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
                
                // Sync visual lever/switch states
                for(auto& o: g_scene.activeRoom().objects) {
                    if(o.name.find("Lever") != std::string::npos || o.name.find("Switch") != std::string::npos) {
                        int lidx = o.name.back()-'0';
                        if (lidx >= 0 && lidx < 4) {
                            o.isOpen = g_puzzles.get(pid)->leverPulled[lidx];
                            // Push buttons inward when pressed
                            if (o.isOpen) {
                                if (o.name=="VaultSwitch0") o.transform.position.z = -4.95f; // North
                                if (o.name=="VaultSwitch1") o.transform.position.x = 4.95f;  // East
                                if (o.name=="VaultSwitch2") o.transform.position.x = -4.95f; // West
                                if (o.name=="VaultSwitch3") o.transform.position.z = 4.95f;  // South
                            } else {
                                if (o.name=="VaultSwitch0") o.transform.position.z = -4.85f;
                                if (o.name=="VaultSwitch1") o.transform.position.x = 4.85f;
                                if (o.name=="VaultSwitch2") o.transform.position.x = -4.85f;
                                if (o.name=="VaultSwitch3") o.transform.position.z = 4.85f;
                            }
                        }
                    }
                }
                
                if (solved) {
                    g_interactMsg="Correct sequence! A mechanism triggers.";
                } else if (g_puzzles.get(pid)->playerSequence.empty()) {
                    g_interactMsg="*CLUNK* The switches reset!";
                } else {
                    g_interactMsg="Switch pressed.";
                }
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
            int prevSize = (int)pz->playerSequence.size();
            bool solved = g_puzzles.interactPressurePlate(obj.puzzleId,idx);
            if(solved) {
                // onSolve callback handles the message
            } else if((int)pz->playerSequence.size() > prevSize) {
                g_interactMsg = obj.interactMsg + " [" + std::to_string(pz->playerSequence.size()) + "/" + std::to_string(pz->sequence.size()) + "]";
                g_msgTimer = 2.f;
            } else if(pz->playerSequence.empty() && prevSize > 0) {
                g_interactMsg = "Wrong order! The plates reset.";
                g_msgTimer = 3.f;
            }
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
            if(g_state==GameState::PLAYING){
                GameObject* obj = pickObject(INTERACT_DIST);
                if (obj && obj->name == "Mirror" && g_activePuzzle == 9) {
                    g_puzzles.interactMirror(9, +5.f);
                    g_interactMsg="Mirror rotated right."; g_msgTimer=1.f;
                } else {
                    doInteract();
                }
            }
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
                // Full restart: rebuild everything
                g_state=GameState::MENU;
                g_timer=GAME_TIMER;
                g_player.position={0,0,2.5f};
                g_scene.currentRoom=0;
                g_inv=Inventory{};
                g_puzzles=PuzzleManager{};
                g_scene.rooms.clear();
                g_scene.buildScene();
                setupPuzzles();
                g_activePuzzle=-1;
                g_interactMsg.clear(); g_msgTimer=0.f;
                g_hint.clear(); g_hintTimer=0.f;
                g_winTimer=-1.f;
                glfwSetInputMode(win,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
            }
            break;
        case GLFW_KEY_Q:
            if(g_state==GameState::PAUSED) glfwSetWindowShouldClose(win,true);
            if(g_state==GameState::PLAYING && g_activePuzzle==9) {
                g_puzzles.interactMirror(9,-5.f);
                g_interactMsg="Mirror rotated left."; g_msgTimer=1.f;
            }
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

    GLFWwindow* win=glfwCreateWindow(SCR_W,SCR_H,"Enigma",nullptr,nullptr);
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
        if(g_state==GameState::PLAYING || g_state==GameState::SEQUENCE_UI)
        {
            if (g_state == GameState::PLAYING) {
                bool W=glfwGetKey(win,GLFW_KEY_W)==GLFW_PRESS;
                bool S=glfwGetKey(win,GLFW_KEY_S)==GLFW_PRESS;
                bool A=glfwGetKey(win,GLFW_KEY_A)==GLFW_PRESS;
                bool D=glfwGetKey(win,GLFW_KEY_D)==GLFW_PRESS;
                bool JP=glfwGetKey(win,GLFW_KEY_SPACE)==GLFW_PRESS;
                bool CR=glfwGetKey(win,GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS;
                g_player.processKeyboard(W,S,A,D,JP,CR,dt);
            }
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
                    g_showingSeq=false;
                    g_state=GameState::PLAYING; // player's turn to repeat it
                }
            }

            // Update prompt timers
            if(g_msgTimer>0.f) g_msgTimer-=dt;
            if(g_hintTimer>0.f) g_hintTimer-=dt;
            if(g_winTimer>0.f) {
                g_winTimer-=dt;
                if(g_winTimer<=0.f) g_state=GameState::WIN;
            }

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

            // Door animations & Mirror sync
            for(auto& obj: g_scene.activeRoom().objects) {
                if(obj.isDoor) obj.updateDoorAnim(dt);
                if(obj.name == "Mirror" && g_activePuzzle == 9) {
                    Puzzle* pz = g_puzzles.get(9);
                    if (pz) obj.transform.rotation.y = pz->mirrorAngle;
                }
            }
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
