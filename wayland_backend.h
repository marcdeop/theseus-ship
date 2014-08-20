/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2013 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#ifndef KWIN_WAYLAND_BACKEND_H
#define KWIN_WAYLAND_BACKEND_H
// KWin
#include <kwinglobals.h>
// Qt
#include <QDir>
#include <QHash>
#include <QImage>
#include <QObject>
#include <QPoint>
#include <QSize>
// wayland
#include <wayland-client.h>

class QTemporaryFile;
class QImage;
struct wl_cursor_theme;
struct wl_buffer;
struct wl_shm;
struct wl_event_queue;

namespace KWin
{

namespace Wayland
{
class ShmPool;
class WaylandBackend;
class WaylandSeat;
class ConnectionThread;
class FullscreenShell;
class Output;
class Registry;
class Shell;
class ShellSurface;

class CursorData
{
public:
    CursorData();
    ~CursorData();
    bool isValid() const;
    const QPoint &hotSpot() const;
    const QImage &cursor() const;
private:
    bool init();
    QImage m_cursor;
    QPoint m_hotSpot;
    bool m_valid;
};

class X11CursorTracker : public QObject
{
    Q_OBJECT
public:
    explicit X11CursorTracker(WaylandSeat *seat, WaylandBackend *backend, QObject* parent = 0);
    virtual ~X11CursorTracker();
    void resetCursor();
private Q_SLOTS:
    void cursorChanged(uint32_t serial);
private:
    void installCursor(const CursorData &cursor);
    WaylandSeat *m_seat;
    QHash<uint32_t, CursorData> m_cursors;
    WaylandBackend *m_backend;
    uint32_t m_installedCursor;
    uint32_t m_lastX11Cursor;
};

class Buffer
{
public:
    Buffer(wl_buffer *buffer, const QSize &size, int32_t stride, size_t offset);
    ~Buffer();
    void copy(const void *src);
    void setReleased(bool released);
    void setUsed(bool used);

    wl_buffer *buffer() const;
    const QSize &size() const;
    int32_t stride() const;
    bool isReleased() const;
    bool isUsed() const;
    uchar *address();
private:
    wl_buffer *m_nativeBuffer;
    bool m_released;
    QSize m_size;
    int32_t m_stride;
    size_t m_offset;
    bool m_used;
};

class ShmPool : public QObject
{
    Q_OBJECT
public:
    ShmPool(wl_shm *shm);
    ~ShmPool();
    bool isValid() const;
    wl_buffer *createBuffer(const QImage &image);
    wl_buffer *createBuffer(const QSize &size, int32_t stride, const void *src);
    void *poolAddress() const;
    Buffer *getBuffer(const QSize &size, int32_t stride);
    wl_shm *shm();
Q_SIGNALS:
    void poolResized();
private:
    bool createPool();
    bool resizePool(int32_t newSize);
    wl_shm *m_shm;
    wl_shm_pool *m_pool;
    void *m_poolData;
    int32_t m_size;
    QScopedPointer<QTemporaryFile> m_tmpFile;
    bool m_valid;
    int m_offset;
    QList<Buffer*> m_buffers;
};

class WaylandSeat : public QObject
{
    Q_OBJECT
public:
    WaylandSeat(wl_seat *seat, WaylandBackend *backend);
    virtual ~WaylandSeat();

    void changed(uint32_t capabilities);
    wl_seat *seat();
    void pointerEntered(uint32_t serial);
    void resetCursor();
    void installCursorImage(wl_buffer *image, const QSize &size, const QPoint &hotspot);
    void installCursorImage(Qt::CursorShape shape);
private Q_SLOTS:
    void loadTheme();
private:
    void destroyPointer();
    void destroyKeyboard();
    void destroyTheme();
    wl_seat *m_seat;
    wl_pointer *m_pointer;
    wl_keyboard *m_keyboard;
    wl_surface *m_cursor;
    wl_cursor_theme *m_theme;
    uint32_t m_enteredSerial;
    QScopedPointer<X11CursorTracker> m_cursorTracker;
    WaylandBackend *m_backend;
};

/**
* @brief Class encapsulating all Wayland data structures needed by the Egl backend.
*
* It creates the connection to the Wayland Compositor, sets up the registry and creates
* the Wayland surface and its shell mapping.
*/
class KWIN_EXPORT WaylandBackend : public QObject
{
    Q_OBJECT
public:
    virtual ~WaylandBackend();
    wl_display *display();
    wl_registry *registry();
    void setCompositor(wl_compositor *c);
    wl_compositor *compositor();
    void addOutput(wl_output *o);
    const QList<Output*> &outputs() const;
    ShmPool *shmPool();
    void createSeat(uint32_t name);
    void createShm(uint32_t name);

    wl_surface *surface() const;
    QSize shellSurfaceSize() const;
    void installCursorImage(Qt::CursorShape shape);
Q_SIGNALS:
    void shellSurfaceSizeChanged(const QSize &size);
    void systemCompositorDied();
    void backendReady();
    void outputsChanged();
    void connectionFailed();
private:
    void initConnection();
    void createSurface();
    void destroyOutputs();
    void checkBackendReady();
    wl_display *m_display;
    wl_event_queue *m_eventQueue;
    Registry *m_registry;
    wl_compositor *m_compositor;
    Shell *m_shell;
    wl_surface *m_surface;
    ShellSurface *m_shellSurface;
    QScopedPointer<WaylandSeat> m_seat;
    QScopedPointer<ShmPool> m_shm;
    QList<Output*> m_outputs;
    ConnectionThread *m_connectionThreadObject;
    QThread *m_connectionThread;
    FullscreenShell *m_fullscreenShell;

    KWIN_SINGLETON(WaylandBackend)
};

inline
bool CursorData::isValid() const
{
    return m_valid;
}

inline
const QPoint& CursorData::hotSpot() const
{
    return m_hotSpot;
}

inline
const QImage &CursorData::cursor() const
{
    return m_cursor;
}

inline
wl_seat *WaylandSeat::seat()
{
    return m_seat;
}

inline
bool ShmPool::isValid() const
{
    return m_valid;
}

inline
void* ShmPool::poolAddress() const
{
    return m_poolData;
}

inline
wl_shm *ShmPool::shm()
{
    return m_shm;
}

inline
wl_display *WaylandBackend::display()
{
    return m_display;
}

inline
void WaylandBackend::setCompositor(wl_compositor *c)
{
    m_compositor = c;
}

inline
wl_compositor *WaylandBackend::compositor()
{
    return m_compositor;
}

inline
ShmPool* WaylandBackend::shmPool()
{
    return m_shm.data();
}

inline
wl_surface *WaylandBackend::surface() const
{
    return m_surface;
}

inline
const QList< Output* >& WaylandBackend::outputs() const
{
    return m_outputs;
}

inline
wl_buffer* Buffer::buffer() const
{
    return m_nativeBuffer;
}

inline
const QSize& Buffer::size() const
{
    return m_size;
}

inline
int32_t Buffer::stride() const
{
    return m_stride;
}

inline
bool Buffer::isReleased() const
{
    return m_released;
}

inline
void Buffer::setReleased(bool released)
{
    m_released = released;
}

inline
bool Buffer::isUsed() const
{
    return m_used;
}

inline
void Buffer::setUsed(bool used)
{
    m_used = used;
}

} // namespace Wayland
} // namespace KWin

#endif //  KWIN_WAYLAND_BACKEND_H
