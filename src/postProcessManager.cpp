#include "logging.h"
#include "postProcessManager.h"
#include "shaderManager.h"
#include "resources.h"
#include "graphics/opengl.h"
#include <stddef.h>


PostProcessor::PostProcessor(string shadername, RenderChain* chain)
: shader(ShaderManager::getShader(shadername)), render_texture({128, 128}), chain(chain), enabled{false}
{
}

void PostProcessor::render(sp::RenderTarget& target)
{
    if (!enabled)
    {
        chain->render(target);
        return;
    }
    target.finish();

    render_texture.setSize(target.getPhysicalSize());
    if (!render_texture.activateRenderTarget())
    {
        chain->render(target);
        return;
    }

    chain->render(target);
    target.finish();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    shader->bind();

    glUniform1i(shader->getUniformLocation("u_texture"), 0);
    glActiveTexture(GL_TEXTURE0);
    render_texture.bind();
    for(auto it : uniforms)
        glUniform1f(shader->getUniformLocation(it.first.c_str()), it.second);

    using VertexType = std::pair<glm::vec2, glm::vec2>;

    if (vertices_vbo == 0)
    {
        glGenBuffers(1, &vertices_vbo);
        glGenBuffers(1, &indices_vbo);

        std::vector<VertexType> vertex_data{
            {{-1, -1}, {0, 0}},
            {{-1,  1}, {0, 1}},
            {{ 1, -1}, {1, 0}},
            {{ 1,  1}, {1, 1}},
        };
        std::vector<uint16_t> index_data{0, 1, 2, 1, 3, 2};

        glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VertexType) * vertex_data.size(), vertex_data.data(), GL_DYNAMIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * index_data.size(), index_data.data(), GL_DYNAMIC_DRAW);
    }
    else
    {
        glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo);
    }

    glVertexAttribPointer(shader->getAttributeLocation("a_position"), 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(VertexType)), (void*)offsetof(VertexType, first));
    glEnableVertexAttribArray(shader->getAttributeLocation("a_position"));
    glVertexAttribPointer(shader->getAttributeLocation("a_texcoords"), 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(VertexType)), (void*)offsetof(VertexType, second));
    glEnableVertexAttribArray(shader->getAttributeLocation("a_texcoords"));

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_NONE);
}

void PostProcessor::setUniform(string name, float value)
{
    uniforms[name] = value;
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
