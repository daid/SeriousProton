#include "graphics/renderTarget.h"
#include "graphics/textureAtlas.h"
#include "textureManager.h"
#include "windowManager.h"
#include "engine.h"

#include "graphics/ktx2texture.h"
#include "graphics/opengl.h"
#include "graphics/shader.h"
#include "vectorUtils.h"
#include <glm/gtc/type_ptr.hpp>
#include <variant>

#include <SDL_assert.h>


namespace sp {

static sp::Font* default_font = nullptr;

static sp::Shader* shader = nullptr;
static sp::Shader* wide_line_shader = nullptr;
static unsigned int vertices_vbo = 0;
static unsigned int indices_vbo = 0;

// Vertex buffers
static std::vector<RenderTarget::VertexData> vertex_data;
static std::vector<uint16_t> index_data;

// GL lines buffers
static std::vector<RenderTarget::VertexData> lines_vertex_data;
static std::vector<uint16_t> lines_index_data;

// Point buffers
static std::vector<RenderTarget::VertexData> points_vertex_data;
static std::vector<uint16_t> points_index_data;

// Anti-aliased quad line buffers
static std::vector<RenderTarget::VertexData> quad_lines_vertex_data;
static std::vector<uint16_t> quad_lines_index_data;

// Circle/rect point generation buffer
static thread_local std::vector<glm::vec2> shape_points_buffer;

// Calculate optimal point count for a circle based on its screen-space radius
static size_t calculateCirclePointCount(float radius, float points_per_pixel = 0.5f)
{
    size_t count = static_cast<size_t>(std::ceil(2.0f * static_cast<float>(M_PI) * radius * points_per_pixel));
    // Clamp between reasonable bounds
    return std::max(static_cast<size_t>(16), std::min(count, static_cast<size_t>(360)));
}


struct ImageInfo
{
    Texture* texture;
    glm::ivec2 size;
    Rect uv_rect;
};
static sp::AtlasTexture* atlas_texture;
static std::unordered_map<string, ImageInfo> image_info;
static std::unordered_map<sp::Font*, std::unordered_map<int, Rect>> atlas_glyphs;
static constexpr glm::ivec2 atlas_size = {2048, 2048};
static constexpr glm::vec2 atlas_white_pixel = {(float(atlas_size.x)-0.5f)/float(atlas_size.x), (float(atlas_size.y)-0.5f)/float(atlas_size.y)};


static ImageInfo getTextureInfo(std::string_view texture)
{
    auto it = image_info.find(texture);
    if (it != image_info.end())
        return it->second;

    P<ResourceStream> stream;
    // filename variants:
    //  name
    //  name.notanextension
    //  name.ext
    //  name.notanext.ext
    // Attempt to load the best version.
    auto last_dot = texture.find_last_of('.');
    if (last_dot != std::string::npos)
    {
        // Extension found, try and substitute it.
        stream = getResourceStream(string(texture.substr(0, static_cast<uint32_t>(last_dot))) + ".ktx2");
    }

    if (!stream)
    {
        // No extension, or substitution failed (maybe it wasn't an extension), blindly add it.
        stream = getResourceStream(string(texture) + ".ktx2");
    }

    constexpr glm::ivec2 atlas_threshold{ 128, 128 };
    KTX2Texture ktxtexture;
    Image image;
    if (stream)
    {
        if (ktxtexture.loadFromStream(stream))
        {
            auto size = ktxtexture.getSize();
            if (size.x > atlas_threshold.x || size.y > atlas_threshold.y)
            {
                auto mip_level = std::min(textureManager.getBaseMipLevel(), ktxtexture.getMipCount() - 1);
                size = ktxtexture.getSize(mip_level);
                auto gltexture = ktxtexture.toTexture(mip_level);
                if (gltexture)
                {
                    LOG(Info, "Loaded ", texture.data(), " (ktx2)");
                    image_info[texture] = { gltexture.get(), size, {0.0f, 0.0f, 1.0f, 1.0f} };
                    return { gltexture.release(), size, {0.0f, 0.0f, 1.0f, 1.0f} };
                }
                else
                {
                    LOG(Warning, "[ktx2]: ", texture.data(), " failed to load into texture.");
                }
            }
            else if (auto to_image = ktxtexture.toImage(); to_image.has_value())
            {
                image = std::move(to_image.value());
            }
            else
                LOG(Warning, "[ktx2]: ", texture.data(), " failed to load into image.");
        }
        else
        {
            LOG(Warning, "[ktx2]: ", texture.data(), " failed to read stream.");
        }

        // failed to load from the KTX2 file - fallback on image.
        stream = nullptr;
    }

    if (!stream)
    {
        stream = getResourceStream(texture);
        if (!stream)
            stream = getResourceStream(string(texture) + ".png");
        image.loadFromStream(stream);
    }
    
    auto size = image.getSize();
    if (size.x > atlas_threshold.x || size.y > atlas_threshold.y)
    {
        LOG(Info, "Loaded ", string(texture));
        auto gltexture = new sp::BasicTexture(image);
        image_info[texture] = {gltexture, size, {0.0f, 0.0f, 1.0f, 1.0f}};
        return {gltexture, size, {0.0f, 0.0f, 1.0f, 1.0f}};
    }

    Rect uv_rect = atlas_texture->add(std::move(image), 1);
    image_info[texture] = {nullptr, size, uv_rect};
    LOG(Info, "Added ", string(texture), " to atlas@", uv_rect.position, " ", uv_rect.size, "  ", atlas_texture->usageRate() * 100.0f, "%");
    return {nullptr, size, uv_rect};
}

RenderTarget::RenderTarget(glm::vec2 virtual_size, glm::ivec2 physical_size)
: virtual_size(virtual_size), physical_size(physical_size)
{
    if (!shader)
        shader = new Shader("rendertargetshader", R"(
[vertex]
uniform mat3 u_projection;

attribute vec2 a_position;
attribute vec2 a_texcoords;
attribute vec4 a_color;

varying vec2 v_texcoords;
varying vec4 v_color;

void main()
{
    v_texcoords = a_texcoords;
    v_color = a_color;
    gl_Position = vec4(u_projection * vec3(a_position, 1.0), 1.0);
}

[fragment]
uniform sampler2D u_texture;

varying vec2 v_texcoords;
varying vec4 v_color;

void main()
{
    gl_FragColor = texture2D(u_texture, v_texcoords) * v_color;
}
)");
    if (!wide_line_shader)
        wide_line_shader = new Shader("widelineshader", R"(
[vertex]
uniform mat3 u_projection;

attribute vec2 a_position;
attribute vec2 a_texcoords;
attribute vec4 a_color;

varying vec2 v_texcoords;
varying vec4 v_color;

void main()
{
    v_texcoords = a_texcoords;
    v_color = a_color;
    gl_Position = vec4(u_projection * vec3(a_position, 1.0), 1.0);
}

[fragment]
varying vec2 v_texcoords;
varying vec4 v_color;

void main()
{
    // v_texcoords.x: 0.0 = no AA, 1.0 = AA enabled
    // v_texcoords.y: signed distance from line center (-1 to 1, where |1| is the edge)
    float dist = abs(v_texcoords.y);
    float aa = v_texcoords.x;
    // When AA is enabled, smoothstep from 0.8 to 1.0 for soft edge
    // When AA is disabled, use step at 1.0 for hard edge
    float alpha = mix(step(dist, 1.0), 1.0 - smoothstep(0.7, 1.0, dist), aa);
    gl_FragColor = vec4(v_color.rgb, v_color.a * alpha);
}
)");
    if (!vertices_vbo)
    {
        glGenBuffers(1, &vertices_vbo);
        glGenBuffers(1, &indices_vbo);
    }
    shader->bind();

    glm::mat3 project_matrix{1.0f};
    project_matrix[0][0] = 2.0f / float(virtual_size.x);
    project_matrix[1][1] = -2.0f / float(virtual_size.y);
    project_matrix[2][0] = -1.0f;
    project_matrix[2][1] = 1.0f;
    glUniformMatrix3fv(shader->getUniformLocation("u_projection"), 1, GL_FALSE, glm::value_ptr(project_matrix));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!atlas_texture)
        atlas_texture = new AtlasTexture(atlas_size);
}

void RenderTarget::setDefaultFont(sp::Font* font)
{
    default_font = font;
}

sp::Font* RenderTarget::getDefaultFont()
{
    return default_font;
}

void RenderTarget::drawSprite(std::string_view texture, glm::vec2 center, float size, glm::u8vec4 color)
{
    auto info = getTextureInfo(texture);
    if (info.texture || vertex_data.size() >= std::numeric_limits<uint16_t>::max() - 4U)
        finish();

    auto n = vertex_data.size();
    index_data.insert(index_data.end(), {
        uint16_t(n), uint16_t(n + 1), uint16_t(n + 2),
        uint16_t(n + 1), uint16_t(n + 3), uint16_t(n + 2),
    });
    size *= 0.5f;
    glm::vec2 offset{size / static_cast<float>(info.size.y) * static_cast<float>(info.size.x), size};
    vertex_data.push_back({
        {center.x - offset.x, center.y - offset.y},
        color, {info.uv_rect.position.x, info.uv_rect.position.y}});
    vertex_data.push_back({
        {center.x - offset.x, center.y + offset.y},
        color, {info.uv_rect.position.x, info.uv_rect.position.y + info.uv_rect.size.y}});
    vertex_data.push_back({
        {center.x + offset.x, center.y - offset.y},
        color, {info.uv_rect.position.x + info.uv_rect.size.x, info.uv_rect.position.y}});
    vertex_data.push_back({
        {center.x + offset.x, center.y + offset.y},
        color, {info.uv_rect.position.x + info.uv_rect.size.x, info.uv_rect.position.y + info.uv_rect.size.y}});

    if (info.texture)
        finish(info.texture);
}

void RenderTarget::drawSpriteClipped(std::string_view texture, glm::vec2 center, float size, sp::Rect clip_rect, glm::u8vec4 color)
{
    if (clip_rect.size.x < 0 || clip_rect.size.y < 0)
        return;

    auto info = getTextureInfo(texture);
    if (info.texture || vertex_data.size() >= std::numeric_limits<uint16_t>::max() - 4U)
        finish();

    auto n = vertex_data.size();
    index_data.insert(index_data.end(), {
        uint16_t(n), uint16_t(n + 1), uint16_t(n + 2),
        uint16_t(n + 1), uint16_t(n + 3), uint16_t(n + 2),
    });

    size *= 0.5f;
    glm::vec2 offset{size / float(info.size.y) * float(info.size.x), size};

    float x0 = center.x - offset.x;
    float x1 = center.x + offset.x;
    float y0 = center.y - offset.y;
    float y1 = center.y + offset.y;

    const auto& uv_rect = info.uv_rect;
    float u0 = uv_rect.position.x;
    float u1 = uv_rect.position.x + uv_rect.size.x;
    float v0 = uv_rect.position.y;
    float v1 = uv_rect.position.y + uv_rect.size.y;

    if (x1 < clip_rect.position.x)
    {
        return;
    }
    else if (x0 < clip_rect.position.x)
    {
        u0 = u1 - uv_rect.size.x * (clip_rect.position.x - x1) / (x0 - x1);
        x0 = clip_rect.position.x;
    }

    if (x0 > clip_rect.position.x + clip_rect.size.x)
    {
        return;
    }
    else if (x1 > clip_rect.position.x + clip_rect.size.x)
    {
        u1 = u0 + uv_rect.size.x * ((clip_rect.position.x + clip_rect.size.x) - x0) / (x1 - x0);
        x1 = clip_rect.position.x + clip_rect.size.x;
    }

    if (y1 < clip_rect.position.y)
    {
        return;
    }
    else if (y0 < clip_rect.position.y)
    {
        v0 = v1 - uv_rect.size.y * (clip_rect.position.y - y1) / (y0 - y1);
        y0 = clip_rect.position.y;
    }

    if (y0 > clip_rect.position.y + clip_rect.size.y)
    {
        return;
    }
    else if (y1 > clip_rect.position.y + clip_rect.size.y)
    {
        v1 = v0 + uv_rect.size.y * (clip_rect.size.y - y0) / (y1 - y0);
        y1 = clip_rect.position.y + clip_rect.size.y;
    }

    vertex_data.push_back({
        {x0, y0}, color, {u0, v0}
    });
    vertex_data.push_back({
        {x1, y0}, color, {u1, v0}
    });
    vertex_data.push_back({
        {x0, y1}, color, {u0, v1}
    });
    vertex_data.push_back({
        {x1, y1}, color, {u1, v1}
    });

    if (info.texture)
        finish(info.texture);
}

void RenderTarget::drawRotatedSprite(std::string_view texture, glm::vec2 center, float size, float rotation, glm::u8vec4 color)
{
    if (rotation == 0)
        return drawSprite(texture, center, size, color);
    auto info = getTextureInfo(texture);
    if (info.texture || vertex_data.size() >= std::numeric_limits<uint16_t>::max() - 4U)
        finish();
    auto& uv_rect = info.uv_rect;

    auto n = vertex_data.size();
    index_data.insert(index_data.end(), {
        uint16_t(n + 0), uint16_t(n + 1), uint16_t(n + 2),
        uint16_t(n + 1), uint16_t(n + 3), uint16_t(n + 2),
    });
    size *= 0.5f;
    glm::vec2 offset0 = rotateVec2({size / uv_rect.size.y * uv_rect.size.x, size}, rotation);
    glm::vec2 offset1{offset0.y, -offset0.x};
    vertex_data.push_back({
        center - offset0,
        color, {uv_rect.position.x, uv_rect.position.y}});
    vertex_data.push_back({
        center - offset1,
        color, {uv_rect.position.x, uv_rect.position.y + uv_rect.size.y}});
    vertex_data.push_back({
        center + offset1,
        color, {uv_rect.position.x + uv_rect.size.x, uv_rect.position.y}});
    vertex_data.push_back({
        center + offset0,
        color, {uv_rect.position.x + uv_rect.size.x, uv_rect.position.y + uv_rect.size.y}});
    
    if (info.texture)
        finish(info.texture);
}

void RenderTarget::drawRotatedSpriteBlendAdd(std::string_view texture, glm::vec2 center, float size, float rotation)
{
    finish();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    drawRotatedSprite(texture, center, size, rotation);
    finish();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RenderTarget::drawGLLine(glm::vec2 start, glm::vec2 end, glm::u8vec4 color)
{
    if (lines_index_data.size() >= std::numeric_limits<uint16_t>::max() - 2U)
        finish();
    auto n = lines_vertex_data.size();
    lines_vertex_data.push_back({start, color, atlas_white_pixel});
    lines_vertex_data.push_back({end, color, atlas_white_pixel});
    lines_index_data.insert(lines_index_data.end(), {
        uint16_t(n), uint16_t(n + 1),
    });
}

void RenderTarget::drawGLLine(glm::vec2 start, glm::vec2 end, glm::u8vec4 start_color, glm::u8vec4 end_color)
{
    if (lines_index_data.size() >= std::numeric_limits<uint16_t>::max() - 2U)
        finish();
    auto n = lines_vertex_data.size();
    lines_vertex_data.push_back({start, start_color, atlas_white_pixel});
    lines_vertex_data.push_back({end, end_color, atlas_white_pixel});
    lines_index_data.insert(lines_index_data.end(), {
        uint16_t(n), uint16_t(n + 1),
    });
}

void RenderTarget::drawGLLine(const std::initializer_list<glm::vec2>& points, glm::u8vec4 color)
{
    if (lines_index_data.size() >= std::numeric_limits<uint16_t>::max() - points.size())
        finish();
    auto n = lines_vertex_data.size();
    for(auto& p : points)
        lines_vertex_data.push_back({p, color, atlas_white_pixel});
    for(unsigned int idx=0; idx<points.size() - 1;idx++)
    {
        lines_index_data.insert(lines_index_data.end(), {
            uint16_t(n + idx), uint16_t(n + idx + 1),
        });
    }
}

void RenderTarget::drawGLLine(const std::vector<glm::vec2>& points, glm::u8vec4 color)
{
    if (points.size() < 1)
        return;
    if (lines_index_data.size() >= std::numeric_limits<uint16_t>::max() - points.size())
        finish();
    auto n = lines_vertex_data.size();
    for(auto& p : points)
        lines_vertex_data.push_back({p, color, atlas_white_pixel});
    for(unsigned int idx=0; idx<points.size() - 1;idx++)
    {
        lines_index_data.insert(lines_index_data.end(), {
            uint16_t(n + idx), uint16_t(n + idx + 1),
        });
    }
}

void RenderTarget::drawGLLineBlendAdd(const std::vector<glm::vec2>& points, glm::u8vec4 color)
{
    finish();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    auto n = lines_vertex_data.size();
    for(auto& p : points)
        lines_vertex_data.push_back({p, color, atlas_white_pixel});
    for(unsigned int idx=0; idx<points.size() - 1;idx++)
    {
        lines_index_data.insert(lines_index_data.end(), {
            uint16_t(n + idx), uint16_t(n + idx + 1),
        });
    }
    finish();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Generates a quad for a line segment.
// uv.x = AA flag (0 or 1), uv.y = signed distance from center (-1 to 1)
static void generateLineQuad(
    glm::vec2 start, glm::vec2 end, float half_width,
    glm::u8vec4 start_color, glm::u8vec4 end_color, bool antialiased,
    std::vector<RenderTarget::VertexData>& vertices,
    std::vector<uint16_t>& indices)
{
    glm::vec2 dir = end - start;
    float len = glm::length(dir);
    if (len < 0.0001f) return;

    // Normalize and calculate perpendicular offset
    dir /= len;
    glm::vec2 perp{-dir.y, dir.x};

    glm::vec2 offset = perp * half_width;
    float aa_flag = antialiased ? 1.0f : 0.0f;

    auto n = vertices.size();
    if (n >= std::numeric_limits<uint16_t>::max() - 4U) return;

    // Four corners of the quad:
    vertices.push_back({start - offset, start_color, {aa_flag, -1.0f}}); // bottom left
    vertices.push_back({start + offset, start_color, {aa_flag,  1.0f}}); // top left
    vertices.push_back({end - offset, end_color, {aa_flag, -1.0f}});     // bottom right
    vertices.push_back({end + offset, end_color, {aa_flag,  1.0f}});     // top right

    // Quad triangles
    indices.insert(indices.end(), {
        uint16_t(n + 0), uint16_t(n + 1), uint16_t(n + 2),
        uint16_t(n + 1), uint16_t(n + 3), uint16_t(n + 2),
    });
}

// Generate a bevel join triangle to connect line segments.
static void generateBevelJoin(glm::vec2 joint_point, float half_width, glm::vec2 prev_dir, glm::vec2 next_dir, glm::u8vec4 color, bool antialiased, std::vector<RenderTarget::VertexData>& vertices, std::vector<uint16_t>& indices)
{
    // Calculate perpendiculars for both segments.
    glm::vec2 prev_perp{-prev_dir.y, prev_dir.x};
    glm::vec2 next_perp{-next_dir.y, next_dir.x};

    // Determine which side needs the bevel (based on turn direction).
    float cross = prev_dir.x * next_dir.y - prev_dir.y * next_dir.x;

    // Skip if segments are nearly parallel.
    if (std::abs(cross) < 0.001f) return;

    // Pass anti-aliasing flag as coordinate value.
    float aa_flag = antialiased ? 1.0f : 0.0f;

    // Bail if there are more vertices than we can count.
    auto n = vertices.size();
    if (n >= std::numeric_limits<uint16_t>::max() - 3U) return;

    // Left turn: bevel on the right side (negative perpendicular)
    if (cross > 0)
    {
        vertices.push_back({joint_point, color, {aa_flag, 0.0f}});
        vertices.push_back({joint_point - prev_perp * half_width, color, {aa_flag, -1.0f}});
        vertices.push_back({joint_point - next_perp * half_width, color, {aa_flag, -1.0f}});
    }
    // Right turn: bevel on the left side (positive perpendicular)
    else
    {
        vertices.push_back({joint_point, color, {aa_flag, 0.0f}});
        vertices.push_back({joint_point + prev_perp * half_width, color, {aa_flag, 1.0f}});
        vertices.push_back({joint_point + next_perp * half_width, color, {aa_flag, 1.0f}});
    }

    indices.insert(indices.end(), {
        uint16_t(n), uint16_t(n + 1), uint16_t(n + 2),
    });
}

// Single-color line segment
void RenderTarget::drawLine(glm::vec2 start, glm::vec2 end, float width, glm::u8vec4 color, bool antialiased)
{
    drawLine(start, end, width, color, color, antialiased);
}

// Gradient line segment
void RenderTarget::drawLine(glm::vec2 start, glm::vec2 end, float width, glm::u8vec4 start_color, glm::u8vec4 end_color, bool antialiased)
{
    if (quad_lines_vertex_data.size() >= std::numeric_limits<uint16_t>::max() - 4U)
        finish();

    // Convert screen-space pixel width to virtual coordinate width.
    float virtual_width = width * virtual_size.x / static_cast<float>(physical_size.x);
    float half_width = virtual_width * 0.5f;

    // For AA, add 20% extra width to accommodate the soft edge.
    if (antialiased) half_width *= 1.2f;

    generateLineQuad(start, end, half_width, start_color, end_color, antialiased, quad_lines_vertex_data, quad_lines_index_data);
}

// Multi-segment line with bevel joins
void RenderTarget::drawLine(const std::vector<glm::vec2>& points, float width, glm::u8vec4 color, bool antialiased)
{
    if (points.size() < 2) return;

    // Convert screen-space pixel width to virtual coordinate width.
    const float virtual_width = width * virtual_size.x / static_cast<float>(physical_size.x);
    float half_width = virtual_width * 0.5f;
    if (antialiased) half_width *= 1.2f;

    // Draw each segment and add bevel joins between them.
    for (size_t i = 0; i < points.size() - 1; ++i)
    {
        if (quad_lines_vertex_data.size() >= std::numeric_limits<uint16_t>::max() - 7U)
            finish();

        glm::vec2 start = points[i];
        glm::vec2 end = points[i + 1];

        // Generate the line segment quad.
        generateLineQuad(start, end, half_width, color, color, antialiased, quad_lines_vertex_data, quad_lines_index_data);

        // Add bevel join if there's a next segment.
        if (i + 2 < points.size())
        {
            glm::vec2 next_end = points[i + 2];

            // Calculate direction vectors.
            glm::vec2 curr_dir = end - start;
            glm::vec2 next_dir = next_end - end;
            const float curr_len = glm::length(curr_dir);
            const float next_len = glm::length(next_dir);

            // Skip joining if current or next lines are of neglible size.
            if (curr_len > 0.0001f && next_len > 0.0001f)
            {
                curr_dir /= curr_len;
                next_dir /= next_len;
                generateBevelJoin(end, half_width, curr_dir, next_dir, color, antialiased, quad_lines_vertex_data, quad_lines_index_data);
            }
        }
    }
}

void RenderTarget::drawLineBlendAdd(glm::vec2 start, glm::vec2 end, float width, glm::u8vec4 color, bool antialiased)
{
    finish();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    drawLine(start, end, width, color, antialiased);
    finish();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RenderTarget::drawLineBlendAdd(const std::vector<glm::vec2>& points, float width, glm::u8vec4 color, bool antialiased)
{
    if (points.size() < 2) return;
    finish();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    drawLine(points, width, color, antialiased);
    finish();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RenderTarget::drawPoint(glm::vec2 position, glm::u8vec4 color)
{
    if (points_index_data.size() >= std::numeric_limits<uint16_t>::max() - 2U)
        finish();
    points_vertex_data.push_back({position, color, atlas_white_pixel});
    points_index_data.insert(points_index_data.end(), {uint16_t(points_vertex_data.size())});
}

void RenderTarget::drawRectColorMultiply(const sp::Rect& rect, glm::u8vec4 color)
{
    finish();
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    fillRect(rect, color);
    finish();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RenderTarget::drawCircleOutline(glm::vec2 center, float radius, float thickness, glm::u8vec4 color, size_t point_count, bool antialiased)
{
    // Stroke from radius inward (outer edge at radius, inner edge at radius - thickness)
    // This matches the previous triangle-strip implementation behavior
    float stroke_radius = radius - thickness * 0.5f;

    // Calculate screen-space radius for dynamic point count.
    // Use automatic point count if 0, otherwise use specified count.
    size_t actual_point_count = (point_count == 0)
        ? calculateCirclePointCount(radius * static_cast<float>(physical_size.x) / virtual_size.x)
        : point_count;

    shape_points_buffer.clear();
    shape_points_buffer.reserve(actual_point_count + 1);

    for (size_t idx = 0; idx <= actual_point_count; ++idx)
    {
        float f = static_cast<float>(idx) / static_cast<float>(actual_point_count) * static_cast<float>(M_PI) * 2.0f;
        shape_points_buffer.emplace_back(center + glm::vec2{std::sin(f) * stroke_radius, std::cos(f) * stroke_radius});
    }

    drawLine(shape_points_buffer, thickness, color, antialiased);
}

void RenderTarget::drawTiled(const sp::Rect& rect, std::string_view texture, glm::vec2 offset)
{
    auto info = getTextureInfo(texture);
    if (info.texture)
        finish();

    glm::vec2 increment = info.size;
    offset.x *= increment.x;
    offset.y *= increment.y;
    glm::ivec2 tile_count{int((rect.size.x + offset.x) / increment.x) + 1, int((rect.size.y + offset.y) / increment.y) + 1};
    for(int x=0; x<tile_count.x; x++)
    {
        for(int y=0; y<tile_count.y; y++)
        {
            if(vertex_data.size() >= std::numeric_limits<uint64_t>::max() - 4)
            {
                if (info.texture)
                    finish(info.texture);
                else
                    finish();
            }

            auto n = vertex_data.size();
            index_data.insert(index_data.end(), {
                uint16_t(n), uint16_t(n + 1), uint16_t(n + 2),
                uint16_t(n + 1), uint16_t(n + 3), uint16_t(n + 2),
            });
            glm::vec2 p0 = rect.position + glm::vec2(increment.x * x, increment.y * y) - offset;
            glm::vec2 p1 = rect.position + glm::vec2(increment.x * (x + 1), increment.y * (y + 1)) - offset;
            glm::vec2 uv0 = info.uv_rect.position;
            glm::vec2 uv1 = info.uv_rect.position + info.uv_rect.size;
            if (p0.x < 0) {
                uv0.x += info.uv_rect.size.x * -p0.x / increment.x;
                p0.x = 0;
            }
            if (p0.y < 0) {
                uv0.y += info.uv_rect.size.y * -p0.y / increment.y;
                p0.y = 0;
            }
            if (p1.x > rect.position.x + rect.size.x)
            {
                uv1.x -= info.uv_rect.size.x * (p1.x - (rect.position.x + rect.size.x)) / increment.x;
                p1.x = rect.position.x + rect.size.x;
            }
            if (p1.y > rect.position.y + rect.size.y)
            {
                uv1.y -= info.uv_rect.size.y * (p1.y - (rect.position.y + rect.size.y)) / increment.y;
                p1.y = rect.position.y + rect.size.y;
            }
            vertex_data.push_back({
                p0, {255, 255, 255, 255},
                {uv0.x, uv0.y}});
            vertex_data.push_back({
                {p0.x, p1.y}, {255, 255, 255, 255},
                {uv0.x, uv1.y}});
            vertex_data.push_back({
                {p1.x, p0.y}, {255, 255, 255, 255},
                {uv1.x, uv0.y}});
            vertex_data.push_back({
                p1, {255, 255, 255, 255},
                {uv1.x, uv1.y}});
        }
    }
    
    if (info.texture)
        finish(info.texture);
}

void RenderTarget::drawTriangleStrip(const std::initializer_list<glm::vec2>& points, glm::u8vec4 color)
{
    if (vertex_data.size() >= std::numeric_limits<uint16_t>::max() - points.size())
        finish();

    auto n = vertex_data.size();
    for(auto& p : points)
        vertex_data.push_back({p, color, atlas_white_pixel});
    for(unsigned int idx=0; idx<points.size() - 2;idx++)
    {
        index_data.insert(index_data.end(), {
            uint16_t(n + idx), uint16_t(n + idx + 1), uint16_t(n + idx + 2),
        });
    }
}

void RenderTarget::drawTriangleStrip(const std::vector<glm::vec2>& points, glm::u8vec4 color)
{
    if (vertex_data.size() >= std::numeric_limits<uint16_t>::max() - points.size())
        finish();

    auto n = vertex_data.size();
    for(auto& p : points)
        vertex_data.push_back({p, color, atlas_white_pixel});
    for(unsigned int idx=0; idx<points.size() - 2;idx++)
    {
        index_data.insert(index_data.end(), {
            uint16_t(n + idx), uint16_t(n + idx + 1), uint16_t(n + idx + 2),
        });
    }
}

void RenderTarget::drawTriangles(const std::vector<glm::vec2>& points, const std::vector<uint16_t>& indices, glm::u8vec4 color)
{
    if (vertex_data.size() >= std::numeric_limits<uint16_t>::max() - points.size())
        finish();

    auto n = vertex_data.size();
    for(auto& p : points)
        vertex_data.push_back({p, color, atlas_white_pixel});
    for(auto idx : indices)
        index_data.push_back(uint16_t(n + idx));
}

void RenderTarget::fillCircle(glm::vec2 center, float radius, glm::u8vec4 color, size_t point_count)
{
    // Calculate screen-space radius for dynamic point count.
    // Use automatic point count if 0, otherwise use specified count.
    // TODO: DRY with outline
    size_t actual_point_count = (point_count == 0)
        ? calculateCirclePointCount(radius * static_cast<float>(physical_size.x) / virtual_size.x)
        : point_count;

    if (vertex_data.size() >= std::numeric_limits<uint16_t>::max() - actual_point_count)
        finish();

    auto n = vertex_data.size();
    for (size_t idx = 0; idx < actual_point_count; idx++)
    {
        float f = static_cast<float>(idx) / static_cast<float>(actual_point_count) * static_cast<float>(M_PI) * 2.0f;
        vertex_data.push_back({center + glm::vec2{std::sin(f) * radius, std::cos(f) * radius}, color, atlas_white_pixel});
    }
    for (size_t idx = 2; idx < actual_point_count; idx++)
    {
        index_data.insert(index_data.end(), {
            uint16_t(n), uint16_t(n + idx - 1), uint16_t(n + idx),
        });
    }
}

void RenderTarget::drawRectOutline(const sp::Rect& rect, float width, glm::u8vec4 color, bool antialiased)
{
    // Buffer points
    shape_points_buffer.clear();
    shape_points_buffer.reserve(5);
    shape_points_buffer.emplace_back(rect.position);
    shape_points_buffer.emplace_back(glm::vec2(rect.position.x + rect.size.x, rect.position.y));
    shape_points_buffer.emplace_back(rect.position + rect.size);
    shape_points_buffer.emplace_back(glm::vec2(rect.position.x, rect.position.y + rect.size.y));
    shape_points_buffer.emplace_back(rect.position);

    drawLine(shape_points_buffer, width, color, antialiased);
}

void RenderTarget::fillRect(const sp::Rect& rect, glm::u8vec4 color)
{
    if (vertex_data.size() >= std::numeric_limits<uint16_t>::max() - 4U)
        finish();

    auto n = vertex_data.size();
    index_data.insert(index_data.end(), {
        uint16_t(n), uint16_t(n + 1), uint16_t(n + 2),
        uint16_t(n + 1), uint16_t(n + 3), uint16_t(n + 2),
    });
    vertex_data.push_back({{rect.position.x, rect.position.y}, color, atlas_white_pixel});
    vertex_data.push_back({{rect.position.x, rect.position.y + rect.size.y}, color, atlas_white_pixel});
    vertex_data.push_back({{rect.position.x + rect.size.x, rect.position.y}, color, atlas_white_pixel});
    vertex_data.push_back({{rect.position.x + rect.size.x, rect.position.y + rect.size.y}, color, atlas_white_pixel});
}

void RenderTarget::drawTexturedQuad(std::string_view texture,
    glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3,
    glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3,
    glm::u8vec4 color)
{
    auto info = getTextureInfo(texture);
    if (info.texture || vertex_data.size() >= std::numeric_limits<uint16_t>::max() - 4U)
        finish();
    auto& uv_rect = info.uv_rect;

    auto n = vertex_data.size();
    index_data.insert(index_data.end(), {
        uint16_t(n), uint16_t(n + 1), uint16_t(n + 2),
        uint16_t(n + 1), uint16_t(n + 3), uint16_t(n + 2),
    });

    vertex_data.push_back({p0, color,
        {uv_rect.position.x + uv_rect.size.x * uv0.x, uv_rect.position.y + uv_rect.size.y * uv0.y}});
    vertex_data.push_back({p1, color,
        {uv_rect.position.x + uv_rect.size.x * uv1.x, uv_rect.position.y + uv_rect.size.y * uv1.y}});
    vertex_data.push_back({p3, color,
        {uv_rect.position.x + uv_rect.size.x * uv3.x, uv_rect.position.y + uv_rect.size.y * uv3.y}});
    vertex_data.push_back({p2, color,
        {uv_rect.position.x + uv_rect.size.x * uv2.x, uv_rect.position.y + uv_rect.size.y * uv2.y}});

    if (info.texture) finish(info.texture);
}

void RenderTarget::drawText(sp::Rect rect, std::string_view text, Alignment align, float font_size, sp::Font* font, glm::u8vec4 color, int flags)
{
    if (!font) font = default_font;
    auto prepared = font->prepare(text, 32, font_size, color, rect.size, align, flags);
    drawText(rect, prepared, flags);
}

void RenderTarget::drawText(sp::Rect rect, const sp::Font::PreparedFontString& prepared, int flags)
{
    auto& ags = atlas_glyphs[prepared.getFont()];
    for(auto gd : prepared.data)
    {
        Font::GlyphInfo glyph;
        if (gd.char_code == 0 || !prepared.getFont()->getGlyphInfo(gd.char_code, 32, glyph))
        {
            glyph.advance = 0.0f;
            glyph.bounds.size.x = 0.0f;
        }

        if (glyph.bounds.size.x > 0.0f)
        {
            Rect uv_rect;
            auto it = ags.find(gd.char_code);
            if (it == ags.end())
            {
                uv_rect = atlas_texture->add(prepared.getFont()->drawGlyph(gd.char_code, 32), 1);
                ags[gd.char_code] = uv_rect;
                //LOG(Info, "Added glyph '", char(gd.char_code), "' to atlas@", uv_rect.position, " ", uv_rect.size, "  ", atlas_texture->usageRate() * 100.0f, "%");
            }
            else
            {
                uv_rect = it->second;
            }
            float size_scale = gd.size / 32.0f;

            float u0 = uv_rect.position.x;
            float v0 = uv_rect.position.y;
            float u1 = uv_rect.position.x + uv_rect.size.x;
            float v1 = uv_rect.position.y + uv_rect.size.y;

            float left = gd.position.x + glyph.bounds.position.x * size_scale;
            float right = left + glyph.bounds.size.x * size_scale;
            // Adjust font baseline if set.
            float top = gd.position.y - glyph.bounds.position.y * size_scale + (prepared.getFont()->getBaselineOffset() * gd.size / 32.0f);
            float bottom = top + glyph.bounds.size.y * size_scale;

            if (flags & Font::FlagClip)
            {
                if (right < 0)
                    continue;
                if (left < 0)
                {
                    u0 = u1 - uv_rect.size.x * (0 - right) / (left - right);
                    left = 0;
                }

                if (left > rect.size.x)
                    continue;
                if (right > rect.size.x)
                {
                    u1 = u0 + uv_rect.size.x * (rect.size.x - left) / (right - left);
                    right = rect.size.x;
                }

                if (bottom < 0)
                    continue;
                if (top < 0)
                {
                    v0 = v1 - uv_rect.size.y * (0 - bottom) / (top - bottom);
                    top = 0;
                }

                if (top > rect.size.y)
                    continue;
                if (bottom > rect.size.y)
                {
                    v1 = v0 + uv_rect.size.y * (rect.size.y - top) / (bottom - top);
                    bottom = rect.size.y;
                }
            }

            glm::vec2 p0{left, top};
            glm::vec2 p1{right, top};
            glm::vec2 p2{left, bottom};
            glm::vec2 p3{right, bottom};

            if (flags & Font::FlagVertical)
            {
                p0 = {p0.y, rect.size.y - p0.x};
                p1 = {p1.y, rect.size.y - p1.x};
                p2 = {p2.y, rect.size.y - p2.x};
                p3 = {p3.y, rect.size.y - p3.x};
            }

            p0 += rect.position;
            p1 += rect.position;
            p2 += rect.position;
            p3 += rect.position;

            if (vertex_data.size() >= std::numeric_limits<uint16_t>::max() - 4U)
                finish();

            auto n = vertex_data.size();
            index_data.insert(index_data.end(), {
                uint16_t(n + 0), uint16_t(n + 1), uint16_t(n + 2),
                uint16_t(n + 1), uint16_t(n + 3), uint16_t(n + 2),
            });
            vertex_data.push_back({
                p0, gd.color, {u0, v0}});
            vertex_data.push_back({
                p2, gd.color, {u0, v1}});
            vertex_data.push_back({
                p1, gd.color, {u1, v0}});
            vertex_data.push_back({
                p3, gd.color, {u1, v1}});
        }
    }
}

void RenderTarget::drawRotatedText(glm::vec2 center, float rotation, std::string_view text, float font_size, sp::Font* font, glm::u8vec4 color)
{
    if (!font)
        font = default_font;
    auto prepared = font->prepare(text, 32, font_size, color, {0.0f, 0.0f}, sp::Alignment::Center, 0);

    auto sin = std::sin(-glm::radians(rotation));
    auto cos = std::cos(-glm::radians(rotation));
    glm::mat2 mat{cos, -sin, sin, cos};

    auto& ags = atlas_glyphs[prepared.getFont()];
    float size_scale = font_size / 32.0f;
    for(auto gd : prepared.data)
    {
        Font::GlyphInfo glyph;
        if (gd.char_code == 0 || !prepared.getFont()->getGlyphInfo(gd.char_code, 32, glyph))
        {
            glyph.advance = 0.0f;
            glyph.bounds.size.x = 0.0f;
        }

        if (glyph.bounds.size.x > 0.0f)
        {
            Rect uv_rect;
            auto it = ags.find(gd.char_code);
            if (it == ags.end())
            {
                uv_rect = atlas_texture->add(prepared.getFont()->drawGlyph(gd.char_code, 32), 1);
                ags[gd.char_code] = uv_rect;
                LOG(Info, "Added glyph '", char(gd.char_code), "' to atlas@", uv_rect.position, " ", uv_rect.size, "  ", atlas_texture->usageRate() * 100.0f, "%");
            }
            else
            {
                uv_rect = it->second;
            }

            float u0 = uv_rect.position.x;
            float v0 = uv_rect.position.y;
            float u1 = uv_rect.position.x + uv_rect.size.x;
            float v1 = uv_rect.position.y + uv_rect.size.y;

            float left = gd.position.x + glyph.bounds.position.x * size_scale;
            float right = left + glyph.bounds.size.x * size_scale;
            // Adjust font baseline if set.
            float top = gd.position.y - glyph.bounds.position.y * size_scale + (prepared.getFont()->getBaselineOffset() * gd.size / 32.0f);
            float bottom = top + glyph.bounds.size.y * size_scale;

            glm::vec2 p0 = mat * glm::vec2{left, top} + center;
            glm::vec2 p1 = mat * glm::vec2{right, top} + center;
            glm::vec2 p2 = mat * glm::vec2{left, bottom} + center;
            glm::vec2 p3 = mat * glm::vec2{right, bottom} + center;

            if (vertex_data.size() >= std::numeric_limits<uint16_t>::max() - 4U)
                finish();

            auto n = vertex_data.size();
            index_data.insert(index_data.end(), {
                uint16_t(n + 0), uint16_t(n + 1), uint16_t(n + 2),
                uint16_t(n + 1), uint16_t(n + 3), uint16_t(n + 2),
            });
            vertex_data.push_back({
                p0, color, {u0, v0}});
            vertex_data.push_back({
                p2, color, {u0, v1}});
            vertex_data.push_back({
                p1, color, {u1, v0}});
            vertex_data.push_back({
                p3, color, {u1, v1}});
        }
    }
}

void RenderTarget::drawStretched(sp::Rect rect, std::string_view texture, glm::u8vec4 color)
{
    if (rect.size.x >= rect.size.y)
    {
        drawStretchedH(rect, texture, color);
    }else{
        drawStretchedV(rect, texture, color);
    }
}

void RenderTarget::drawStretchedH(sp::Rect rect, std::string_view texture, glm::u8vec4 color)
{
    auto info = getTextureInfo(texture);
    if (info.texture || vertex_data.size() >= std::numeric_limits<uint16_t>::max() - 8U)
        finish();
    auto& uv_rect = info.uv_rect;

    float w = rect.size.y / 2.0f;
    if (w * 2 > rect.size.x)
        w = rect.size.x / 2.0f;
    
    auto n = vertex_data.size();
    index_data.insert(index_data.end(), {
        uint16_t(n + 0), uint16_t(n + 1), uint16_t(n + 2),
        uint16_t(n + 1), uint16_t(n + 3), uint16_t(n + 2),
        uint16_t(n + 2), uint16_t(n + 3), uint16_t(n + 4),
        uint16_t(n + 3), uint16_t(n + 5), uint16_t(n + 4),
        uint16_t(n + 4), uint16_t(n + 5), uint16_t(n + 6),
        uint16_t(n + 5), uint16_t(n + 7), uint16_t(n + 6),
    });
    vertex_data.push_back({
        {rect.position.x, rect.position.y},
        color, {uv_rect.position.x, uv_rect.position.y}});
    vertex_data.push_back({
        {rect.position.x, rect.position.y + rect.size.y},
        color, {uv_rect.position.x, uv_rect.position.y + uv_rect.size.y}});
    vertex_data.push_back({
        {rect.position.x + w, rect.position.y},
        color, {uv_rect.position.x + uv_rect.size.x * 0.5f, uv_rect.position.y}});
    vertex_data.push_back({
        {rect.position.x + w, rect.position.y + rect.size.y},
        color, {uv_rect.position.x + uv_rect.size.x * 0.5f, uv_rect.position.y + uv_rect.size.y}});
    vertex_data.push_back({
        {rect.position.x + rect.size.x - w, rect.position.y},
        color, {uv_rect.position.x + uv_rect.size.x * 0.5f, uv_rect.position.y}});
    vertex_data.push_back({
        {rect.position.x + rect.size.x - w, rect.position.y + rect.size.y},
        color, {uv_rect.position.x + uv_rect.size.x * 0.5f, uv_rect.position.y + uv_rect.size.y}});
    vertex_data.push_back({
        {rect.position.x + rect.size.x, rect.position.y},
        color, {uv_rect.position.x + uv_rect.size.x, uv_rect.position.y}});
    vertex_data.push_back({
        {rect.position.x + rect.size.x, rect.position.y + rect.size.y},
        color, {uv_rect.position.x + uv_rect.size.x, uv_rect.position.y + uv_rect.size.y}});

    if (info.texture)
        finish(info.texture);
}

void RenderTarget::drawStretchedV(sp::Rect rect, std::string_view texture, glm::u8vec4 color)
{
    auto info = getTextureInfo(texture);
    if (info.texture || vertex_data.size() >= std::numeric_limits<uint16_t>::max() - 8U)
        finish();
    auto& uv_rect = info.uv_rect;

    float h = rect.size.x / 2.0f;
    if (h * 2 > rect.size.y)
        h = rect.size.y / 2.0f;
    
    auto n = vertex_data.size();
    index_data.insert(index_data.end(), {
        uint16_t(n + 0), uint16_t(n + 1), uint16_t(n + 2),
        uint16_t(n + 1), uint16_t(n + 3), uint16_t(n + 2),
        uint16_t(n + 2), uint16_t(n + 3), uint16_t(n + 4),
        uint16_t(n + 3), uint16_t(n + 5), uint16_t(n + 4),
        uint16_t(n + 4), uint16_t(n + 5), uint16_t(n + 6),
        uint16_t(n + 5), uint16_t(n + 7), uint16_t(n + 6),
    });
    vertex_data.push_back({
        {rect.position.x, rect.position.y},
        color, {uv_rect.position.x, uv_rect.position.y}});
    vertex_data.push_back({
        {rect.position.x + rect.size.x, rect.position.y},
        color, {uv_rect.position.x, uv_rect.position.y + uv_rect.size.y}});
    vertex_data.push_back({
        {rect.position.x, rect.position.y + h},
        color, {uv_rect.position.x + uv_rect.size.x * 0.5f, uv_rect.position.y}});
    vertex_data.push_back({
        {rect.position.x + rect.size.x, rect.position.y + h},
        color, {uv_rect.position.x + uv_rect.size.x * 0.5f, uv_rect.position.y + uv_rect.size.y}});
    vertex_data.push_back({
        {rect.position.x, rect.position.y + rect.size.y - h},
        color, {uv_rect.position.x + uv_rect.size.x * 0.5f, uv_rect.position.y}});
    vertex_data.push_back({
        {rect.position.x + rect.size.x, rect.position.y + rect.size.y - h},
        color, {uv_rect.position.x + uv_rect.size.x * 0.5f, uv_rect.position.y + uv_rect.size.y}});
    vertex_data.push_back({
        {rect.position.x, rect.position.y + rect.size.y},
        color, {uv_rect.position.x + uv_rect.size.x, uv_rect.position.y}});
    vertex_data.push_back({
        {rect.position.x + rect.size.x, rect.position.y + rect.size.y},
        color, {uv_rect.position.x + uv_rect.size.x, uv_rect.position.y + uv_rect.size.y}});

    if (info.texture)
        finish(info.texture);
}

void RenderTarget::drawStretchedHV(sp::Rect rect, float corner_size, std::string_view texture, glm::u8vec4 color)
{
    auto info = getTextureInfo(texture);
    if (info.texture || vertex_data.size() >= std::numeric_limits<uint16_t>::max() - 16U)
        finish();
    auto& uv_rect = info.uv_rect;

    corner_size = std::min(corner_size, rect.size.y / 2.0f);
    corner_size = std::min(corner_size, rect.size.x / 2.0f);

    auto n = vertex_data.size();
    index_data.insert(index_data.end(), {
        uint16_t(n + 0), uint16_t(n + 4), uint16_t(n + 1),
        uint16_t(n + 1), uint16_t(n + 4), uint16_t(n + 5),
        uint16_t(n + 1), uint16_t(n + 5), uint16_t(n + 2),
        uint16_t(n + 2), uint16_t(n + 5), uint16_t(n + 6),
        uint16_t(n + 2), uint16_t(n + 6), uint16_t(n + 3),
        uint16_t(n + 3), uint16_t(n + 6), uint16_t(n + 7),

        uint16_t(n + 4), uint16_t(n + 8), uint16_t(n + 5),
        uint16_t(n + 5), uint16_t(n + 8), uint16_t(n + 9),
        uint16_t(n + 5), uint16_t(n + 9), uint16_t(n + 6),
        uint16_t(n + 6), uint16_t(n + 9), uint16_t(n + 10),
        uint16_t(n + 6), uint16_t(n + 10), uint16_t(n + 7),
        uint16_t(n + 7), uint16_t(n + 10), uint16_t(n + 11),

        uint16_t(n + 8), uint16_t(n + 12), uint16_t(n + 9),
        uint16_t(n + 9), uint16_t(n + 12), uint16_t(n + 13),
        uint16_t(n + 9), uint16_t(n + 13), uint16_t(n + 10),
        uint16_t(n + 10), uint16_t(n + 13), uint16_t(n + 14),
        uint16_t(n + 10), uint16_t(n + 14), uint16_t(n + 11),
        uint16_t(n + 11), uint16_t(n + 14), uint16_t(n + 15),
    });
    vertex_data.push_back({
        {rect.position.x, rect.position.y},
        color, {uv_rect.position.x, uv_rect.position.y}});
    vertex_data.push_back({
        {rect.position.x + corner_size, rect.position.y},
        color, {uv_rect.position.x + uv_rect.size.x * 0.5f, uv_rect.position.y}});
    vertex_data.push_back({
        {rect.position.x + rect.size.x - corner_size, rect.position.y},
        color, {uv_rect.position.x + uv_rect.size.x * 0.5f, uv_rect.position.y}});
    vertex_data.push_back({
        {rect.position.x + rect.size.x, rect.position.y},
        color, {uv_rect.position.x + uv_rect.size.x, uv_rect.position.y}});

    vertex_data.push_back({
        {rect.position.x, rect.position.y + corner_size},
        color, {uv_rect.position.x, uv_rect.position.y + uv_rect.size.y * 0.5f}});
    vertex_data.push_back({
        {rect.position.x + corner_size, rect.position.y + corner_size},
        color, {uv_rect.position.x + uv_rect.size.x * 0.5f, uv_rect.position.y + uv_rect.size.y * 0.5f}});
    vertex_data.push_back({
        {rect.position.x + rect.size.x - corner_size, rect.position.y + corner_size},
        color, {uv_rect.position.x + uv_rect.size.x * 0.5f, uv_rect.position.y + uv_rect.size.y * 0.5f}});
    vertex_data.push_back({
        {rect.position.x + rect.size.x, rect.position.y + corner_size},
        color, {uv_rect.position.x + uv_rect.size.x, uv_rect.position.y + uv_rect.size.y * 0.5f}});

    vertex_data.push_back({
        {rect.position.x, rect.position.y + rect.size.y - corner_size},
        color, {uv_rect.position.x, uv_rect.position.y + uv_rect.size.y * 0.5f}});
    vertex_data.push_back({
        {rect.position.x + corner_size, rect.position.y + rect.size.y - corner_size},
        color, {uv_rect.position.x + uv_rect.size.x * 0.5f, uv_rect.position.y + uv_rect.size.y * 0.5f}});
    vertex_data.push_back({
        {rect.position.x + rect.size.x - corner_size, rect.position.y + rect.size.y - corner_size},
        color, {uv_rect.position.x + uv_rect.size.x * 0.5f, uv_rect.position.y + uv_rect.size.y * 0.5f}});
    vertex_data.push_back({
        {rect.position.x + rect.size.x, rect.position.y + rect.size.y - corner_size},
        color, {uv_rect.position.x + uv_rect.size.x, uv_rect.position.y + uv_rect.size.y * 0.5f}});

    vertex_data.push_back({
        {rect.position.x, rect.position.y + rect.size.y},
        color, {uv_rect.position.x, uv_rect.position.y + uv_rect.size.y}});
    vertex_data.push_back({
        {rect.position.x + corner_size, rect.position.y + rect.size.y},
        color, {uv_rect.position.x + uv_rect.size.x * 0.5f, uv_rect.position.y + uv_rect.size.y}});
    vertex_data.push_back({
        {rect.position.x + rect.size.x - corner_size, rect.position.y + rect.size.y},
        color, {uv_rect.position.x + uv_rect.size.x * 0.5f, uv_rect.position.y + uv_rect.size.y}});
    vertex_data.push_back({
        {rect.position.x + rect.size.x, rect.position.y + rect.size.y},
        color, {uv_rect.position.x + uv_rect.size.x, uv_rect.position.y + uv_rect.size.y}});

    if (info.texture)
        finish(info.texture);
}

void RenderTarget::drawStretchedHVClipped(sp::Rect rect, sp::Rect clip_rect, float corner_size, std::string_view texture, glm::u8vec4 color)
{
    if (clip_rect.size.x < 0 || clip_rect.size.y < 0)
        return;

    auto info = getTextureInfo(texture);
    if (info.texture || vertex_data.size() >= std::numeric_limits<uint16_t>::max() - 16U)
        finish();
    const auto& uv_rect = info.uv_rect;

    corner_size = std::min(corner_size, rect.size.y / 2.0f);
    corner_size = std::min(corner_size, rect.size.x / 2.0f);

    auto n = vertex_data.size();
    index_data.insert(index_data.end(), {
        uint16_t(n + 0), uint16_t(n + 4), uint16_t(n + 1),
        uint16_t(n + 1), uint16_t(n + 4), uint16_t(n + 5),
        uint16_t(n + 1), uint16_t(n + 5), uint16_t(n + 2),
        uint16_t(n + 2), uint16_t(n + 5), uint16_t(n + 6),
        uint16_t(n + 2), uint16_t(n + 6), uint16_t(n + 3),
        uint16_t(n + 3), uint16_t(n + 6), uint16_t(n + 7),

        uint16_t(n + 4), uint16_t(n + 8), uint16_t(n + 5),
        uint16_t(n + 5), uint16_t(n + 8), uint16_t(n + 9),
        uint16_t(n + 5), uint16_t(n + 9), uint16_t(n + 6),
        uint16_t(n + 6), uint16_t(n + 9), uint16_t(n + 10),
        uint16_t(n + 6), uint16_t(n + 10), uint16_t(n + 7),
        uint16_t(n + 7), uint16_t(n + 10), uint16_t(n + 11),

        uint16_t(n + 8), uint16_t(n + 12), uint16_t(n + 9),
        uint16_t(n + 9), uint16_t(n + 12), uint16_t(n + 13),
        uint16_t(n + 9), uint16_t(n + 13), uint16_t(n + 10),
        uint16_t(n + 10), uint16_t(n + 13), uint16_t(n + 14),
        uint16_t(n + 10), uint16_t(n + 14), uint16_t(n + 11),
        uint16_t(n + 11), uint16_t(n + 14), uint16_t(n + 15),
    });
    float x0 = rect.position.x;
    float x1 = rect.position.x + corner_size;
    float x2 = rect.position.x + rect.size.x - corner_size;
    float x3 = rect.position.x + rect.size.x;
    float y0 = rect.position.y;
    float y1 = rect.position.y + corner_size;
    float y2 = rect.position.y + rect.size.y - corner_size;
    float y3 = rect.position.y + rect.size.y;
    float uvx0 = uv_rect.position.x;
    float uvx1 = uv_rect.position.x + uv_rect.size.x * 0.5f;
    float uvx2 = uv_rect.position.x + uv_rect.size.x;
    float uvy0 = uv_rect.position.y;
    float uvy1 = uv_rect.position.y + uv_rect.size.y * 0.5f;
    float uvy2 = uv_rect.position.y + uv_rect.size.y;

    if (x3 < clip_rect.position.x) {
        return;
    } else if (x2 < clip_rect.position.x) {
        uvx0 = uvx1 = uvx1 + (uvx2 - uvx1) * ((clip_rect.position.x - x2) / (x3 - x2));
        x0 = x1 = x2 = clip_rect.position.x;
    } else if (x1 < clip_rect.position.x) {
        uvx0 = uvx1;
        x0 = x1 = clip_rect.position.x;
    } else if (x0 < clip_rect.position.x) {
        uvx0 = uvx0 + (uvx1 - uvx0) * ((clip_rect.position.x - x0) / (x1 - x0));
        x0 = clip_rect.position.x;
    }
    if (x0 > clip_rect.position.x + clip_rect.size.x) {
        return;
    } else if (x1 > clip_rect.position.x + clip_rect.size.x) {
        uvx1 = uvx2 = uvx0 + (uvx1 - uvx0) * ((clip_rect.position.x + clip_rect.size.x - x0) / (x1 - x0));
        x1 = x2 = x3 = clip_rect.position.x + clip_rect.size.x;
    } else if (x2 > clip_rect.position.x + clip_rect.size.x) {
        uvx2 = uvx1;
        x2 = x3 = clip_rect.position.x + clip_rect.size.x;
    } else if (x3 > clip_rect.position.x + clip_rect.size.x) {
        uvx2 = uvx1 + (uvx2 - uvx1) * ((clip_rect.position.x + clip_rect.size.x - x2) / (x3 - x2));
        x3 = clip_rect.position.x + clip_rect.size.x;
    }
    if (y3 < clip_rect.position.y) {
        return;
    } else if (y2 < clip_rect.position.y) {
        uvy0 = uvy1 = uvy1 + (uvy2 - uvy1) * ((clip_rect.position.y - y2) / (y3 - y2));
        y0 = y1 = y2 = clip_rect.position.y;
    } else if (y1 < clip_rect.position.y) {
        uvy0 = uvy1;
        y0 = y1 = clip_rect.position.y;
    } else if (y0 < clip_rect.position.y) {
        uvy0 = uvy0 + (uvy1 - uvy0) * ((clip_rect.position.y - y0) / (y1 - y0));
        y0 = clip_rect.position.y;
    }
    if (y0 > clip_rect.position.y + clip_rect.size.y) {
        return;
    } else if (y1 > clip_rect.position.y + clip_rect.size.y) {
        uvy1 = uvy2 = uvy0 + (uvy1 - uvy0) * ((clip_rect.position.y + clip_rect.size.y - y0) / (y1 - y0));
        y1 = y2 = y3 = clip_rect.position.y + clip_rect.size.y;
    } else if (y2 > clip_rect.position.y + clip_rect.size.y) {
        uvy2 = uvy1;
        y2 = y3 = clip_rect.position.y + clip_rect.size.y;
    } else if (y3 > clip_rect.position.y + clip_rect.size.y) {
        uvy2 = uvy1 + (uvy2 - uvy1) * ((clip_rect.position.y + clip_rect.size.y - y2) / (y3 - y2));
        y3 = clip_rect.position.y + clip_rect.size.y;
    }

    vertex_data.push_back({ {x0, y0}, color, {uvx0, uvy0}});
    vertex_data.push_back({ {x1, y0}, color, {uvx1, uvy0}});
    vertex_data.push_back({ {x2, y0}, color, {uvx1, uvy0}});
    vertex_data.push_back({ {x3, y0}, color, {uvx2, uvy0}});

    vertex_data.push_back({ {x0, y1}, color, {uvx0, uvy1}});
    vertex_data.push_back({ {x1, y1}, color, {uvx1, uvy1}});
    vertex_data.push_back({ {x2, y1}, color, {uvx1, uvy1}});
    vertex_data.push_back({ {x3, y1}, color, {uvx2, uvy1}});

    vertex_data.push_back({ {x0, y2}, color, {uvx0, uvy1}});
    vertex_data.push_back({ {x1, y2}, color, {uvx1, uvy1}});
    vertex_data.push_back({ {x2, y2}, color, {uvx1, uvy1}});
    vertex_data.push_back({ {x3, y2}, color, {uvx2, uvy1}});

    vertex_data.push_back({ {x0, y3}, color, {uvx0, uvy2}});
    vertex_data.push_back({ {x1, y3}, color, {uvx1, uvy2}});
    vertex_data.push_back({ {x2, y3}, color, {uvx1, uvy2}});
    vertex_data.push_back({ {x3, y3}, color, {uvx2, uvy2}});

    if (info.texture)
        finish(info.texture);
}

void RenderTarget::finish()
{
    finish(atlas_texture);
}

void RenderTarget::applyBuffer(sp::Texture* texture, std::vector<VertexData> &data, std::vector<uint16_t> &index, int mode)
{
    if (data.size())
    {
        shader->bind();

        glUniform1i(shader->getUniformLocation("u_texture"), 0);
        glActiveTexture(GL_TEXTURE0);
        texture->bind();

        glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData) * data.size(), data.data(), GL_DYNAMIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * index.size(), index.data(), GL_DYNAMIC_DRAW);

        glVertexAttribPointer(shader->getAttributeLocation("a_position"), 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(VertexData)), (void*)0);
        glEnableVertexAttribArray(shader->getAttributeLocation("a_position"));
        glVertexAttribPointer(shader->getAttributeLocation("a_color"), 4, GL_UNSIGNED_BYTE, GL_TRUE, static_cast<GLsizei>(sizeof(VertexData)), (void*)offsetof(VertexData, color));
        glEnableVertexAttribArray(shader->getAttributeLocation("a_color"));
        glVertexAttribPointer(shader->getAttributeLocation("a_texcoords"), 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(VertexData)), (void*)offsetof(VertexData, uv));
        glEnableVertexAttribArray(shader->getAttributeLocation("a_texcoords"));

        glDrawElements(mode, static_cast<GLsizei>(index.size()), GL_UNSIGNED_SHORT, nullptr);

        data.clear();
        index.clear();
    }
}

void RenderTarget::finish(sp::Texture* texture)
{
    applyBuffer(texture, vertex_data, index_data, GL_TRIANGLES);
    applyBuffer(texture, lines_vertex_data, lines_index_data, GL_LINES);
    applyBuffer(texture, points_vertex_data, points_index_data, GL_POINTS);

    // Draw quad-based lines with the AA shader
    if (quad_lines_vertex_data.size())
    {
        wide_line_shader->bind();

        glm::mat3 project_matrix{1.0f};
        project_matrix[0][0] = 2.0f / static_cast<float>(virtual_size.x);
        project_matrix[1][1] = -2.0f / static_cast<float>(virtual_size.y);
        project_matrix[2][0] = -1.0f;
        project_matrix[2][1] = 1.0f;
        glUniformMatrix3fv(wide_line_shader->getUniformLocation("u_projection"), 1, GL_FALSE, glm::value_ptr(project_matrix));

        glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData) * quad_lines_vertex_data.size(), quad_lines_vertex_data.data(), GL_DYNAMIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * quad_lines_index_data.size(), quad_lines_index_data.data(), GL_DYNAMIC_DRAW);

        glVertexAttribPointer(wide_line_shader->getAttributeLocation("a_position"), 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(VertexData)), (void*)0);
        glEnableVertexAttribArray(wide_line_shader->getAttributeLocation("a_position"));
        glVertexAttribPointer(wide_line_shader->getAttributeLocation("a_color"), 4, GL_UNSIGNED_BYTE, GL_TRUE, static_cast<GLsizei>(sizeof(VertexData)), (void*)offsetof(VertexData, color));
        glEnableVertexAttribArray(wide_line_shader->getAttributeLocation("a_color"));
        glVertexAttribPointer(wide_line_shader->getAttributeLocation("a_texcoords"), 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(VertexData)), (void*)offsetof(VertexData, uv));
        glEnableVertexAttribArray(wide_line_shader->getAttributeLocation("a_texcoords"));

        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(quad_lines_index_data.size()), GL_UNSIGNED_SHORT, nullptr);

        quad_lines_vertex_data.clear();
        quad_lines_index_data.clear();

        // Re-bind the standard shader for subsequent operations
        shader->bind();
        glUniformMatrix3fv(shader->getUniformLocation("u_projection"), 1, GL_FALSE, glm::value_ptr(project_matrix));
    }

    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_NONE);
}

glm::vec2 RenderTarget::getVirtualSize()
{
    return virtual_size;
}

glm::ivec2 RenderTarget::getPhysicalSize()
{
    return physical_size;
}

glm::ivec2 RenderTarget::virtualToPixelPosition(glm::vec2 v)
{
    return {v.x * physical_size.x / virtual_size.x, v.y * physical_size.y / virtual_size.y};
}

}
