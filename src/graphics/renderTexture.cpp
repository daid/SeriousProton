#include <graphics/renderTexture.h>
#include <graphics/opengl.h>


namespace sp {

RenderTexture::RenderTexture(glm::ivec2 size)
: size(size)
{
}

RenderTexture::~RenderTexture()
{
    if (frame_buffer)
    {
        glDeleteFramebuffers(1, &frame_buffer);
        glDeleteTextures(1, &color_buffer);
        glDeleteRenderbuffers(1, &depth_buffer);
    }
}

bool RenderTexture::create()
{
    if (!glad_glGenFramebuffers)
        return false;
    if (!frame_buffer)
    {
        glGenFramebuffers(1, &frame_buffer);
        glGenTextures(1, &color_buffer);
        glGenRenderbuffers(1, &depth_buffer);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

    glBindTexture(GL_TEXTURE_2D, color_buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_buffer, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.x, size.y);
    if (gl::contextIsES)
    {
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
    } else {
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
    }

    bool result = true;
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        result = false;
        glDeleteFramebuffers(1, &frame_buffer);
        glDeleteTextures(1, &color_buffer);
        glDeleteRenderbuffers(1, &depth_buffer);
        frame_buffer = color_buffer = depth_buffer = 0;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    return result;
}

void RenderTexture::bind()
{
    if (dirty)
    {
        glFlush(); //Is this needed?
        dirty = false;
    }
    glBindTexture(GL_TEXTURE_2D, color_buffer);
}

void RenderTexture::setSize(glm::ivec2 new_size)
{
    if (size != new_size)
    {
        size = new_size;
        create_buffers = true;
    }
}

glm::ivec2 RenderTexture::getSize() const
{
    return size;
}

bool RenderTexture::activateRenderTarget()
{
    if (create_buffers)
    {
        create_buffers = false;
        if (!create())
            return false;
    }
    if (!frame_buffer)
        return false;

    dirty = true;
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    return true;
}

}//namespace sp
