#ifndef RENDERABLE_H
#define RENDERABLE_H

#include "graphics/renderTarget.h"
#include "io/pointer.h"
#include "io/textinput.h"
#include "P.h"

class Renderable;
class RenderLayer;
extern RenderLayer* defaultRenderLayer;

class RenderChain : sp::NonCopyable
{
public:
    virtual void render(sp::RenderTarget& window) = 0;

    virtual bool onPointerMove(glm::vec2 position, sp::io::Pointer::ID id) { return false; }
    virtual void onPointerLeave(sp::io::Pointer::ID id) {}
    virtual bool onPointerDown(sp::io::Pointer::Button button, glm::vec2 position, sp::io::Pointer::ID id) { return false; }
    virtual void onPointerDrag(glm::vec2 position, sp::io::Pointer::ID id) {}
    virtual void onPointerUp(glm::vec2 position, sp::io::Pointer::ID id) {}
    virtual void onTextInput(const string& text) {}
    virtual void onTextInput(sp::TextInputEvent e) {}
};

class RenderLayer : public RenderChain
{
private:
    PVector<Renderable> renderableList;
    RenderChain* link;

public:
    bool active;

    RenderLayer();
    RenderLayer(RenderChain* link);
    
    virtual void render(sp::RenderTarget& target) override;

    virtual bool onPointerMove(glm::vec2 position, sp::io::Pointer::ID id) override;
    virtual void onPointerLeave(sp::io::Pointer::ID id) override;
    virtual bool onPointerDown(sp::io::Pointer::Button button, glm::vec2 position, sp::io::Pointer::ID id) override;
    virtual void onPointerDrag(glm::vec2 position, sp::io::Pointer::ID id) override;
    virtual void onPointerUp(glm::vec2 position, sp::io::Pointer::ID id) override;
    virtual void onTextInput(const string& text) override;
    virtual void onTextInput(sp::TextInputEvent e) override;

    friend class Renderable;
};

//Abstract class for entity that can be rendered.
class Renderable: public virtual PObject
{
    public:
        Renderable();
        Renderable(RenderLayer* renderLayer);
        virtual ~Renderable();
        virtual void render(sp::RenderTarget& target) = 0;
        void moveToRenderLayer(RenderLayer* renderLayer);
        RenderLayer* getRenderLayer();
        
        //Anything that can be rendered can process input.
        virtual bool onPointerMove(glm::vec2 position, sp::io::Pointer::ID id) { return false; }
        virtual void onPointerLeave(sp::io::Pointer::ID id) {}
        virtual bool onPointerDown(sp::io::Pointer::Button button, glm::vec2 position, sp::io::Pointer::ID id) { return false; }
        virtual void onPointerDrag(glm::vec2 position, sp::io::Pointer::ID id) {}
        virtual void onPointerUp(glm::vec2 position, sp::io::Pointer::ID id) {}
        virtual void onTextInput(const string& text) {}
        virtual void onTextInput(sp::TextInputEvent e) {}
    protected:
    private:
        RenderLayer* layer;
};

#endif // RENDERABLE_H
