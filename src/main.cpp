#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "ClaudePopup.hpp"

using namespace geode::prelude;

class $modify(ClaudeEditorUI, EditorUI) {
    bool init(LevelEditorLayer* lel) {
        if (!EditorUI::init(lel)) return false;

        auto winSize = CCDirector::get()->getWinSize();
        auto menu = CCMenu::create();
        menu->setID("claude-builder-menu"_spr);
        menu->setPosition({0, 0});

        auto spr = ButtonSprite::create("Claude", "goldFont.fnt", "GJ_button_04.png", 0.6f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ClaudeEditorUI::onClaude));
        btn->setPosition({winSize.width - 50.f, winSize.height - 30.f});
        menu->addChild(btn);
        this->addChild(menu, 100);
        return true;
    }

    void onClaude(CCObject*) {
        if (auto p = ClaudePopup::create(m_editorLayer)) p->show();
    }
};
