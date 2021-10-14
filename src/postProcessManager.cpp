#include "logging.h"
#include "postProcessManager.h"
#include "resources.h"

bool PostProcessor::global_post_processor_enabled = true;

PostProcessor::PostProcessor(string name, RenderChain* chain)
: chain(chain), enabled{false}
{
// SDL2 TODO post processors not implemented, might remove them?
    /*
    if (sf::Shader::isAvailable())
    {
        if (auto shader_frag = getResourceStream(name + ".frag"); shader_frag)
        {
            if (shader.loadFromStream(**shader_frag, sf::Shader::Fragment))
            {
                LOG(INFO) << "Loaded shader: " << name;
                enabled = true;
            }
            else {
                LOG(WARNING) << "Failed to load shader:" << name;
            }
        }
        else
        {
            LOG(WARNING) << "Failed to open shader stream for " << name;
        }
    }else{
        LOG(WARNING) << "Did not load load shader: " << name;
        LOG(WARNING) << "Because of no shader support in video card driver.";
    }
    */
}

void PostProcessor::render(sp::RenderTarget& target)
{/*
    if (!enabled || !sf::Shader::isAvailable() || !global_post_processor_enabled)
    {*/
        chain->render(target);/*
        return;
    } 

    //Hack the rectangle for this element so it sits perfectly on pixel boundaries.
    sf::Vector2u pixel_size{ target.getSFMLTarget().getSize() };

    // If the window or texture size is 0/impossible, or if the window size has
    // changed, resize the viewport, render texture, and input/textureSizes.
    if (size.x < 1 || renderTexture.getSize().x < 1 || size != pixel_size)
    {
        size = pixel_size;

        //Setup a backBuffer to render the game on. Then we can render the backbuffer back to the main screen with full-screen shader effects
        sf::ContextSettings settings{ 24, 8 }; // 24/8 depth/stencil.
        if (!renderTexture.create(size.x, size.y, settings))
        {
            // If we fail to create the RT, just disable the post processor and fallback to the backbuffer.
            LOG(WARNING) << "Failed to setup the render texture for post processing effects. They will be disabled.";
            global_post_processor_enabled = false;
            chain->render(target);
            return;
        }

        renderTexture.setRepeated(true);
        renderTexture.setSmooth(true);

        shader.setUniform("inputSize", sf::Vector2f{ size });
        shader.setUniform("textureSize", sf::Vector2f{ renderTexture.getSize() });
    }

    // The view can evolve independently from the window size - update every frame.
    renderTexture.setView(target.getSFMLTarget().getView());
    renderTexture.clear(sf::Color(20, 20, 20));
   
    sp::RenderTarget textureTarget(renderTexture);
    chain->render(textureTarget);

    renderTexture.display();

    // The RT is a fullscreen texture.
    // Setup the view to cover the entire RT.
    target.getSFMLTarget().setView(sf::View({ 0.f, 0.f, static_cast<float>(renderTexture.getSize().x), static_cast<float>(renderTexture.getSize().y) }));
    
    sf::Sprite backBufferSprite(renderTexture.getTexture());
    target.getSFMLTarget().draw(backBufferSprite, &shader);

    // Restore view
    target.getSFMLTarget().setView(renderTexture.getView());
    */
}

void PostProcessor::setUniform(string name, float value)
{
    /*
    if (sf::Shader::isAvailable() && global_post_processor_enabled)
        shader.setUniform(name, value);
        */
}

bool PostProcessor::onPointerMove(glm::vec2 position, sp::io::Pointer::ID id)
{
    return chain->onPointerMove(position, id);
}

void PostProcessor::onPointerLeave(sp::io::Pointer::ID id)
{
    chain->onPointerLeave(id);
}

bool PostProcessor::onPointerDown(sp::io::Pointer::Button button, glm::vec2 position, sp::io::Pointer::ID id)
{
    return chain->onPointerDown(button, position, id);
}

void PostProcessor::onPointerDrag(glm::vec2 position, sp::io::Pointer::ID id)
{
    chain->onPointerDrag(position, id);
}

void PostProcessor::onPointerUp(glm::vec2 position, sp::io::Pointer::ID id)
{
    chain->onPointerUp(position, id);
}

void PostProcessor::onTextInput(const string& text)
{
    chain->onTextInput(text);
}

void PostProcessor::onTextInput(sp::TextInputEvent e)
{
    chain->onTextInput(e);
}
