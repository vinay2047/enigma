#include "room.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <functional>
#include <cmath>
#include <algorithm>

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
    if (std::abs(delta) < 0.1f) {
        curAngle = target;
        transform.rotation = doorAxis * curAngle;
        // When door is fully open, disable collision so player can walk through
        solid = !isOpen;
        return;
    }
    curAngle += (delta > 0 ? 1.f : -1.f) * animSpeed * dt;
    curAngle  = std::clamp(curAngle, std::min(0.f, openAngle), std::max(0.f, openAngle));
    transform.rotation = doorAxis * curAngle;
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

        // Locked drawer — solid block embedded in cabinet
        GameObject drawer = makeBox("Drawer",{-4.0f,0.9f,-0.95f},{0.6f,0.18f,0.4f},cm);
        drawer.interaction = InteractionType::INSPECT;
        drawer.interactMsg = "The drawer is locked shut.";
        drawer.puzzleId    = 0;
        r.objects.push_back(drawer);
    }

    // Painting (hides button) — removed from wall when interacted
    {
        Material pm; pm.color={0.55f,0.28f,0.08f};
        GameObject painting = makeBox("Painting",{-0.9f,1.8f,-4.89f},{1.0f,0.7f,0.05f},pm,false);
        painting.interaction = InteractionType::PUZZLE;
        painting.interactMsg = "A dark oil painting. [E] to take it off the wall.";
        painting.puzzleId    = 0;
        r.objects.push_back(painting);

        // Hidden button behind painting (only visible after painting removed)
        Material bm; bm.color={0.08f,0.08f,0.08f};
        GameObject btn = makeBox("HiddenButton",{-0.9f,1.8f,-4.87f},{0.14f,0.14f,0.06f},bm,false);
        btn.visible    = false;
        btn.interaction= InteractionType::PUZZLE;
        btn.interactMsg= "Press the hidden button! [E]";
        btn.puzzleId   = 0;
        r.objects.push_back(btn);
    }

    // Brass key — spawns resting on top of the pulled-out drawer block
    {
        Material km; km.color={0.85f,0.68f,0.05f};
        GameObject key = makeBox("BrassKey",{-4.0f,1.02f,-0.65f},{0.14f,0.06f,0.05f},km,false);
        key.visible    = false;
        key.solid      = false;
        key.interaction= InteractionType::PICKUP;
        key.interactMsg= "A small brass key. Pick it up! [E]";
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
        r.objects.push_back(makeBox("Shelf",{1.5f,1.5f,-5.6f},{3.f,2.5f,0.4f},sm));

        // Book clue item
        Material bm; bm.color={0.6f,0.08f,0.08f};
        GameObject book = makeBox("BookClue",{1.5f,1.5f,-5.5f},{0.15f,0.35f,0.3f},bm,false);
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
        // Decorative back panel
        Material pm; pm.color={0.1f,0.1f,0.15f};
        r.objects.push_back(makeBox("SeqPanel",{-2.f,1.5f,-5.7f},{1.6f,0.6f,0.1f},pm,false));

        // Start button (distinct, located below the panel)
        Material startMat; startMat.color={0.1f, 0.5f, 0.1f}; // Green
        GameObject startBtn = makeBox("SeqStartBtn",{-2.f, 1.0f, -5.7f},{0.5f,0.2f,0.1f},startMat,false);
        startBtn.interaction = InteractionType::PUZZLE;
        startBtn.interactMsg = "Press [E] to Play Sequence";
        startBtn.puzzleId    = 2;
        r.objects.push_back(startBtn);

        // 4 input buttons on panel
        for (int i = 0; i < 4; i++) {
            Material bm; bm.color={0.2f,0.2f,0.2f};
            float bx = -2.6f + i * 0.4f;
            GameObject btn = makeBox("SeqBtn"+std::to_string(i),
                {bx, 1.5f,-5.62f},{0.25f,0.3f,0.06f},bm,false);
            btn.interaction= InteractionType::PUZZLE;
            btn.interactMsg= "Input Button " + std::to_string(i+1) + " [E]";
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
        lever.isDoor      = true;
        lever.isOpen      = false;
        lever.openAngle   = 45.f; // pulls down
        lever.doorAxis    = {1.f, 0.f, 0.f}; // rotates on X axis
        lever.animSpeed   = 200.f;
        lever.interaction = InteractionType::PUZZLE;
        lever.interactMsg = "Pull the lever.";
        lever.puzzleId    = 3;  // LEVER_SEQUENCE puzzle
        r.objects.push_back(lever);
    }

    // Pressure plate (floor center, becomes active after levers)
    {
        Material pm; pm.color={0.2f,0.2f,0.3f};
        GameObject plate = makeBox("PressurePlate",{0.f,0.001f,0.f},{1.f,0.04f,1.f},pm,false);
        plate.visible     = false;  // Hidden until lever sequence (puzzle 3) is solved
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
        door.interaction = InteractionType::INSPECT;
        door.interactMsg = "The keypad is disabled. It needs power.";
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

    // Room shell (10x10) — north wall split for doorway
    r.objects.push_back(makeBox("Floor",  {0,-0.05f,0},{10,0.1f,10},makeMat(metalTex)));
    r.objects.push_back(makeBox("Ceiling",{0,3.05f,0}, {10,0.1f,10},makeMat(stoneTex)));
    // North wall: left segment + right segment + lintel above door
    r.objects.push_back(makeBox("WallNL", {-3.2f,1.5f,-5},{3.6f,3.1f,0.2f},makeMat(metalTex)));
    r.objects.push_back(makeBox("WallNR", { 3.2f,1.5f,-5},{3.6f,3.1f,0.2f},makeMat(metalTex)));
    r.objects.push_back(makeBox("WallNT", {0.f,2.65f,-5},{2.8f,0.8f,0.2f},makeMat(metalTex)));
    r.objects.push_back(makeBox("WallS",  {0,1.5f, 5}, {10,3.1f,0.2f},makeMat(metalTex)));
    r.objects.push_back(makeBox("WallW",  {-5,1.5f,0},{0.2f,3.1f,10},makeMat(metalTex)));
    r.objects.push_back(makeBox("WallE",  { 5,1.5f,0},{0.2f,3.1f,10},makeMat(metalTex)));

    // ── Riddle Tablet (west wall) ──
    {
        Material tbm; tbm.color={0.35f,0.3f,0.25f};
        r.objects.push_back(makeBox("TabletBG",{-4.85f,1.6f,0.f},{0.1f,0.8f,1.4f},tbm,false));
        Material tm; tm.color={0.9f,0.85f,0.7f};
        GameObject tablet = makeBox("RiddleTablet",{-4.82f,1.6f,0.f},{0.06f,0.6f,1.2f},tm,false);
        tablet.interaction = InteractionType::INSPECT;
        tablet.interactMsg = "A compass is etched here. It says: 'Follow the winds: North, East, West, South.'";
        r.objects.push_back(tablet);
    }

    // ── Table in the corner (where key appears) ──
    {
        Material tm; tm.color={0.25f,0.2f,0.15f};
        r.objects.push_back(makeBox("SmallTable",{-3.5f,0.4f,-3.5f},{0.6f,0.05f,0.6f},tm));
        r.objects.push_back(makeBox("TableLeg",{-3.5f,0.2f,-3.5f},{0.1f,0.4f,0.1f},tm));

        Material km; km.color={0.5f,0.5f,0.55f};
        GameObject key = makeBox("IronKey",{-3.5f,0.48f,-3.5f},{0.15f,0.08f,0.04f},km,false);
        key.visible = false;
        key.interaction = InteractionType::PICKUP;
        key.interactMsg = "An iron key.";
        key.givesItem = ItemType::KEY_IRON;
        r.objects.push_back(key);
    }

    // ── 4 Wall Buttons (Puzzle 6 - sequence) ──
    {
        Material bm; bm.color={0.4f,0.35f,0.3f};
        // Button 0: North
        GameObject btnN = makeBox("VaultSwitch0",{2.5f,1.3f,-4.85f},{0.15f,0.15f,0.06f},bm,false);
        btnN.interaction = InteractionType::PUZZLE;
        btnN.interactMsg = "A stone button on the North wall.";
        btnN.puzzleId = 6;
        r.objects.push_back(btnN);

        // Button 1: East
        GameObject btnE = makeBox("VaultSwitch1",{4.85f,1.3f,1.5f},{0.06f,0.15f,0.15f},bm,false);
        btnE.interaction = InteractionType::PUZZLE;
        btnE.interactMsg = "A stone button on the East wall.";
        btnE.puzzleId = 6;
        r.objects.push_back(btnE);

        // Button 2: West
        GameObject btnW = makeBox("VaultSwitch2",{-4.85f,1.3f,1.5f},{0.06f,0.15f,0.15f},bm,false);
        btnW.interaction = InteractionType::PUZZLE;
        btnW.interactMsg = "A stone button on the West wall.";
        btnW.puzzleId = 6;
        r.objects.push_back(btnW);

        // Button 3: South
        GameObject btnS = makeBox("VaultSwitch3",{1.5f,1.3f,4.85f},{0.15f,0.15f,0.06f},bm,false);
        btnS.interaction = InteractionType::PUZZLE;
        btnS.interactMsg = "A stone button on the South wall.";
        btnS.puzzleId = 6;
        r.objects.push_back(btnS);
    }

    // ── Locked Chest (south wall, near bench) ──
    {
        // Chest body
        Material chm; chm.color={0.35f,0.22f,0.1f};
        r.objects.push_back(makeBox("LockedChestBody",{-3.f,0.3f,4.f},{1.0f,0.6f,0.6f},chm));
        
        // Chest lid (animates open)
        GameObject lid = makeBox("LockedChest",{-3.f,0.65f,4.f},{1.0f,0.1f,0.6f},chm);
        lid.interaction = InteractionType::DOOR;
        lid.interactMsg = "A heavy iron-bound chest. It's locked.";
        lid.puzzleId = 7;
        lid.requiredItem = ItemType::KEY_IRON;
        lid.isDoor = false; // We'll manually trigger it in the puzzle solver to avoid regular door logic taking over
        lid.isOpen = false;
        lid.openAngle = 80.f; // pulls up and back
        lid.doorAxis = {1.f, 0.f, 0.f}; // hinge on X axis
        lid.animSpeed = 100.f;
        r.objects.push_back(lid);

        // Chest lock detail
        Material lkm; lkm.color={0.6f,0.5f,0.15f};
        r.objects.push_back(makeBox("ChestLock",{-3.f,0.45f,3.68f},{0.15f,0.15f,0.06f},lkm,false));
    }

    // ── Red Crystal (hidden inside chest, easily visible when lid opens) ──
    {
        Material cm; cm.color={0.9f,0.15f,0.1f};
        GameObject crystal = makeBox("RedCrystal",{-3.f,0.65f,3.6f},{0.18f,0.18f,0.18f},cm,false);
        crystal.visible = false;
        crystal.interaction = InteractionType::PICKUP;
        crystal.interactMsg = "A crimson crystal, pulsing with ancient energy.";
        crystal.givesItem = ItemType::CRYSTAL_RED;
        r.objects.push_back(crystal);
    }

    // ── Crystal Altar (center of room) ──
    {
        Material am; am.color={0.18f,0.15f,0.22f};
        r.objects.push_back(makeBox("AltarBase",{0.f,0.35f,0.f},{1.0f,0.7f,1.0f},am));
        Material at; at.color={0.25f,0.2f,0.3f};
        r.objects.push_back(makeBox("AltarTop",{0.f,0.72f,0.f},{1.1f,0.04f,1.1f},at,false));
        // Single crystal slot
        Material sm; sm.color={0.12f,0.1f,0.15f};
        r.objects.push_back(makeBox("CrystalSlot",{0.f,0.78f,0.f},{0.25f,0.1f,0.25f},sm,false));
        // Altar interaction
        GameObject altarUse = makeBox("Altar",{0.f,0.85f,0.f},{1.0f,0.1f,1.0f},at,false);
        altarUse.visible = true;
        altarUse.interaction = InteractionType::PUZZLE;
        altarUse.interactMsg = "An ancient altar with an empty crystal slot.";
        altarUse.puzzleId = 8;
        r.objects.push_back(altarUse);
    }

    // ── Light Emitter (south wall, hidden until plates solved) ──
    {
        Material em; em.color={0.15f,0.15f,0.2f};
        GameObject emitter = makeBox("LightEmitter",{0.f,1.5f,4.85f},{0.4f,0.4f,0.1f},em,false);
        emitter.visible = false;
        r.objects.push_back(emitter);

        Material lm; lm.color={1.f,0.9f,0.3f};
        GameObject lens = makeBox("EmitterLens",{0.f,1.5f,4.78f},{0.15f,0.15f,0.06f},lm,false);
        lens.visible = false;
        r.objects.push_back(lens);
    }

    // ── Laser beam: emitter to mirror (thin ray, hidden until plates solved) ──
    {
        Material bm; bm.color={1.f,0.95f,0.3f};
        GameObject beam = makeBox("LaserBeam",{0.f,1.5f,2.4f},{0.02f,0.02f,4.7f},bm,false);
        beam.visible = false;
        r.objects.push_back(beam);
    }

    // ── Mirror (on top of altar, rotatable, hidden until crystal placed) ──
    {
        Material sm; sm.color={0.2f,0.2f,0.25f};
        GameObject stand = makeBox("MirrorStand",{0.f,1.0f,0.f},{0.15f,0.5f,0.15f},sm);
        stand.visible = false;
        r.objects.push_back(stand);

        Material mm; mm.color={0.85f,0.9f,0.95f}; mm.shininess=128.f;
        GameObject mirror = makeBox("Mirror",{0.f,1.5f,0.f},{0.5f,0.6f,0.05f},mm,false);
        mirror.visible     = false;
        mirror.interaction = InteractionType::PUZZLE;
        mirror.interactMsg = "A polished mirror on a swivel. [Q/E] to rotate.";
        mirror.puzzleId    = 9;
        r.objects.push_back(mirror);
    }

    // ── Reflected beam: mirror to sensor (thin ray, hidden until mirror correct) ──
    // Mirror at (0, 1.5, 0), Sensor at (-2.2, 1.5, -4.88)
    // Midpoint: (-1.1, 1.5, -2.44), Length: sqrt(2.2^2 + 4.88^2) ≈ 5.35
    // Angle: atan2(-2.2, -4.88) ≈ 24.3 degrees from -Z axis
    {
        Material bm; bm.color={1.f,0.95f,0.3f};
        GameObject refBeam = makeBox("ReflectedBeam",{-1.1f,1.5f,-2.44f},{0.02f,0.02f,5.35f},bm,false);
        refBeam.visible = false;
        refBeam.transform.rotation.y = 24.3f;
        r.objects.push_back(refBeam);
    }

    // ── Photo Sensor (near final door, north wall) ──
    {
        Material sm; sm.color={0.15f,0.6f,0.15f};
        GameObject sensor = makeBox("PhotoSensor",{-2.2f,1.5f,-4.88f},{0.25f,0.25f,0.06f},sm,false);
        sensor.interaction = InteractionType::INSPECT;
        sensor.interactMsg = "A photo-sensitive receptor. Needs a beam of light.";
        r.objects.push_back(sensor);
    }

    // ── Final exit door (north wall) ──
    {
        Material dm; dm.color={0.12f,0.1f,0.08f};
        GameObject door = makeBox("FinalDoor",{0.f,1.05f,-4.91f},{1.4f,2.1f,0.18f},dm);
        door.isDoor      = true;
        door.interaction = InteractionType::DOOR;
        door.interactMsg = "A massive iron door. The sensor beside it is dark.";
        door.puzzleId    = -1;
        r.objects.push_back(door);

        Material fm; fm.color={0.4f,0.35f,0.15f};
        r.objects.push_back(makeBox("DoorFrame",{0.f,1.5f,-4.95f},{1.7f,2.5f,0.08f},fm,false));
    }

    // ── Lights (dim — vault starts dark) ──
    {
        PointLight l;
        l.position={0,2.7f,0}; l.color={0.3f,0.35f,0.6f};
        l.baseIntensity=1.5f; l.intensity=1.5f; l.radius=20.f;
        l.flickers=true; l.flickerSpeed=3.f; l.flickerAmp=0.3f;
        r.lighting.addLight(l);
    }
}

void SceneManager::buildScene() {
    rooms.resize(3);
    buildEntryHall(rooms[0]);
    buildStudyRoom(rooms[1]);
    buildVault    (rooms[2]);
}
