#include "puzzle.h"
#include <algorithm>
#include <cmath>

int PuzzleManager::addPuzzle(Puzzle p) {
    p.id = (int)puzzles.size();
    puzzles.push_back(p);
    return p.id;
}

Puzzle* PuzzleManager::get(int id) {
    if (id < 0 || id >= (int)puzzles.size()) return nullptr;
    return &puzzles[id];
}

bool PuzzleManager::interactHiddenButton(int id) {
    auto* p = get(id);
    if (!p || p->isSolved()) return false;
    p->state = PuzzleState::SOLVED;
    if (p->onSolve) p->onSolve();
    return true;
}

bool PuzzleManager::interactKeyLock(int id, bool hasKey) {
    auto* p = get(id);
    if (!p || p->isSolved()) return false;
    if (!hasKey) return false;
    p->state = PuzzleState::SOLVED;
    if (p->onSolve) p->onSolve();
    return true;
}

bool PuzzleManager::interactCodePad(int id, const std::string& code) {
    auto* p = get(id);
    if (!p || p->isSolved()) return false;
    p->currentInput = code;
    if (code == p->solution) {
        p->state = PuzzleState::SOLVED;
        if (p->onSolve) p->onSolve();
        return true;
    }
    return false;
}

bool PuzzleManager::interactLeverSequence(int id, int leverIndex) {
    auto* p = get(id);
    if (!p || p->isSolved()) return false;
    if ((int)p->leverPulled.size() <= leverIndex) return false;

    // Toggle lever
    p->leverPulled[leverIndex] = !p->leverPulled[leverIndex];

    // Check if player sequence matches
    p->playerSequence.push_back(leverIndex);
    int step = (int)p->playerSequence.size() - 1;

    if (step >= (int)p->sequence.size() || p->playerSequence[step] != p->sequence[step]) {
        // Wrong order — reset
        p->playerSequence.clear();
        std::fill(p->leverPulled.begin(), p->leverPulled.end(), false);
        return false;
    }

    if ((int)p->playerSequence.size() == (int)p->sequence.size()) {
        p->state = PuzzleState::SOLVED;
        if (p->onSolve) p->onSolve();
        return true;
    }
    return false;
}

bool PuzzleManager::interactPressurePlate(int id, int plateIndex) {
    auto* p = get(id);
    if (!p || p->isSolved()) return false;

    int step = (int)p->playerSequence.size();
    if (step >= (int)p->sequence.size() || plateIndex != p->sequence[step]) {
        p->playerSequence.clear();
        return false;
    }
    p->playerSequence.push_back(plateIndex);
    if ((int)p->playerSequence.size() == (int)p->sequence.size()) {
        p->state = PuzzleState::SOLVED;
        if (p->onSolve) p->onSolve();
        return true;
    }
    return false;
}

bool PuzzleManager::interactMirror(int id, float angleDelta) {
    auto* p = get(id);
    if (!p || p->isSolved()) return false;
    p->mirrorAngle += angleDelta;
    float diff = std::abs(p->mirrorAngle - p->requiredAngle);
    if (diff < 3.f) {
        p->state = PuzzleState::SOLVED;
        if (p->onSolve) p->onSolve();
        return true;
    }
    return false;
}

// Light sequence: show then player repeats
bool PuzzleManager::startLightSequence(int id) {
    auto* p = get(id);
    if (!p || p->isSolved()) return false;
    p->state      = PuzzleState::IN_PROGRESS;
    p->currentStep= 0;
    p->playerSequence.clear();
    return true;
}

bool PuzzleManager::submitLightSequence(int id, const std::vector<int>& input) {
    auto* p = get(id);
    if (!p) return false;
    if (input == p->sequence) {
        p->state = PuzzleState::SOLVED;
        if (p->onSolve) p->onSolve();
        return true;
    }
    p->playerSequence.clear();
    p->currentStep = 0;
    return false;
}

int PuzzleManager::lightSequenceCurrentStep(int id) const {
    if (id < 0 || id >= (int)puzzles.size()) return -1;
    const auto& p = puzzles[id];
    if (p.state != PuzzleState::IN_PROGRESS) return -1;
    if (p.currentStep < 0 || p.currentStep >= (int)p.sequence.size()) return -1;
    return p.sequence[p.currentStep];
}

void PuzzleManager::updateLightSequence(int id, float dt) {
    static float timer = 0.f;
    auto* p = get(id);
    if (!p || p->state != PuzzleState::IN_PROGRESS) return;

    timer += dt;
    if (timer >= 0.8f) {
        timer = 0.f;
        p->currentStep++;
        if (p->currentStep >= (int)p->sequence.size())
            p->currentStep = -1; // sequence done — player's turn
    }
}
