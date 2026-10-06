#include "ClaudePopup.hpp"
#include "SystemPrompt.hpp"

using namespace geode::prelude;

ClaudePopup* ClaudePopup::create(LevelEditorLayer* lel) {
    auto ret = new ClaudePopup();
    if (ret->initAnchored(380.f, 220.f, lel)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ClaudePopup::setup(LevelEditorLayer* lel) {
    m_lel = lel;
    this->setTitle("Claude Level Builder");

    auto size = m_mainLayer->getContentSize();

    m_input = TextInput::create(340.f, "Describe the level (theme, length, gamemodes, decoration)...", "chatFont.fnt");
    m_input->setPosition({size.width / 2, size.height / 2 + 25.f});
    m_input->setMaxCharCount(1500);
    m_input->setCommonFilter(CommonFilter::Any);
    m_mainLayer->addChild(m_input);

    m_status = CCLabelBMFont::create("Ready.", "chatFont.fnt");
    m_status->setScale(0.6f);
    m_status->setPosition({size.width / 2, size.height / 2 - 20.f});
    m_mainLayer->addChild(m_status);

    auto spr = ButtonSprite::create("Build with Claude");
    m_sendBtn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ClaudePopup::onSend));
    m_sendBtn->setPosition({size.width / 2, 35.f});
    m_buttonMenu->addChild(m_sendBtn);

    return true;
}

void ClaudePopup::setStatus(std::string const& s, ccColor3B col) {
    m_status->setString(s.c_str());
    m_status->setColor(col);
}

void ClaudePopup::onSend(CCObject*) {
    if (m_busy) return;

    auto key = Mod::get()->getSettingValue<std::string>("api-key");
    if (key.empty()) {
        setStatus("Set your API key in the mod settings first.", {255, 90, 90});
        return;
    }
    auto prompt = m_input->getString();
    if (prompt.empty()) {
        setStatus("Type a description first.", {255, 90, 90});
        return;
    }

    auto model = Mod::get()->getSettingValue<std::string>("model");
    auto maxTokens = Mod::get()->getSettingValue<int64_t>("max-tokens");

    matjson::Value msg = matjson::makeObject({
        {"role", "user"},
        {"content", prompt}
    });
    matjson::Value body = matjson::makeObject({
        {"model", model},
        {"max_tokens", maxTokens},
        {"system", std::string(CLAUDE_SYSTEM_PROMPT)},
        {"messages", std::vector<matjson::Value>{msg}}
    });

    m_busy = true;
    setStatus("Claude is designing your level... (can take a few minutes)", {255, 220, 90});

    m_listener.bind([this](web::WebTask::Event* e) {
        if (auto* res = e->getValue()) {
            m_busy = false;
            if (!res->ok()) {
                setStatus("API error " + std::to_string(res->code()), {255, 90, 90});
                log::error("Anthropic API error: {}", res->string().unwrapOr("?"));
                return;
            }
            auto json = res->json();
            if (json.isErr()) {
                setStatus("Bad response JSON.", {255, 90, 90});
                return;
            }
            auto& root = json.unwrap();
            std::string text;
            if (root.contains("content") && root["content"].isArray()) {
                for (auto& block : root["content"].asArray().unwrapOr(std::vector<matjson::Value>{})) {
                    if (block["type"].asString().unwrapOr("") == "text") {
                        text += block["text"].asString().unwrapOr("");
                    }
                }
            }
            handleResponse(text);
        } else if (e->isCancelled()) {
            m_busy = false;
            setStatus("Request cancelled.", {255, 90, 90});
        }
    });

    web::WebRequest req;
    req.header("x-api-key", key);
    req.header("anthropic-version", "2023-06-01");
    req.header("content-type", "application/json");
    req.timeout(std::chrono::seconds(600));
    req.bodyJSON(body);
    m_listener.setFilter(req.post("https://api.anthropic.com/v1/messages"));
}

void ClaudePopup::handleResponse(std::string const& raw) {
    auto start = raw.find('{');
    auto end = raw.rfind('}');
    if (start == std::string::npos || end == std::string::npos || end < start) {
        setStatus("Claude returned no JSON.", {255, 90, 90});
        return;
    }
    auto parsed = matjson::parse(raw.substr(start, end - start + 1));
    if (parsed.isErr()) {
        setStatus("Could not parse Claude's level data.", {255, 90, 90});
        log::error("Parse failure: {}", parsed.unwrapErr());
        return;
    }
    auto& root = parsed.unwrap();
    if (!root.contains("objects") || !root["objects"].isArray()) {
        setStatus("No objects array in response.", {255, 90, 90});
        return;
    }

    auto maxObjects = static_cast<size_t>(Mod::get()->getSettingValue<int64_t>("max-objects"));
    size_t placed = 0;

    for (auto& o : root["objects"].asArray().unwrapOr(std::vector<matjson::Value>{})) {
        if (placed >= maxObjects) break;
        int id = o["id"].asInt().unwrapOr(0);
        if (id <= 0 || id > 5000) continue;
        float x = static_cast<float>(o["x"].asDouble().unwrapOr(0.0));
        float y = static_cast<float>(o["y"].asDouble().unwrapOr(0.0));
        float rot = static_cast<float>(o["rot"].asDouble().unwrapOr(0.0));
        float scale = static_cast<float>(o["scale"].asDouble().unwrapOr(1.0));
        bool fx = o["flipx"].asBool().unwrapOr(false);
        bool fy = o["flipy"].asBool().unwrapOr(false);

        auto obj = m_lel->createObject(id, ccp(x, y), false);
        if (!obj) continue;
        if (rot != 0.f) obj->setRotation(rot);
        if (scale != 1.f) {
            obj->updateCustomScaleX(scale);
            obj->updateCustomScaleY(scale);
        }
        if (fx) obj->setFlipX(true);
        if (fy) obj->setFlipY(true);
        placed++;
    }

    setStatus("Placed " + std::to_string(placed) + " objects!", {90, 255, 120});
    FLAlertLayer::create("Claude Level Builder",
        "Placed " + std::to_string(placed) + " objects. Save your level, then test it!", "OK")->show();
}
