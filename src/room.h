#pragma once
#include <functional>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "collision.h"
#include "inventory.h"
#include "lighting.h"

enum class MeshType  { BOX, PLANE };
enum class InteractionType { NONE, PICKUP, USE_KEY, INSPECT, PUZZLE, DOOR };

struct Material {
    glm::vec3    color        {1.f, 1.f, 1.f};
    unsigned int diffuseTex   {0};
    bool         useTexture   {false};
    float        shininess    {32.f};
};

struct Transform {
    glm::vec3 position {0.f};
    glm::vec3 rotation {0.f};   // degrees (yaw only for doors)
    glm::vec3 scale    {1.f};
};

struct GameObject {
    std::string     name;
    Transform       transform;
    AABB            aabb;
    Material        material;
    MeshType        meshType   {MeshType::BOX};
    bool            visible    {true};
    bool            solid      {true};

    // Interaction
    InteractionType interaction {InteractionType::NONE};
    std::string     interactMsg;
    ItemType        requiredItem {ItemType::NONE};
    ItemType        givesItem    {ItemType::NONE};
    int             puzzleId     {-1};

    // Door
    bool  isDoor      {false};
    bool  isOpen      {false};
    float openAngle   {90.f};
    float curAngle    {0.f};
    int   leadsToRoom {-1};
    float animSpeed   {120.f};  // degrees/sec

    // Light (attached lamp mesh)
    int   lightIndex  {-1};

    void updateDoorAnim(float dt);
};

struct Room {
    int                      id {-1};
    std::string              name;
    std::vector<GameObject>  objects;
    LightingSystem           lighting;
    glm::vec3                ambientColor {0.05f, 0.04f, 0.08f};
    float                    ambientStrength {0.15f};
    float                    fogDensity {0.06f};
    glm::vec3                fogColor   {0.01f, 0.01f, 0.03f};
};

// ── Texture helpers ──────────────────────────────────────────────────────────
unsigned int makeProceduralTexture(int w, int h,
    std::function<glm::vec3(int x, int y)> fn);
unsigned int makeStoneTexture();
unsigned int makeWoodTexture();
unsigned int makeMetalTexture();
unsigned int makeBrickTexture();
unsigned int makeCarpetTexture();

// ── Scene builder ─────────────────────────────────────────────────────────────
class SceneManager {
public:
    std::vector<Room>   rooms;
    int                 currentRoom {0};

    void buildScene();          // constructs all rooms & objects
    Room& activeRoom()          { return rooms[currentRoom]; }
    void  changeRoom(int idx)   { if (idx>=0 && idx<(int)rooms.size()) currentRoom=idx; }

private:
    void buildEntryHall(Room& r);
    void buildStudyRoom(Room& r);
    void buildVault    (Room& r);
    GameObject makeBox (const std::string& name,
                        glm::vec3 pos, glm::vec3 scale, Material mat,
                        bool solid=true);
};
