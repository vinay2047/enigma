#include "room.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <functional>
#include <cmath>

// ── Procedural texture generation ────────────────────────────────────────────
unsigned int makeProceduralTexture(int w, int h,
    std::function<glm::vec3(int,int)> fn)
{
    std::vector<unsigned char> data(w * h * 3);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            glm::vec3 c = glm::clamp(fn(x,y), {0.f},{1.f});
            int idx = (y*w+x)*3;
            data[idx+0] = (unsigned char)(c.r*255);
            data[idx+1] = (unsigned char)(c.g*255);
            data[idx+2] = (unsigned char)(c.b*255);
        }
    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    return id;
}

static float hash(int x, int y) {
    int n = x + y * 57;
    n = (n<<13)^n;
    return 1.f - ((n*(n*n*15731+789221)+1376312589)&0x7fffffff)/1073741824.f;
}

unsigned int makeStoneTexture() {
    return makeProceduralTexture(128,128,[](int x,int y){
        float v = hash(x/4,y/4)*0.5f + hash(x,y)*0.1f + 0.35f;
        return glm::vec3(v*0.55f, v*0.50f, v*0.48f);
    });
}
unsigned int makeWoodTexture() {
    return makeProceduralTexture(128,128,[](int x,int y){
        float ring = std::sin((float)(x+hash(x/16,y/16)*8)*0.4f)*0.15f + 0.6f;
        float grain= hash(x,y)*0.05f;
        return glm::vec3(ring+grain, (ring+grain)*0.65f, (ring+grain)*0.25f);
    });
}
unsigned int makeMetalTexture() {
    return makeProceduralTexture(128,128,[](int x,int y){
        float v = hash(x,y)*0.08f + 0.55f + std::sin(y*0.4f)*0.05f;
        return glm::vec3(v*0.75f,v*0.78f,v*0.82f);
    });
}
unsigned int makeBrickTexture() {
    return makeProceduralTexture(128,64,[](int x,int y){
        int row=y/8; int col=(x+(row%2)*32)/16;
        bool mortar=(y%8<1)||(x%16<1+((row%2)*32)%1);
        float v = mortar ? 0.35f : 0.52f+hash(col,row)*0.1f;
        return glm::vec3(v*1.1f, v*0.65f, v*0.5f);
    });
}
unsigned int makeCarpetTexture() {
    return makeProceduralTexture(64,64,[](int x,int y){
        float v = hash(x,y)*0.12f + 0.28f;
        return glm::vec3(v*0.5f, v*0.2f, v*0.6f);
    });
}

// ── AABB from box transform ────────────────────────────────────────────────────
static AABB aabbFromTransform(const Transform& t) {
    glm::vec3 half = t.scale * 0.5f;
    return AABB::fromCenter(t.position, half);
}

// ── GameObject ────────────────────────────────────────────────────────────────
void GameObject::updateDoorAnim(float dt) {
    if (!isDoor) return;
    float target = isOpen ? openAngle : 0.f;
    float delta  = target - curAngle;
    if (std::abs(delta) < 0.1f) { curAngle = target; return; }
    curAngle += (delta > 0 ? 1.f : -1.f) * animSpeed * dt;
    curAngle  = std::clamp(curAngle, std::min(0.f, openAngle), std::max(0.f, openAngle));
}

// ── SceneManager helpers ──────────────────────────────────────────────────────
GameObject SceneManager::makeBox(const std::string& name,
    glm::vec3 pos, glm::vec3 scale, Material mat, bool solid)
{
    GameObject g;
    g.name           = name;
    g.transform.position = pos;
    g.transform.scale    = scale;
    g.material       = mat;
    g.meshType       = MeshType::BOX;
    g.solid          = solid;
    g.aabb           = aabbFromTransform(g.transform);
    return g;
}

// ── Room builders ─────────────────────────────────────────────────────────────
void SceneManager::buildEntryHall(Room& r) {
    r.id   = 0;
    r.name = "Entry Hall";
    r.ambientStrength = 0.85f;
    r.ambientColor    = {1.0f, 0.95f, 0.85f};
    r.fogDensity      = 0.0f;
    r.fogColor        = {0.8f, 0.75f, 0.7f};

    unsigned int stoneTex  = makeStoneTexture();
    unsigned int woodTex   = makeWoodTexture();
    unsigned int brickTex  = makeBrickTexture();

    auto makeMat = [](unsigned int tex, glm::vec3 col={1,1,1}) {
        Material m; m.diffuseTex=tex; m.useTexture=(tex!=0); m.color=col; return m;
    };

    // Floor (10x10, y=0)
    r.objects.push_back(makeBox("Floor",{0,-0.05f,0},{10,0.1f,10},makeMat(woodTex)));
    // Ceiling (y=3)
    r.objects.push_back(makeBox("Ceiling",{0,3.05f,0},{10,0.1f,10},makeMat(stoneTex)));
    // Walls
    r.objects.push_back(makeBox("WallN",{0,1.5f,-5},{10,3.1f,0.2f},makeMat(brickTex)));
    r.objects.push_back(makeBox("WallS",{0,1.5f, 5},{10,3.1f,0.2f},makeMat(brickTex)));
    r.objects.push_back(makeBox("WallW",{-5,1.5f,0},{0.2f,3.1f,10},makeMat(brickTex)));
    r.objects.push_back(makeBox("WallE",{ 5,1.5f,0},{0.2f,3.1f,10},makeMat(brickTex)));

    // Table
    {
        Material tm = makeMat(woodTex);
        r.objects.push_back(makeBox("TableTop",{1.f,0.75f,1.f},{1.4f,0.08f,0.8f},tm));
        r.objects.push_back(makeBox("TableLeg1",{0.35f,0.375f,0.65f},{0.08f,0.75f,0.08f},tm));
        r.objects.push_back(makeBox("TableLeg2",{1.65f,0.375f,0.65f},{0.08f,0.75f,0.08f},tm));
        r.objects.push_back(makeBox("TableLeg3",{0.35f,0.375f,1.35f},{0.08f,0.75f,0.08f},tm));
        r.objects.push_back(makeBox("TableLeg4",{1.65f,0.375f,1.35f},{0.08f,0.75f,0.08f},tm));
    }

    // Cabinet (against west wall, holds drawer with key)
    {
        Material cm; cm.color={0.35f,0.22f,0.10f};
        r.objects.push_back(makeBox("Cabinet",{-4.0f,0.6f,-1.f},{0.8f,1.2f,0.5f},cm));

        // Locked drawer
        GameObject drawer = makeBox("Drawer",{-4.0f,0.9f,-0.75f},{0.6f,0.18f,0.04f},cm);
        drawer.interaction = InteractionType::INSPECT;
        drawer.interactMsg = "The drawer is locked. You need a key.";
        drawer.puzzleId    = 0;  // Hidden button puzzle
        r.objects.push_back(drawer);
    }

    // Painting (hides button)
    {
        Material pm; pm.color={0.6f,0.3f,0.1f};
        GameObject painting = makeBox("Painting",{-0.9f,1.8f,-4.89f},{1.0f,0.7f,0.05f},pm,false);
        painting.interaction = InteractionType::PUZZLE;
        painting.interactMsg = "A dark oil painting. Something feels off about it.";
        painting.puzzleId    = 0;
        r.objects.push_back(painting);

        // Hidden button behind painting (only visible/interactable after painting inspected)
        Material bm; bm.color={0.1f,0.1f,0.1f};
        GameObject btn = makeBox("HiddenButton",{-0.9f,1.8f,-4.88f},{0.12f,0.12f,0.05f},bm,false);
        btn.visible    = false;
        btn.interaction= InteractionType::PUZZLE;
        btn.interactMsg= "A hidden button!";
        btn.puzzleId   = 0;
        r.objects.push_back(btn);
    }

    // Brass key (spawns after puzzle 0 solved — starts invisible)
    {
        Material km; km.color={0.8f,0.65f,0.1f};
        GameObject key = makeBox("BrassKey",{-4.0f,0.95f,-0.95f},{0.12f,0.05f,0.04f},km,false);
        key.visible    = false;
        key.interaction= InteractionType::PICKUP;
        key.interactMsg= "A small brass key.";
        key.givesItem  = ItemType::KEY_BRASS;
        r.objects.push_back(key);
    }

    // Door to Study Room (east wall middle)
    {
        Material dm; dm.color={0.3f,0.18f,0.08f};
        GameObject door = makeBox("DoorToStudy",{4.91f,1.05f,0.f},{0.2f,2.1f,0.9f},dm);
        door.isDoor      = true;
        door.interaction = InteractionType::DOOR;
        door.interactMsg = "The door is locked. Need the brass key.";
        door.requiredItem= ItemType::KEY_BRASS;
        door.puzzleId    = 1;
        door.leadsToRoom = 1;
        r.objects.push_back(door);
    }

    // Lantern (flickering overhead light)
    {
        PointLight l;
        l.position      = {0.f, 2.7f, 0.f};
        l.color         = {1.0f, 0.88f, 0.6f};
        l.baseIntensity = 5.0f;
        l.intensity     = 5.0f;
        l.radius        = 25.f;
        l.flickers      = true;
        l.flickerSpeed  = 4.f;
        l.flickerAmp    = 0.2f;
        l.flickerPhase  = 0.f;
        r.lighting.addLight(l);

        // Corner lamp
        PointLight l2;
        l2.position      = {-3.5f, 2.0f, -3.5f};
        l2.color         = {1.0f, 0.8f, 0.5f};
        l2.baseIntensity = 3.0f;
        l2.intensity     = 3.0f;
        l2.radius        = 15.f;
        l2.flickers      = false;
        r.lighting.addLight(l2);
    }
}

void SceneManager::buildStudyRoom(Room& r) {
    r.id   = 1;
    r.name = "Study Room";
    r.ambientStrength = 0.85f;
    r.ambientColor    = {0.95f, 0.9f, 1.0f};
    r.fogDensity      = 0.0f;
    r.fogColor        = {0.7f, 0.7f, 0.8f};

    unsigned int woodTex  = makeWoodTexture();
    unsigned int stoneTex = makeStoneTexture();
    unsigned int carpetTex= makeCarpetTexture();
    auto makeMat = [](unsigned int tex, glm::vec3 col={1,1,1}) {
        Material m; m.diffuseTex=tex; m.useTexture=(tex!=0); m.color=col; return m;
    };

    // Room shell (12x12)
    r.objects.push_back(makeBox("Floor",  {0,-0.05f,0},{12,0.1f,12},makeMat(carpetTex)));
    r.objects.push_back(makeBox("Ceiling",{0,3.05f,0}, {12,0.1f,12},makeMat(stoneTex)));
    r.objects.push_back(makeBox("WallN",  {0,1.5f,-6}, {12,3.1f,0.2f},makeMat(stoneTex)));
    r.objects.push_back(makeBox("WallS",  {0,1.5f, 6}, {12,3.1f,0.2f},makeMat(stoneTex)));
    r.objects.push_back(makeBox("WallW",  {-6,1.5f,0},{0.2f,3.1f,12},makeMat(stoneTex)));
    r.objects.push_back(makeBox("WallE",  { 6,1.5f,0},{0.2f,3.1f,12},makeMat(stoneTex)));

    // Bookshelf (north wall)
    {
        Material sm; sm.color={0.28f,0.17f,0.07f};
        r.objects.push_back(makeBox("Shelf",{0,1.5f,-5.6f},{4.f,2.5f,0.4f},sm));

        // Book clue item
        Material bm; bm.color={0.6f,0.08f,0.08f};
        GameObject book = makeBox("BookClue",{0.5f,1.5f,-5.5f},{0.15f,0.35f,0.3f},bm,false);
        book.interaction = InteractionType::PICKUP;
        book.interactMsg = "An old tome. The spine reads: LEFT, RIGHT, MIDDLE.";
        book.givesItem   = ItemType::BOOK_CLUE;
        r.objects.push_back(book);
    }

    // Desk + chair
    {
        Material dm; dm.color={0.35f,0.22f,0.10f};
        r.objects.push_back(makeBox("Desk",  {2.f,0.75f,0.f},{1.6f,0.08f,0.9f},makeMat(woodTex)));
        r.objects.push_back(makeBox("DeskLegFL",{1.3f,0.375f,-0.4f},{0.08f,0.75f,0.08f},dm));
        r.objects.push_back(makeBox("DeskLegFR",{2.7f,0.375f,-0.4f},{0.08f,0.75f,0.08f},dm));
        r.objects.push_back(makeBox("DeskLegBL",{1.3f,0.375f,0.4f},{0.08f,0.75f,0.08f},dm));
        r.objects.push_back(makeBox("DeskLegBR",{2.7f,0.375f,0.4f},{0.08f,0.75f,0.08f},dm));
    }

    // Light sequence panel (wall, north)
    {
        Material pm; pm.color={0.1f,0.1f,0.15f};
        r.objects.push_back(makeBox("SeqPanel",{-2.f,1.5f,-5.7f},{1.2f,0.6f,0.1f},pm,false));
        // 4 light buttons on panel
        for (int i = 0; i < 4; i++) {
            Material bm; bm.color={0.05f,0.05f,0.05f};
            float bx = -2.6f + i * 0.4f;
            GameObject btn = makeBox("SeqBtn"+std::to_string(i),
                {bx, 1.5f,-5.62f},{0.25f,0.25f,0.06f},bm,false);
            btn.interaction= InteractionType::PUZZLE;
            btn.interactMsg= "Press to enter sequence.";
            btn.puzzleId   = 2;  // LIGHT_SEQUENCE puzzle
            r.objects.push_back(btn);
        }
    }

    // Code note (reveals after light sequence solved — starts invisible)
    {
        Material nm; nm.color={0.9f,0.85f,0.7f};
        GameObject note = makeBox("CodeNote",{2.f,0.82f,0.f},{0.2f,0.01f,0.15f},nm,false);
        note.visible    = false;
        note.interaction= InteractionType::PICKUP;
        note.interactMsg= "A scrap of paper with numbers: 4821";
        note.givesItem  = ItemType::CODE_NOTE;
        r.objects.push_back(note);
    }

    // 3 Levers (south wall)
    for (int i = 0; i < 3; i++) {
        Material lm; lm.color={0.5f,0.5f,0.55f};
        float lx = -1.5f + i * 1.5f;
        GameObject lever = makeBox("Lever"+std::to_string(i),
            {lx, 1.2f, 5.6f},{0.15f,0.5f,0.1f},lm,false);
        lever.interaction = InteractionType::PUZZLE;
        lever.interactMsg = "Pull the lever.";
        lever.puzzleId    = 3;  // LEVER_SEQUENCE puzzle
        r.objects.push_back(lever);
    }

    // Pressure plate (floor center, becomes active after levers)
    {
        Material pm; pm.color={0.2f,0.2f,0.3f};
        GameObject plate = makeBox("PressurePlate",{0.f,0.001f,0.f},{1.f,0.04f,1.f},pm,false);
        plate.interaction = InteractionType::PUZZLE;
        plate.interactMsg = "A pressure plate...";
        plate.puzzleId    = 4;
        r.objects.push_back(plate);
    }

    // Code pad door (east wall)
    {
        Material dm; dm.color={0.15f,0.1f,0.08f};
        GameObject door = makeBox("DoorToVault",{5.91f,1.05f,0.f},{0.2f,2.1f,0.9f},dm);
        door.isDoor      = true;
        door.interaction = InteractionType::PUZZLE;
        door.interactMsg = "A keypad lock. Enter the 4-digit code.";
        door.puzzleId    = 5;  // CODE_PAD
        door.leadsToRoom = 2;
        r.objects.push_back(door);

        // Code pad object
        Material km; km.color={0.05f,0.05f,0.1f};
        r.objects.push_back(makeBox("CodePad",{5.8f,1.3f,0.6f},{0.12f,0.2f,0.05f},km,false));
    }

    // Lights
    {
        PointLight l;
        l.position={0,2.7f,0}; l.color={1.0f,0.9f,0.7f};
        l.baseIntensity=5.0f; l.intensity=5.0f; l.radius=28.f;
        l.flickers=true; l.flickerSpeed=3.f; l.flickerAmp=0.15f;
        r.lighting.addLight(l);

        PointLight l2;
        l2.position={-3.f,2.7f,-4.f}; l2.color={0.7f,0.6f,1.0f};
        l2.baseIntensity=3.0f; l2.intensity=3.0f; l2.radius=18.f;
        l2.flickers=false;
        r.lighting.addLight(l2);
    }
}

void SceneManager::buildVault(Room& r) {
    r.id   = 2;
    r.name = "The Vault";
    r.ambientStrength = 0.80f;
    r.ambientColor    = {0.8f, 0.85f, 1.0f};
    r.fogDensity      = 0.0f;
    r.fogColor        = {0.5f, 0.6f, 0.8f};

    unsigned int metalTex = makeMetalTexture();
    unsigned int stoneTex = makeStoneTexture();
    auto makeMat = [](unsigned int tex, glm::vec3 col={1,1,1}) {
        Material m; m.diffuseTex=tex; m.useTexture=(tex!=0); m.color=col; return m;
    };

    // Room shell (10x10)
    r.objects.push_back(makeBox("Floor",  {0,-0.05f,0},{10,0.1f,10},makeMat(metalTex)));
    r.objects.push_back(makeBox("Ceiling",{0,3.05f,0}, {10,0.1f,10},makeMat(stoneTex)));
    r.objects.push_back(makeBox("WallN",  {0,1.5f,-5}, {10,3.1f,0.2f},makeMat(metalTex)));
    r.objects.push_back(makeBox("WallS",  {0,1.5f, 5}, {10,3.1f,0.2f},makeMat(metalTex)));
    r.objects.push_back(makeBox("WallW",  {-5,1.5f,0},{0.2f,3.1f,10},makeMat(metalTex)));
    r.objects.push_back(makeBox("WallE",  { 5,1.5f,0},{0.2f,3.1f,10},makeMat(metalTex)));

    // Mirror (rotatable)
    {
        Material mm; mm.color={0.85f,0.90f,0.95f};
        GameObject mirror = makeBox("Mirror",{-2.f,1.5f,-4.8f},{0.6f,1.0f,0.06f},mm,false);
        mirror.interaction = InteractionType::PUZZLE;
        mirror.interactMsg = "A mounted mirror. [Q/E] to rotate.";
        mirror.puzzleId    = 6;  // MIRROR_LIGHT
        r.objects.push_back(mirror);
    }

    // Light source (beam)
    {
        Material bm; bm.color={1.f,0.95f,0.4f};
        r.objects.push_back(makeBox("LightBeam",{-4.f,1.5f,-4.9f},{0.2f,0.2f,0.2f},bm,false));
    }

    // Light sensor
    {
        Material sm; sm.color={0.1f,0.8f,0.1f};
        GameObject sensor = makeBox("Sensor",{2.f,1.5f,-4.85f},{0.25f,0.25f,0.06f},sm,false);
        sensor.interaction = InteractionType::INSPECT;
        sensor.interactMsg = "A photo sensor. Needs direct light.";
        sensor.puzzleId    = 6;
        r.objects.push_back(sensor);
    }

    // 3 Pressure plates
    {
        glm::vec3 platePos[3] = {{-2.f,0.001f,0.f},{0.f,0.001f,0.f},{2.f,0.001f,0.f}};
        glm::vec3 colors[3]   = {{0.8f,0.2f,0.2f},{0.2f,0.8f,0.2f},{0.2f,0.2f,0.8f}};
        for (int i=0;i<3;i++) {
            Material pm; pm.color=colors[i];
            GameObject plate = makeBox("VaultPlate"+std::to_string(i),
                platePos[i],{0.8f,0.04f,0.8f},pm,false);
            plate.interaction = InteractionType::PUZZLE;
            plate.interactMsg = "A pressure plate. Step on them in order!";
            plate.puzzleId    = 5;  // PRESSURE_PLATE
            r.objects.push_back(plate);
        }
    }

    // Final exit door
    {
        Material dm; dm.color={0.1f,0.4f,0.1f};
        GameObject door = makeBox("FinalDoor",{0.f,1.05f,-4.91f},{1.2f,2.1f,0.18f},dm);
        door.isDoor      = true;
        door.interaction = InteractionType::DOOR;
        door.interactMsg = "The final exit. Solve all puzzles to open!";
        door.puzzleId    = 6;
        r.objects.push_back(door);
    }

    // Lights
    {
        PointLight l;
        l.position={0,2.7f,0}; l.color={0.5f,0.6f,1.0f};
        l.baseIntensity=5.0f; l.intensity=5.0f; l.radius=25.f;
        l.flickers=true; l.flickerSpeed=2.f; l.flickerAmp=0.1f;
        r.lighting.addLight(l);

        PointLight l2;
        l2.position={-3.f,1.5f,-3.f}; l2.color={1.f,0.85f,0.3f};
        l2.baseIntensity=4.0f; l2.intensity=4.0f; l2.radius=15.f;
        l2.flickers=false;
        r.lighting.addLight(l2);
    }
}

void SceneManager::buildScene() {
    rooms.resize(3);
    buildEntryHall(rooms[0]);
    buildStudyRoom(rooms[1]);
    buildVault    (rooms[2]);
}
