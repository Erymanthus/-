#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class StatusSpinner : public CCNode {
protected:
    CCSprite* m_spinner = nullptr;
    CCLabelBMFont* m_label = nullptr;

   
    void updateIcon(const char* name, bool isLoading) {
     
        if (m_spinner) {
            m_spinner->removeFromParent();
            m_spinner = nullptr;
        }

       
        m_spinner = CCSprite::create(name);

      
        bool isFallback = false;
        if (!m_spinner && isLoading) {
            m_spinner = CCSprite::create("loading-circle.png");
            isFallback = true;
        }

     
        if (m_spinner) {
            this->addChild(m_spinner);

            if (isLoading) {       
                if (isFallback) {
                    m_spinner->runAction(CCRepeatForever::create(CCRotateBy::create(1.0f, 360.f)));
                }
                else {
                
                    m_spinner->setScale(1.0f);
                }
            }
            else {
             
                m_spinner->stopAllActions();
                m_spinner->setRotation(0);
                m_spinner->setScale(1.0f);
            }
        }
    }

    bool init() override {
        if (!CCNode::init()) return false;

        this->updateIcon("loading.gif"_spr, true);

       
        m_label = CCLabelBMFont::create("", "bigFont.fnt");
        m_label->setScale(0.3f);
        m_label->setPosition({ 0, -35.f });
        this->addChild(m_label);

        this->setVisible(false);
        return true;
    }

public:
    static StatusSpinner* create() {
        auto ret = new StatusSpinner();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

  

    void setLoading(std::string text) {
        this->setVisible(true);

        this->updateIcon("loading.gif"_spr, true);

        m_label->setString(text.c_str());
        m_label->setColor({ 255, 255, 255 });
    }

    void setError(std::string text) {
        this->setVisible(true);

      
        this->updateIcon("error_face.png"_spr, false);

        m_label->setString(text.c_str());
        m_label->setColor({ 255, 80, 80 });
    }

    void setSuccess(std::string text) {
        this->setVisible(true);

       
        if (m_spinner) m_spinner->setVisible(false);

        m_label->setString(text.c_str());
        m_label->setColor({ 80, 255, 80 }); 
    }

    void hide() {
        this->setVisible(false);
        if (m_spinner) m_spinner->stopAllActions();
    }
};