#pragma once
#include "Scene.h"
#include <deque>
#include <string>
#include <vector>

// ============================================================================
// UndoManager — Multi-level Undo/Redo System (52 steps capacity)
// Tracks scene, actor, property, material, light, and transform changes.
// ============================================================================

struct UndoStep {
    std::string actionName;
    Scene sceneSnapshot;
    int selectedId = -1;
};

class UndoManager {
public:
    static constexpr size_t MAX_UNDO_STEPS = 52;

    static UndoManager& Get() {
        static UndoManager instance;
        return instance;
    }

    // Push a new snapshot onto the undo stack (records pre-change state)
    void RecordSnapshot(const Scene& scene, const std::string& actionName) {
        UndoStep step;
        step.actionName = actionName;
        step.sceneSnapshot = scene;
        step.sceneSnapshot.onPreChange = nullptr; // Do not store recursion hook in snapshot
        step.selectedId = scene.selectedId;

        m_undoStack.push_back(std::move(step));
        while (m_undoStack.size() > MAX_UNDO_STEPS) {
            m_undoStack.pop_front();
        }
        m_redoStack.clear();
    }

    bool CanUndo() const { return !m_undoStack.empty(); }
    bool CanRedo() const { return !m_redoStack.empty(); }

    std::string GetUndoActionName() const {
        return m_undoStack.empty() ? "" : m_undoStack.back().actionName;
    }

    std::string GetRedoActionName() const {
        return m_redoStack.empty() ? "" : m_redoStack.back().actionName;
    }

    bool Undo(Scene& scene) {
        if (m_undoStack.empty()) return false;

        auto hook = scene.onPreChange;
        scene.onPreChange = nullptr;

        UndoStep redoStep;
        redoStep.actionName = m_undoStack.back().actionName;
        redoStep.sceneSnapshot = scene;
        redoStep.sceneSnapshot.onPreChange = nullptr;
        redoStep.selectedId = scene.selectedId;
        m_redoStack.push_back(std::move(redoStep));
        while (m_redoStack.size() > MAX_UNDO_STEPS) {
            m_redoStack.pop_front();
        }

        UndoStep target = std::move(m_undoStack.back());
        m_undoStack.pop_back();

        scene = std::move(target.sceneSnapshot);
        scene.onPreChange = hook;
        scene.selectedId = target.selectedId;
        scene.SyncLightPositionsFromActors();
        return true;
    }

    bool Redo(Scene& scene) {
        if (m_redoStack.empty()) return false;

        auto hook = scene.onPreChange;
        scene.onPreChange = nullptr;

        UndoStep undoStep;
        undoStep.actionName = m_redoStack.back().actionName;
        undoStep.sceneSnapshot = scene;
        undoStep.sceneSnapshot.onPreChange = nullptr;
        undoStep.selectedId = scene.selectedId;
        m_undoStack.push_back(std::move(undoStep));
        while (m_undoStack.size() > MAX_UNDO_STEPS) {
            m_undoStack.pop_front();
        }

        UndoStep target = std::move(m_redoStack.back());
        m_redoStack.pop_back();

        scene = std::move(target.sceneSnapshot);
        scene.onPreChange = hook;
        scene.selectedId = target.selectedId;
        scene.SyncLightPositionsFromActors();
        return true;
    }

    // Jump directly to an undo step (0-indexed from oldest to newest)
    bool JumpToUndoStep(size_t index, Scene& scene) {
        if (index >= m_undoStack.size()) return false;
        size_t stepsToUndo = m_undoStack.size() - 1 - index;
        for (size_t i = 0; i < stepsToUndo; ++i) {
            Undo(scene);
        }
        return true;
    }

    const std::deque<UndoStep>& GetUndoStack() const { return m_undoStack; }
    const std::deque<UndoStep>& GetRedoStack() const { return m_redoStack; }

    size_t GetUndoCount() const { return m_undoStack.size(); }
    size_t GetRedoCount() const { return m_redoStack.size(); }

    void Clear() {
        m_undoStack.clear();
        m_redoStack.clear();
    }

private:
    UndoManager() = default;
    std::deque<UndoStep> m_undoStack;
    std::deque<UndoStep> m_redoStack;
};
