#ifndef POST_PROCESS_MANAGER_H
#define POST_PROCESS_MANAGER_H

#include "graphics/shader.h"
#include "stringImproved.h"
#include "Updatable.h"
#include "Renderable.h"

class PostProcessor : public RenderChain
{
private:
    //sf::Shader shader;
    //sf::RenderTexture renderTexture;
    //sf::Vector2u size;
    
    RenderChain* chain;
    
    static bool global_post_processor_enabled;
public:
    bool enabled;
    
    PostProcessor(string name, RenderChain* chain);
    virtual ~PostProcessor() {}
    
    virtual void render(sp::RenderTarget& target) override;

    virtual bool onPointerMove(glm::vec2 position, sp::io::Pointer::ID id) override;
    virtual void onPointerLeave(sp::io::Pointer::ID id) override;
    virtual bool onPointerDown(sp::io::Pointer::Button button, glm::vec2 position, sp::io::Pointer::ID id) override;
    virtual void onPointerDrag(glm::vec2 position, sp::io::Pointer::ID id) override;
    virtual void onPointerUp(glm::vec2 position, sp::io::Pointer::ID id) override;
    virtual void onTextInput(const string& text) override;
    virtual void onTextInput(sp::TextInputEvent e) override;

    void setUniform(string name, float value);
    
    static void setEnable(bool enable) { global_post_processor_enabled = enable; }
    static bool isEnabled() { return global_post_processor_enabled; }
};

#endif//POST_PROCESS_MANAGER_H
