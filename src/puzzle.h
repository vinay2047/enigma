#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <functional>

enum class PuzzleType {
    HIDDEN_BUTTON,    // Find hidden button -> opens drawer
    KEY_LOCK,         // Use correct key -> opens door
    LIGHT_SEQUENCE,   // Watch + repeat sequence
    CODE_PAD,         // 4-digit code entry
    LEVER_SEQUENCE,   // Pull levers in order
    PRESSURE_PLATE,   // Step on plates in order
    MIRROR_LIGHT      // Rotate mirror to hit sensor
};

enum class PuzzleState { UNSOLVED, IN_PROGRESS, SOLVED };

struct Puzzle {
    int         id       {-1};
    PuzzleType  type;
    PuzzleState state    {PuzzleState::UNSOLVED};
    std::string hint;

    // CODE_PAD
    std::string solution;       // e.g. "4821"
    std::string currentInput;

    // LIGHT_SEQUENCE / LEVER_SEQUENCE
    std::vector<int> sequence;       // correct order
    std::vector<int> playerSequence; // what player entered
    int              currentStep{0};

    // PRESSURE_PLATE  (same vectors)
    // MIRROR_LIGHT
    float mirrorAngle    {0.f};
    float requiredAngle  {45.f};

    // Lever states (LEVER_SEQUENCE: 3 levers)
    std::vector<bool> leverPulled;

    // callback on solve
    std::function<void()> onSolve;

    bool isSolved() const { return state == PuzzleState::SOLVED; }
};

class PuzzleManager {
public:
    std::vector<Puzzle> puzzles;

    int  addPuzzle(Puzzle p);
    Puzzle* get(int id);

    // Puzzle interaction handlers (return true when solved)
    bool interactHiddenButton   (int id);
    bool interactKeyLock        (int id, bool hasKey);
    bool interactCodePad        (int id, const std::string& code);
    bool interactLeverSequence  (int id, int leverIndex);
    bool interactPressurePlate  (int id, int plateIndex);
    bool interactMirror         (int id, float angleDelta);
    bool startLightSequence     (int id);         // shows sequence
    bool submitLightSequence    (int id, const std::vector<int>& input);

    // Light sequence display: which light is currently lit (returns -1 when done)
    int  lightSequenceCurrentStep(int id) const;
    void updateLightSequence(int id, float dt);
};
