#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <ctime>
#include <charconv>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>
#include <utility>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <glm/glm.hpp>
#include "arduino_sdl.h"
#include "Wire.h"
#include "LiquidCrystal_I2C.h"

#define FWD(...) std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

using u32 = uint32_t;
using glm::vec2;

struct Rect {
    vec2 pos;
    vec2 size;
};

bool collision_rect_point(Rect r, vec2 p)
{
    return p.x >= r.pos.x && p.x <= r.pos.x + r.size.x
        && p.y >= r.pos.y && p.y <= r.pos.y + r.size.y;
}

void circle_rasterizer(float cx, float cy, float r, auto &&draw)
{
    float x1 = cx - r, y1 = cy - r,
          x2 = cx + r, y2 = cy + r;
    for (auto y = y1; y < y2; y++) {
        for (auto x = x1; x < x2; x++) {
            auto xdist = x - cx + 0.5f,
                 ydist = y - cy + 0.5f;
            auto dist2 = xdist*xdist + ydist*ydist;
            if (dist2 <= r*r)
                draw(x, y);
        }
    }
}

auto rgba_to_components(u32 color)
{
    return std::tuple{ color >> 24 & 0xff, color >> 16 & 0xff, color >> 8 & 0xff, color & 0xff };
}

u32 components_to_rgba(int b, int g, int r, int a) { return b << 24 | g << 16 | r << 8 | a; }

u32 lerp_rgba(u32 min, u32 max, float t)
{
    auto [r1, g1, b1, a1] = rgba_to_components(min);
    auto [r2, g2, b2, a2] = rgba_to_components(max);
    return components_to_rgba(std::lerp(r1, r2, t),
                              std::lerp(g1, g2, t),
                              std::lerp(b1, b2, t),
                              std::lerp(a1, a2, t));
}


enum {
    TEXTURE_BUTTON,
    TEXTURE_POTENTIOMETER,
    TEXTURE_LCD,
    TEXTURE_FONT,
};

struct Component {
    virtual int  digital_read(uint8_t pin) = 0;
    virtual void digital_write(uint8_t pin, uint8_t value) = 0;
    virtual int  analog_read(uint8_t pin) = 0;
    virtual void analog_write(uint8_t pin, uint8_t value) = 0;
    virtual void mouse_click(vec2 mouse_pos, bool pressed) = 0;
    virtual void mouse_wheel(vec2 mouse_pos, bool up_or_down) = 0;
    virtual void draw() = 0;
};

struct ArduinoBoard {
    std::vector<std::unique_ptr<Component>> components;
    std::array<int, 20> ports;
    std::unordered_map<uint8_t, std::function<void(uint8_t)>> i2c_bus;

    template <typename T>
    int push_component(auto&&... args)
    {
        components.emplace_back(std::make_unique<T>(FWD(args)...));
        return components.size() - 1;
    }

    void add_i2c(uint8_t addr, auto &&fn)
    {
        i2c_bus[addr] = fn;
    }
} board;

struct {
    bool running = true;
    SDL_Window *window;
    SDL_Renderer *rd;
    vec2 mouse_pos;

    void init(const char *title, int width, int height)
    {
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  width, height, SDL_WINDOW_SHOWN);
        rd = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    }

    void quit()
    {
        SDL_DestroyRenderer(rd);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
} SDL;

struct Texture {
    SDL_Texture *data;
    vec2 size;
    vec2 image_size;
};

struct {
    std::vector<Texture> loaded_gfx;

    Texture & operator[](int id) { return loaded_gfx[id]; }
    int add(Texture &&gfx)
    {
        loaded_gfx.emplace_back(std::move(gfx));
        return loaded_gfx.size() - 1;
    }
} gfx_handler;



namespace {

void poll()
{
    for (SDL_Event ev; SDL_PollEvent(&ev); ) {
        switch (ev.type) {
        case SDL_QUIT:
            SDL.running = false;
            exit(0);
            break;
        case SDL_KEYUP:
        case SDL_KEYDOWN:
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            if (ev.button.button == SDL_BUTTON_LEFT)
                for (auto &c : board.components)
                    c->mouse_click({ev.button.x, ev.button.y}, ev.button.state == SDL_PRESSED);
            break;
        case SDL_MOUSEWHEEL:
            for (auto &c : board.components)
                c->mouse_wheel(SDL.mouse_pos, ev.wheel.y > 0);
            break;
        case SDL_MOUSEMOTION:
            SDL.mouse_pos.x = ev.motion.x;
            SDL.mouse_pos.y = ev.motion.y;
            break;
        }
    }
}

void draw_frame(vec2 pos, int gfx_id, int frame)
{
    vec2 p = pos;
    auto &tex = gfx_handler[gfx_id];
    SDL_Rect src = { int(frame * tex.size.x), 0, int(tex.size.x), int(tex.size.y) };
    SDL_Rect dst = { int(p.x), int(p.y), int(tex.size.x), int(tex.size.y) };
    SDL_RenderCopy(SDL.rd, tex.data, &src, &dst);
}

void draw_character(vec2 pos, char c)
{
    int x = int(c) % 16;
    int y = int(c) / 16;
    auto &tex = gfx_handler[TEXTURE_FONT];
    SDL_Rect src = {     x * 32,     y * 32, 32, 32 };
    SDL_Rect dst = { int(pos.x), int(pos.y), 32, 32 };
    SDL_RenderCopy(SDL.rd, tex.data, &src, &dst);
    // SDL_SetRenderDrawColor(SDL.rd, 0xff, 0, 0, 0xff);
    // SDL_Rect r = { int(pos.x), int(pos.y), 32, 32 };
    // SDL_RenderDrawRect(SDL.rd, &r);
}

void draw_circle(vec2 pos, float radius, unsigned color)
{
    circle_rasterizer(pos.x, pos.y, radius, [&](float x, float y) {
        auto [r, g, b, a] = rgba_to_components(color);
        SDL_SetRenderDrawColor(SDL.rd, r, g, b, a);
        SDL_RenderDrawPoint(SDL.rd, int(x), int(y));
    });
}

void draw()
{
    SDL_SetRenderDrawColor(SDL.rd, 0, 0, 0, 0xff);
    SDL_RenderClear(SDL.rd);
    for (auto &c : board.components)
        c->draw();
    SDL_RenderPresent(SDL.rd);
}

int load_gfx(std::string_view pathname, vec2 frame_size)
{
    auto *bmp = SDL_LoadBMP(pathname.data());
    assert(bmp && "load of bmp image failed");
    auto *tex = SDL_CreateTextureFromSurface(SDL.rd, bmp);
    SDL_FreeSurface(bmp);
    return gfx_handler.add((Texture) { .data = tex, .size = frame_size, .image_size = {bmp->w, bmp->h} });
}

} // namespace



struct LED : public Component {
    vec2 pos;
    u32 color_min;
    u32 color_max;
    uint8_t val = 0;

    explicit LED(vec2 pos, u32 min, u32 max) : pos{pos}, color_min{min}, color_max{max} {}
    int  digital_read(uint8_t)                 override { return 0; }
    // assume we only pass LOW (0) or HIGH (1) to digital_write
    void digital_write(uint8_t, uint8_t value) override { val = value * 255; }
    int  analog_read(uint8_t)                  override { return 0; }
    void analog_write(uint8_t, uint8_t value)  override { val = value; }
    void mouse_click(vec2 mouse_pos, bool pressed)  override { }
    void mouse_wheel(vec2 mouse_pos, bool up_or_down) override { }

    void draw() override
    {
        draw_circle(pos + vec2{16.f, 1.6f}, 16.f, lerp_rgba(color_min, color_max, val / 255.f));
    }
};

struct Button : public Component {
    vec2 pos;
    bool pressed = false;

    explicit Button(vec2 pos) : pos{pos} {}

    int  digital_read(uint8_t)                 override { return pressed ? HIGH : LOW; }
    void digital_write(uint8_t, uint8_t value) override { }
    int  analog_read(uint8_t)                  override { return 0; }
    void analog_write(uint8_t, uint8_t value)  override { }
    void mouse_wheel(vec2 mouse_pos, bool up_or_down) override { }

    void mouse_click(vec2 mouse_pos, bool button_pressed)  override
    {
        bool inside = collision_rect_point({ .pos = pos, .size = {32,32} }, mouse_pos);
        pressed = inside ? button_pressed : false;
    }

    void draw() override
    {
        draw_frame(pos, TEXTURE_BUTTON, int(pressed));
    }
};

struct Potentiometer : public Component {
    vec2 pos;
    int value = 0;

    explicit Potentiometer(vec2 pos) : pos{pos} {}

    int  digital_read(uint8_t)                 override { return 0; }
    void digital_write(uint8_t, uint8_t value) override { }
    int  analog_read(uint8_t)                  override { return value; }
    void analog_write(uint8_t, uint8_t value)  override { }
    void mouse_click(vec2 mouse_pos, bool pressed)  override { }

    void mouse_wheel(vec2 mouse_pos, bool up_or_down) override
    {
        bool inside = collision_rect_point({ .pos = pos, .size = {32,32} }, mouse_pos);
        if (inside) {
            value += (up_or_down ? 1 : -1) * 64;
            value = value > 1023 ? 1023 : value < 0 ? 0 : value;
            // frame = value / 128;
        }
    }

    void draw() override
    {
        draw_frame(pos, TEXTURE_POTENTIOMETER, value / 128);
    }
};

// struct PIR : public Component {
//     int  digital_read()               override { return 0; }
//     void digital_write(uint8_t value) override { }
//     int  analog_read()                override { return 0; }
//     void analog_write(uint8_t value)  override { }
//     void mouse_click(vec2 mouse_pos, bool pressed)  override { }
//     void mouse_wheel(vec2 mouse_pos, bool up_or_down) override { }
//     void draw() override {}
// };

struct LCD : public Component {
    vec2 pos, size;
    uint8_t sda, scl;
    std::vector<char> char_buf;

    LCD(vec2 pos, vec2 size, uint8_t addr, uint8_t sda, uint8_t scl)
        : pos{pos}, size{size}, sda{sda}, scl{scl}
    {
        board.add_i2c(addr, [&](uint8_t val) {});
        char_buf = std::vector(size.x * size.y, '1');
    }

    int  digital_read(uint8_t)                 override { return 0; }
    void digital_write(uint8_t, uint8_t value) override { }
    int  analog_read(uint8_t)                  override { return 0; }
    void analog_write(uint8_t, uint8_t value)  override { }

    void mouse_click(vec2 mouse_pos, bool pressed)  override { }
    void mouse_wheel(vec2 mouse_pos, bool up_or_down) override { }

    void draw() override
    {
        draw_frame(pos,                                   TEXTURE_LCD, 0);
        draw_frame(pos + vec2{size.x+1,        0} * 32.f, TEXTURE_LCD, 1);
        draw_frame(pos + vec2{       0, size.y+1} * 32.f, TEXTURE_LCD, 2);
        draw_frame(pos + vec2{size.x+1, size.y+1} * 32.f, TEXTURE_LCD, 3);

        for (auto i = 0u; i < size.x; i++) {
            draw_frame(pos + vec2{i+1,        0} * 32.f, TEXTURE_LCD, 4);
            draw_frame(pos + vec2{i+1, size.y+1} * 32.f, TEXTURE_LCD, 5);
        }

        for (auto i = 0u; i < size.y; i++) {
            draw_frame(pos + vec2{       0, i+1} * 32.f, TEXTURE_LCD, 6);
            draw_frame(pos + vec2{size.x+1, i+1} * 32.f, TEXTURE_LCD, 7);
        }

        // draw lcd text
        for (auto y = 0u; y < size.y; y++) {
            for (auto x = 0u; x < size.x; x++) {
                draw_character(pos + vec2{x+1,y+1} * 32.f, char_buf[y * size.x + x]);
            }
        }
    }
};



/* the stuff defined in the header file start here */

void HardwareSerial::begin(int n) { }
void HardwareSerial::println(const String &msg) { printf("%s\n", msg.data()); }
void HardwareSerial::println(const char *msg)   { printf("%s\n", msg); }
void HardwareSerial::println(int n)             { printf("%d\n", n); }

HardwareSerial Serial;

void pinMode(uint8_t pin, uint8_t value) { }
int digitalRead(uint8_t pin)                  { return board.components[board.ports[pin]]->digital_read(pin); }
int analogRead(uint8_t pin)                   { return board.components[board.ports[pin]]->analog_read(pin); }
void digitalWrite(uint8_t pin, uint8_t value) { board.components[board.ports[pin]]->digital_write(pin, value); }
void analogWrite(uint8_t pin, uint8_t value)  { board.components[board.ports[pin]]->analog_write(pin, value); }

unsigned long millis()
{
    return SDL_GetTicks();
}

void delay(unsigned long ms)
{
    poll();
    draw();
    SDL_Delay(ms);
}

void attachInterrupt(uint8_t interruptNum, void (*userFunc)(void), int mode) {}
void detachInterrupt(uint8_t interruptNum) {}

uint16_t makeWord(uint16_t w)     { return 0; }
uint16_t makeWord(byte h, byte l) { return 0; }

long random(long n)
{
    return random(0, n);
}

long random(long a, long b)
{
    return (rand() % int(b - a)) + a;
}

void randomSeed(unsigned long seed)
{
    std::srand(seed);
}

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}



/* Wire stuff */

void _wire::write(uint8_t data)
{
    board.i2c_bus[cur_addr](data);
}



/* LiquidCrystal_I2C stuff */

LiquidCrystal_I2C::LiquidCrystal_I2C(uint8_t addr, uint8_t cols, uint8_t rows)
    : addr{addr}, cols{cols}, rows{rows}
{ }

void LiquidCrystal_I2C::init() {}
void LiquidCrystal_I2C::backlight() {}
void LiquidCrystal_I2C::clear() {}
void LiquidCrystal_I2C::setCursor(int x, int y) {}
void LiquidCrystal_I2C::print(String s) {}



namespace arduino_sdl {

void start(const char *title, int width, int height)
{
    SDL.init(title, width, height);
    std::srand(std::time(nullptr));
    load_gfx("button.bmp",    {32, 32});
    load_gfx("pot.bmp",       {32, 32});
    load_gfx("lcd1.bmp",      {32, 32});
    load_gfx("font.bmp",      {32, 32});
}

void loop()
{
    setup();
    while (SDL.running) {
        poll();
        ::loop();
        draw();
    }
}

void quit()
{
    SDL.quit();
}

template <typename T>
void connect_component(int pin, auto... args)
{
    board.ports[pin] = board.push_component<T>(FWD(args)...);
}

void connect_led(int pin, int x, int y, u32 min, u32 max) { connect_component<LED>(pin, vec2{x,y}, min, max); }
void connect_button(int pin, int x, int y)              { connect_component<Button>(pin, vec2{x,y}); }
void connect_potentiometer(int pin, int x, int y)       { connect_component<Potentiometer>(pin, vec2{x,y}); }

void connect_lcd(uint8_t addr, uint8_t sda, uint8_t scl, int x, int y, int r, int c)
{
    int i = board.push_component<LCD>(vec2{x, y}, vec2{r, c}, addr, sda, scl);
    board.ports[sda] = i;
    board.ports[scl] = i;
}

} // namespace arduino_sdl
