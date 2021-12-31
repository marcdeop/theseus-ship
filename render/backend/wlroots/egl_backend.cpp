/*
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>
    SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "egl_backend.h"

#include "egl_helpers.h"
#include "egl_output.h"
#include "egl_texture.h"
#include "output.h"
#include "platform.h"
#include "surface.h"
#include "wlr_helpers.h"

#include "base/backend/wlroots/output.h"
#include "render/gl/egl.h"
#include "render/gl/gl.h"
#include "render/wayland/egl.h"

#include "screens.h"

#include <QOpenGLContext>
#include <kwinglplatform.h>
#include <stdexcept>
#include <wayland_logging.h>

namespace KWin::render::backend::wlroots
{

using eglFuncPtr = void (*)();
static eglFuncPtr get_proc_address(char const* name)
{
    return eglGetProcAddress(name);
}

std::unique_ptr<egl_output>& egl_backend::get_egl_out(base::output const* out)
{
    return static_cast<output*>(static_cast<base::wayland::output const*>(out)->render.get())->egl;
}

egl_backend::egl_backend(wlroots::platform& platform, bool headless)
    : gl::backend()
    , platform{platform}
    , headless{headless}
{
    platform.egl_data = &data.base;

    // Egl is always direct rendering.
    setIsDirectRendering(true);
    start();
}

egl_backend::~egl_backend()
{
    tear_down();
}

bool egl_backend::hasClientExtension(const QByteArray& ext) const
{
    return data.base.client_extensions.contains(ext);
}

void egl_backend::start()
{
    gl::init_client_extensions(*this);

    if (!init_platform()) {
        throw std::runtime_error("Could not initialize EGL backend");
    }
    if (!gl::init_egl_api(*this)) {
        throw std::runtime_error("Could not initialize EGL API");
    }
    if (!init_buffer_configs(this)) {
        throw std::runtime_error("Could not initialize buffer configs");
    }
    if (!init_rendering_context()) {
        throw std::runtime_error("Could not initialize rendering context");
    }

    gl::init_gl(EglPlatformInterface, get_proc_address);
    gl::init_buffer_age(*this);
    wayland::init_egl(*this, data);
}

bool egl_backend::init_platform()
{
    if (headless) {
        auto egl_display = get_egl_headless(*this);
        if (egl_display == EGL_NO_DISPLAY) {
            return false;
        }
        data.base.display = egl_display;
        platform.egl_display_to_terminate = egl_display;
        return true;
    }

    auto gbm = get_egl_gbm(platform, *this);
    if (!gbm) {
        return false;
    }

    assert(gbm->egl_display != EGL_NO_DISPLAY);
    data.base.display = gbm->egl_display;
    platform.egl_display_to_terminate = gbm->egl_display;

    this->gbm = std::move(gbm);
    return true;
}

bool egl_backend::init_rendering_context()
{
    data.base.context = gl::create_egl_context(*this);

    if (data.base.context == EGL_NO_CONTEXT) {
        return false;
    }

    for (auto& out : platform.base.all_outputs) {
        auto render = static_cast<output*>(static_cast<base::wayland::output*>(out)->render.get());
        get_egl_out(out) = std::make_unique<egl_output>(*render, this);
    }

    // AbstractEglBackend expects a surface to be set but this is not relevant as we render per
    // output and make the context current here on that output's surface. For simplicity we just
    // create a dummy surface and keep that constantly set over the run time.
    dummy_surface = headless ? create_headless_surface(*this, QSize(800, 600))
                             : create_surface(*this, QSize(800, 600));
    data.base.surface = dummy_surface->egl;

    if (platform.base.all_outputs.empty()) {
        // In case no outputs are connected make the context current with our dummy surface.
        return make_current(dummy_surface->egl, *this);
    }

    return get_egl_out(platform.base.all_outputs.front())->make_current();
}

void egl_backend::tear_down()
{
    if (!platform.egl_data) {
        // Already cleaned up.
        return;
    }

    cleanupSurfaces();
    dummy_surface.reset();

    cleanup();
    gbm.reset();

    platform.egl_data = nullptr;
    data = {};
}

void egl_backend::cleanup()
{
    cleanupGL();
    doneCurrent();
    eglDestroyContext(data.base.display, data.base.context);
    cleanupSurfaces();
    eglReleaseThread();

    delete dmabuf;
    dmabuf = nullptr;
}

void egl_backend::cleanupSurfaces()
{
    for (auto out : platform.base.all_outputs) {
        get_egl_out(out).reset();
    }
}

void egl_backend::present()
{
    // Not in use. This backend does per-screen rendering.
    Q_UNREACHABLE();
}

bool egl_backend::makeCurrent()
{
    if (auto context = QOpenGLContext::currentContext()) {
        // Workaround to tell Qt that no QOpenGLContext is current
        context->doneCurrent();
    }
    auto const current = eglMakeCurrent(
        data.base.display, data.base.surface, data.base.surface, data.base.context);
    return current;
}

void egl_backend::doneCurrent()
{
    eglMakeCurrent(data.base.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void egl_backend::screenGeometryChanged(QSize const& size)
{
    Q_UNUSED(size)
    // TODO, create new buffer?
}

gl::texture_private* egl_backend::createBackendTexture(gl::texture* texture)
{
    return new egl_texture(texture, this);
}

QRegion egl_backend::prepareRenderingFrame()
{
    startRenderTimer();
    return QRegion();
}

QRegion egl_backend::prepareRenderingForScreen(base::output* output)
{
    auto const& out = get_egl_out(output);

    out->make_current();
    prepareRenderFramebuffer(*out);
    setViewport(*out);

    if (!supportsBufferAge()) {
        // If buffer age exenstion is not supported we always repaint the whole output as we don't
        // know the status of the back buffer we render to.
        return output->geometry();
    }
    if (out->render.framebuffer) {
        // If we render to the extra frame buffer, do not use buffer age. It leads to artifacts.
        // TODO(romangg): Can we make use of buffer age even in this case somehow?
        return output->geometry();
    }
    if (out->bufferAge == 0) {
        // If buffer age is 0, the contents of the back buffer we now will render to are undefined
        // and it has to be repainted completely.
        return output->geometry();
    }
    if (out->bufferAge > static_cast<int>(out->damageHistory.size())) {
        // If buffer age is older than our damage history has recorded we do not have all damage
        // logged for that age and we need to repaint completely.
        return output->geometry();
    }

    // But if all conditions are satisfied we can look up our damage history up until to the buffer
    // age and repaint only that.
    QRegion region;
    for (int i = 0; i < out->bufferAge - 1; i++) {
        region |= out->damageHistory[i];
    }
    return region;
}

void egl_backend::endRenderingFrame(QRegion const& renderedRegion, QRegion const& damagedRegion)
{
    Q_UNUSED(renderedRegion)
    Q_UNUSED(damagedRegion)
}

void egl_backend::endRenderingFrameForScreen(base::output* output,
                                             QRegion const& renderedRegion,
                                             QRegion const& damagedRegion)
{
    auto& out = get_egl_out(output);
    renderFramebufferToSurface(*out);

    if (GLPlatform::instance()->supports(GLFeature::TimerQuery)) {
        out->out->last_timer_queries.emplace_back();
    }

    if (damagedRegion.intersected(output->geometry()).isEmpty()) {
        // If the damaged region of a window is fully occluded, the only
        // rendering done, if any, will have been to repair a reused back
        // buffer, making it identical to the front buffer.
        //
        // In this case we won't post the back buffer. Instead we'll just
        // set the buffer age to 1, so the repaired regions won't be
        // rendered again in the next frame.
        if (!renderedRegion.intersected(output->geometry()).isEmpty()) {
            glFlush();
        }

        out->bufferAge = 1;
        return;
    }

    eglSwapBuffers(data.base.display, out->surf->egl);
    auto buffer = out->create_buffer();

    if (!out->present(buffer)) {
        out->bufferAge = 0;
        out->out->swap_pending = false;
        return;
    }

    if (supportsBufferAge()) {
        eglQuerySurface(data.base.display, out->surf->egl, EGL_BUFFER_AGE_EXT, &out->bufferAge);

        if (out->damageHistory.size() > 10) {
            out->damageHistory.pop_back();
        }
        out->damageHistory.push_front(damagedRegion.intersected(output->geometry()));
    }
}

void egl_backend::prepareRenderFramebuffer(egl_output const& egl_out) const
{
    // When render.framebuffer is 0 we may just reset to the screen framebuffer.
    glBindFramebuffer(GL_FRAMEBUFFER, egl_out.render.framebuffer);
    GLRenderTarget::setKWinFramebuffer(egl_out.render.framebuffer);
}

void egl_backend::setViewport(egl_output const& egl_out) const
{
    auto const& overall = platform.base.screens.size();
    auto const& geo = egl_out.out->base.geometry();
    auto const& view = egl_out.out->base.view_geometry();

    auto const width_ratio = view.width() / (double)geo.width();
    auto const height_ratio = view.height() / (double)geo.height();

    glViewport(-geo.x() * width_ratio,
               (geo.height() - overall.height() + geo.y()) * height_ratio,
               overall.width() * width_ratio,
               overall.height() * height_ratio);
}

const float vertices[] = {
    -1.0f,
    1.0f,
    -1.0f,
    -1.0f,
    1.0f,
    -1.0f,

    -1.0f,
    1.0f,
    1.0f,
    -1.0f,
    1.0f,
    1.0f,
};

const float texCoords[] = {
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,

    0.0f,
    1.0f,
    1.0f,
    0.0f,
    1.0f,
    1.0f,
};

void egl_backend::initRenderTarget(egl_output& egl_out)
{
    if (egl_out.render.vbo) {
        // Already initialized.
        return;
    }
    std::shared_ptr<GLVertexBuffer> vbo(new GLVertexBuffer(KWin::GLVertexBuffer::Static));
    vbo->setData(6, 2, vertices, texCoords);
    egl_out.render.vbo = vbo;
}

void egl_backend::renderFramebufferToSurface(egl_output& egl_out)
{
    if (!egl_out.render.framebuffer) {
        // No additional render target.
        return;
    }
    initRenderTarget(egl_out);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLRenderTarget::setKWinFramebuffer(0);

    GLuint clearColor[4] = {0, 0, 0, 0};
    glClearBufferuiv(GL_COLOR, 0, clearColor);

    auto geo = egl_out.out->base.view_geometry();
    if (has_portrait_transform(egl_out.out->base)) {
        geo = geo.transposed();
        geo.moveTopLeft(geo.topLeft().transposed());
    }
    glViewport(geo.x(), geo.y(), geo.width(), geo.height());

    auto shader = ShaderManager::instance()->pushShader(ShaderTrait::MapTexture);

    QMatrix4x4 rotationMatrix;
    rotationMatrix.rotate(
        rotation_in_degree(static_cast<base::backend::wlroots::output&>(egl_out.out->base)),
        0,
        0,
        1);
    shader->setUniform(GLShader::ModelViewProjectionMatrix, rotationMatrix);

    glBindTexture(GL_TEXTURE_2D, egl_out.render.texture);
    egl_out.render.vbo->render(GL_TRIANGLES);
    ShaderManager::instance()->popShader();
}

}
