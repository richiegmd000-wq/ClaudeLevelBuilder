#pragma once
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

class ClaudePopup : public geode::Popup<LevelEditorLayer*> {
protected:
    LevelEditorLayer* m_lel = nullptr;
    geode::TextInput* m_input = nullptr;
    CCLabelBMFont* m_status = nullptr;
    CCMenuItemSpriteExtra* m_sendBtn = nullptr;
    geode::EventListener<geode::utils::web::WebTask> m_listener;
    bool m_busy = false;

    bool setup(LevelEditorLayer* lel) override;
    void onSend(cocos2d::CCObject*);
    void handleResponse(std::string const& text);
    void setStatus(std::string const& s, cocos2d::ccColor3B col = {255, 255, 255});

public:
    static ClaudePopup* create(LevelEditorLayer* lel);
};
