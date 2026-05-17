#define _GNU_SOURCE
#include "tree_vis.h"
#include "xdg-shell-client-protocol.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

static const int START_WIDTH = 640;
static const int START_HEIGHT = 480;

/* Wayland code */
typedef struct {
    uint32_t width;
    uint32_t height;

    /* Globals */
    struct wl_display *wl_display;
    struct wl_registry *wl_registry;
    struct wl_compositor *wl_compositor;
    struct xdg_wm_base *xdg_wm_base;
    struct wl_buffer **buffers;
    struct wl_shm *wl_shm;
    uint32_t *pixel_data;
    uint32_t buffer_size;
    bool used_buffers[2];
    uint32_t last_frame_time;
    struct wl_callback *frame_callback;
    bool running;

    /* Objects */
    struct wl_surface *wl_surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
} client_state;

static int allocate_shm_file(size_t size) {
    int fd = memfd_create("wl_tree_vis_shm", MFD_ALLOW_SEALING);

    if (fd < 0)
        return -1;

    int ret;
    do {
        ret = ftruncate(fd, size);
    } while (ret < 0 && errno == EINTR);

    fcntl(fd, F_ADD_SEALS, F_SEAL_GROW | F_SEAL_SHRINK | F_SEAL_SEAL);

    if (ret < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer) {
    client_state *state = data;

    uint32_t buffer_index = wl_buffer == state->buffers[0] ? 0 : 1;

    state->used_buffers[buffer_index] = false;
}

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = wl_buffer_release,
};

static int32_t create_window(client_state *state) {
    if (state->pixel_data) {
        munmap(state->pixel_data, state->buffer_size * 2);
    }

    if (state->buffers[0]) {
        wl_buffer_destroy(state->buffers[0]);
    }
    if (state->buffers[1]) {
        wl_buffer_destroy(state->buffers[1]);
    }

    int stride = state->width * 4;
    int buffer_size = stride * state->height;
    int total_size = buffer_size * 2;

    int fd = allocate_shm_file(total_size);
    if (fd == -1) {
        return -1;
    }

    uint32_t *data = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        return -1;
    }

    struct wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, fd, total_size);
    state->buffers[0] = wl_shm_pool_create_buffer(pool, 0, state->width, state->height, stride,
                                                  WL_SHM_FORMAT_XRGB8888);
    state->buffers[1] = wl_shm_pool_create_buffer(pool, buffer_size, state->width, state->height,
                                                  stride, WL_SHM_FORMAT_XRGB8888);

    wl_buffer_add_listener(state->buffers[0], &wl_buffer_listener, state);
    wl_buffer_add_listener(state->buffers[1], &wl_buffer_listener, state);

    state->used_buffers[0] = false;
    state->used_buffers[1] = false;

    wl_shm_pool_destroy(pool);
    close(fd);

    state->pixel_data = data;
    state->buffer_size = buffer_size;

    return 0;
}

void frame_callback_done(void *data, struct wl_callback *wl_callback, uint32_t time);

static const struct wl_callback_listener frame_callback_listener = {
    .done = frame_callback_done,
};

static void draw_frame(client_state *state, uint32_t time_delta) {
    if (!state->frame_callback) {
        state->frame_callback = wl_surface_frame(state->wl_surface);
        wl_callback_add_listener(state->frame_callback, &frame_callback_listener, state);
    }

    uint32_t current_buffer;
    if (!state->used_buffers[0]) {
        current_buffer = 0;
    } else if (!state->used_buffers[1]) {
        current_buffer = 1;
    } else {
        wl_surface_commit(state->wl_surface);
        return;
    }
    uint32_t *pixels =
        state->pixel_data + (current_buffer * state->buffer_size) /
                                sizeof(uint32_t); // Clangd complains, but this is correct

#if 0
    printf("%d ms\n", time_delta);
    printf("%f fps\n", 1000.0f / (float)time_delta);
#endif

    update_and_render(pixels, state->width, state->height, time_delta);

    wl_surface_attach(state->wl_surface, state->buffers[current_buffer], 0, 0);
    wl_surface_damage_buffer(state->wl_surface, 0, 0, state->width, state->height);
    wl_surface_commit(state->wl_surface);

    state->used_buffers[current_buffer] = true;
}

void frame_callback_done(void *data, struct wl_callback *wl_callback, uint32_t time) {
    client_state *state = data;

    if (wl_callback == state->frame_callback) {
        state->frame_callback = NULL;
    }

    wl_callback_destroy(wl_callback);

    if (state->last_frame_time == 0)
        state->last_frame_time = time;

    draw_frame(state, time - state->last_frame_time);

    state->last_frame_time = time;
}

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
    client_state *state = data;
    xdg_surface_ack_configure(xdg_surface, serial);

    draw_frame(state, 0);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

void xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width,
                            int32_t height, struct wl_array *states) {
    client_state *state = data;
    if (width <= 0) {
        width = START_WIDTH;
    }
    if (height <= 0) {
        height = START_HEIGHT;
    }

    if (state->width == width && state->height == height)
        return;

    state->width = width;
    state->height = height;

    create_window(state);
}

void xdg_toplevel_configure_bounds(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width,
                                   int32_t height) {
}

void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) {
    client_state *state = data;
    state->running = false;
}

void xdg_toplevel_wm_capabilities(void *data, struct xdg_toplevel *xdg_toplevel,
                                  struct wl_array *capabilities) {
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .configure_bounds = xdg_toplevel_configure_bounds,
    .close = xdg_toplevel_close,
    .wm_capabilities = xdg_toplevel_wm_capabilities,
};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void find_and_bind_global(void **bind_to, struct wl_registry *registry, uint32_t name,
                                 const char *find_interface, const struct wl_interface *interface,
                                 uint32_t version) {
    if (strcmp(find_interface, interface->name) == 0) {
        *bind_to = wl_registry_bind(registry, name, interface, version);
    }
}

static void registry_global(void *data, struct wl_registry *wl_registry, uint32_t name,
                            const char *interface, uint32_t version) {
    client_state *state = data;

    find_and_bind_global((void **)&state->wl_shm, wl_registry, name, interface, &wl_shm_interface,
                         2);
    find_and_bind_global((void **)&state->wl_compositor, wl_registry, name, interface,
                         &wl_compositor_interface, 6);
    find_and_bind_global((void **)&state->xdg_wm_base, wl_registry, name, interface,
                         &xdg_wm_base_interface, 6);
}

static void registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {
    /* This space deliberately left blank */
}

static const struct wl_registry_listener wl_registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

int main(int argc, char *argv[]) {
    client_state state = {0};
    state.width = START_WIDTH;
    state.height = START_HEIGHT;

    struct wl_buffer *buffers[2] = {0};
    state.buffers = buffers;

    state.wl_display = wl_display_connect(NULL);
    state.wl_registry = wl_display_get_registry(state.wl_display);
    wl_registry_add_listener(state.wl_registry, &wl_registry_listener, &state);
    wl_display_roundtrip(state.wl_display);

    xdg_wm_base_add_listener(state.xdg_wm_base, &xdg_wm_base_listener, &state);

    state.wl_surface = wl_compositor_create_surface(state.wl_compositor);
    state.xdg_surface = xdg_wm_base_get_xdg_surface(state.xdg_wm_base, state.wl_surface);

    xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, &state);
    state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
    xdg_toplevel_set_title(state.xdg_toplevel, "Wayland RBTree Visualizer");
    xdg_toplevel_add_listener(state.xdg_toplevel, &xdg_toplevel_listener, &state);
    create_window(&state);
    wl_surface_commit(state.wl_surface);

    state.running = true;

    while (state.running && wl_display_dispatch(state.wl_display)) {
        /* This space deliberately left blank */
    }

    wl_buffer_destroy(state.buffers[0]);
    wl_buffer_destroy(state.buffers[1]);
    wl_surface_destroy(state.wl_surface);
    wl_display_disconnect(state.wl_display);

    return 0;
}
