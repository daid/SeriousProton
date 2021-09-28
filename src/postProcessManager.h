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
    
    void setUniform(string name, float value);
    
    static void setEnable(bool enable) { global_post_processor_enabled = enable; }
    static bool isEnabled() { return global_post_processor_enabled; }
};

#endif//POST_PROCESS_MANAGER_H
