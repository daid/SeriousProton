#ifndef POST_PROCESS_MANAGER_H
#define POST_PROCESS_MANAGER_H

#include <SFML/Graphics.hpp>

#include "stringImproved.h"
#include "Updatable.h"
#include "Renderable.h"

class PostProcessor : public RenderChain
{
private:
    sf::Shader shader;
    sf::RenderTexture renderTexture;
    
    RenderChain* chain;
public:
    bool enabled;
    
    PostProcessor(string name, RenderChain* chain);
    virtual ~PostProcessor() {}
    
    virtual void render(sf::RenderTarget& window);
    
    void setUniform(string name, float value);
};

#endif//POST_PROCESS_MANAGER_H
