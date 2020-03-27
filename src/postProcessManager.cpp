#include "logging.h"
#include "postProcessManager.h"

bool PostProcessor::global_post_processor_enabled = true;

static int powerOfTwo(int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

PostProcessor::PostProcessor(string name, RenderChain* chain)
: chain(chain)
{
    if (sf::Shader::isAvailable())
    {
        if (shader.loadFromFile("resources/" + name + ".frag", sf::Shader::Fragment))
        {
            LOG(INFO) << "Loaded shader: " << name;
            enabled = true;
        }else{
            LOG(WARNING) << "Failed to load shader:" << name;
            enabled = false;
        }
    }else{
        LOG(WARNING) << "Did not load load shader: " << name;
        LOG(WARNING) << "Because of no shader support in video card driver.";
        enabled = false;
    }
}

void PostProcessor::render(sf::RenderTarget& window)
{
    if (!enabled || !sf::Shader::isAvailable() || !global_post_processor_enabled)
    {
        chain->render(window);
        return;
    }

    sf::View view(window.getView());

    // If the window or texture size is 0/impossible, or if the window size has
    // changed, resize the viewport, render texture, and input/textureSizes.
    if (size.x < 1 || renderTexture.getSize().x < 1 || size != window.getSize())
    {
        size = window.getSize();

        //Setup a backBuffer to render the game on. Then we can render the backbuffer back to the main screen with full-screen shader effects
        int w = view.getViewport().width * size.x;
        int h = view.getViewport().height * size.y;
        int tw = powerOfTwo(w);
        int th = powerOfTwo(h);
        view.setViewport(sf::FloatRect(0, 1.0 - float(h) / float(th), float(w) / float(tw), float(h) / float(th)));

        renderTexture.create(tw, th, true);
        renderTexture.setRepeated(true);
        renderTexture.setSmooth(true);
        renderTexture.setView(view);

        shader.setUniform("inputSize", sf::Vector2f(size.x, size.y));
        shader.setUniform("textureSize", sf::Vector2f(renderTexture.getSize().x, renderTexture.getSize().y));
    }

    renderTexture.clear(sf::Color(20, 20, 20));
    chain->render(renderTexture);

    renderTexture.display();

    sf::Sprite backBufferSprite(renderTexture.getTexture(), sf::IntRect(0, renderTexture.getSize().y - view.getViewport().height * size.y, view.getViewport().width * size.x, view.getViewport().height * size.y));
    backBufferSprite.setScale(view.getSize().x / float(renderTexture.getSize().x) / renderTexture.getView().getViewport().width, view.getSize().y / float(renderTexture.getSize().y) / renderTexture.getView().getViewport().height);

    window.draw(backBufferSprite, &shader);
}

void PostProcessor::setUniform(string name, float value)
{
    if (sf::Shader::isAvailable() && global_post_processor_enabled)
        shader.setUniform(name, value);
}
