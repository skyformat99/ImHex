#include <hex/views/view.hpp>

#include <imgui.h>

#include <functional>
#include <string>
#include <vector>

#include <hex/helpers/shared_data.hpp>

namespace hex {


    View::View(std::string viewName) : m_viewName(viewName) { }

    void View::drawMenu() { }
    bool View::handleShortcut(int key, int mods) { return false; }

    std::vector<std::function<void()>>& View::getDeferedCalls() {
        return SharedData::deferredCalls;
    }

    void View::postEvent(Events eventType, const void *userData) {
        EventManager::post(eventType, userData);
    }

    void View::drawCommonInterfaces() {
        if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::NewLine();
            if (ImGui::BeginChild("##scrolling", ImVec2(300, 100))) {
                ImGui::SetCursorPosX((300 - ImGui::CalcTextSize(SharedData::errorPopupMessage.c_str(), nullptr, false).x) / 2.0F);
                ImGui::TextWrapped("%s", SharedData::errorPopupMessage.c_str());
                ImGui::EndChild();
            }
            ImGui::NewLine();
            ImGui::SetCursorPosX(75);
            if (ImGui::Button("Okay", ImVec2(150, 20)) || ImGui::IsKeyDown(ImGuiKey_Escape))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }

    void View::showErrorPopup(std::string_view errorMessage) {
        SharedData::errorPopupMessage = errorMessage;

        ImGui::OpenPopup("Error");
    }

    bool View::hasViewMenuItemEntry() {
        return true;
    }

    ImVec2 View::getMinSize() {
        return ImVec2(480, 720);
    }

    ImVec2 View::getMaxSize() {
        return ImVec2(FLT_MAX, FLT_MAX);
    }


    bool& View::getWindowOpenState() {
        return this->m_windowOpen;
    }

    const std::string View::getName() const {
        return this->m_viewName;
    }

    void View::subscribeEvent(Events eventType, std::function<void(const void*)> callback) {
        EventManager::subscribe(eventType, this, callback);
    }

    void View::unsubscribeEvent(Events eventType) {
        EventManager::unsubscribe(eventType, this);
    }

    void View::doLater(std::function<void()> &&function) {
        SharedData::deferredCalls.push_back(function);
    }

    void View::confirmButtons(const char *textLeft, const char *textRight, std::function<void()> leftButtonFn, std::function<void()> rightButtonFn) {
        auto width = ImGui::GetWindowWidth();
        ImGui::SetCursorPosX(width / 9);
        if (ImGui::Button(textLeft, ImVec2(width / 3, 0)))
            leftButtonFn();
        ImGui::SameLine();
        ImGui::SetCursorPosX(width / 9 * 5);
        if (ImGui::Button(textRight, ImVec2(width / 3, 0)))
            rightButtonFn();
    }

}