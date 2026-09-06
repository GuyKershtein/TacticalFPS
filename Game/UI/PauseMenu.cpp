#include "PauseMenu.h"

#include "../../Engine/UI/Font.h"
#include "../../Engine/UI/TextRenderer.h"
#include "../../Engine/UI/UIRenderer.h"
#include "../../Engine/Input/InputManager.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

namespace Game {

bool PauseMenu::Update(Engine::InputManager& input, bool& outShouldResume) {
    // Escape is deliberately not read here — GameApplication::ProcessInput
    // already consumes it (WasKeyPressed mutates the key's edge-detection
    // state, so a second WasKeyPressed(ESCAPE) call this frame would always
    // see the edge as already-consumed and report false). [R] is this
    // menu's own, unambiguous resume key.
    outShouldResume = input.WasKeyPressed(GLFW_KEY_R);
    return input.WasKeyPressed(GLFW_KEY_Q);
}

void PauseMenu::Render(Engine::TextRenderer& text, Engine::UIRenderer& ui, const Engine::Font& font,
    int screenWidth, int screenHeight) const {
    const float w = static_cast<float>(screenWidth);
    const float h = static_cast<float>(screenHeight);

    ui.DrawRect(0.0f, 0.0f, w, h, glm::vec3(0.0f), 0.6f);

    constexpr float kPanelWidth = 320.0f;
    constexpr float kPanelHeight = 160.0f;
    const float panelX = (w - kPanelWidth) * 0.5f;
    const float panelY = (h - kPanelHeight) * 0.5f;
    ui.DrawRect(panelX, panelY, kPanelWidth, kPanelHeight, glm::vec3(0.08f), 0.92f);

    const char* title = "PAUSED";
    const float titleWidth = font.MeasureWidth(title);
    text.Draw(font, title, panelX + (kPanelWidth - titleWidth) * 0.5f, panelY + 20.0f, glm::vec3(1.0f));

    const char* resumeLine = "[R] or [ESC] Resume";
    const float resumeWidth = font.MeasureWidth(resumeLine);
    text.Draw(font, resumeLine, panelX + (kPanelWidth - resumeWidth) * 0.5f, panelY + 70.0f, glm::vec3(0.8f));

    const char* quitLine = "[Q] Quit to desktop";
    const float quitWidth = font.MeasureWidth(quitLine);
    text.Draw(font, quitLine, panelX + (kPanelWidth - quitWidth) * 0.5f, panelY + 100.0f, glm::vec3(0.8f));
}

} // namespace Game
