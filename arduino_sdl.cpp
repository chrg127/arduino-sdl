#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <charconv>
#include <memory>
#include <utility>
#include <vector>
#include <array>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include "arduino_sdl.h"

#define FWD(...) std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

using glm::vec2;

enum {
    TEXTURE_GREEN_LED,
    TEXTURE_RED_LED,
    TEXTURE_BUTTON,
    TEXTURE_POTENTIOMETER,
};

struct Component {
    vec2 pos;
    int id;
    int frame;

    explicit Component(vec2 position, int tex_id, int frame_no)
        : pos{position}, id{tex_id}, frame{frame_no}
    { }

    virtual int  digital_read()  = 0;
    virtual void digital_write(uint8_t value) = 0;
    virtual int  analog_read()   = 0;
    virtual void analog_write(uint8_t value)  = 0;
    virtual void mouse_click(vec2 mouse_pos, bool pressed) = 0;
    virtual void mouse_wheel(vec2 mouse_pos, bool up_or_down) = 0;
};

struct LED : public Component {
    explicit LED(vec2 pos, arduino_sdl::LedColor color)
        : Component(pos, color == arduino_sdl::LedColor::Green ? TEXTURE_GREEN_LED : TEXTURE_RED_LED, 4)
    { }

    int  digital_read()               override { return 0; }
    void digital_write(uint8_t value) override { frame = value == LOW ? 0 : 7; }
    int  analog_read()                override { return 0; }
    void analog_write(uint8_t value)  override { frame = value / 32; }
    void mouse_click(vec2 mouse_pos, bool pressed)  override { }
    void mouse_wheel(vec2 mouse_pos, bool up_or_down) override { }
};

// used by Button
struct Rect {
    vec2 pos;
    vec2 size;
};

bool collision_rect_point(Rect r, vec2 p)
{
    return p.x >= r.pos.x && p.x <= r.pos.x + r.size.x
        && p.y >= r.pos.y && p.y <= r.pos.y + r.size.y;
}

struct Button : public Component {
    bool pressed = false;

    explicit Button(vec2 pos) : Component(pos, TEXTURE_BUTTON, 0) { }

    int  digital_read()               override { return pressed ? HIGH : LOW; }
    void digital_write(uint8_t value) override { }
    int  analog_read()                override { return 0; }
    void analog_write(uint8_t value)  override { }
    void mouse_click(vec2 mouse_pos, bool button_pressed)  override
    {
        bool inside = collision_rect_point({ .pos = this->pos, .size = {32,32} }, mouse_pos);
        pressed = inside ? button_pressed : false;
        frame = pressed;
    }
    void mouse_wheel(vec2 mouse_pos, bool up_or_down) override { }
};

struct Potentiometer : public Component {
    int value = 0;

    explicit Potentiometer(vec2 pos) : Component(pos, TEXTURE_POTENTIOMETER, 0) { }

    int  digital_read()               override { return 0; }
    void digital_write(uint8_t value) override { }
    int  analog_read()                override { return value; }
    void analog_write(uint8_t value)  override { }
    void mouse_click(vec2 mouse_pos, bool pressed)  override { }

    void mouse_wheel(vec2 mouse_pos, bool up_or_down) override
    {
        bool inside = collision_rect_point({ .pos = this->pos, .size = {32,32} }, mouse_pos);
        if (inside) {
            value += (up_or_down ? 1 : -1) * 64;
            value = value > 1023 ? 1023 : value < 0 ? 0 : value;
            frame = value / 128;
        }
    }
};

struct ArduinoBoard {
    std::vector<std::unique_ptr<Component>> components;
    std::array<int, 20> ports;

    template <typename T>
    int push_component(auto... args)
    {
        components.emplace_back(std::make_unique<T>(FWD(args)...));
        return components.size() - 1;
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

void update()
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

void draw()
{
    SDL_SetRenderDrawColor(SDL.rd, 0, 0, 0, 0xff);
    SDL_RenderClear(SDL.rd);
    for (auto &c : board.components)
        if (c->id != -1)
            draw_frame(c->pos, c->id, c->frame);
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


/* the stuff defined in the header file start here */

String::String() : start(nullptr), end(nullptr) { }
String::String(const String &s) { operator=(s); }
String::String(String &&s)      { operator=(std::move(s)); }

String & String::operator=(const String &s)
{
    construct(s.start, s.length());
    return *this;
}

String & String::operator=(String &&s)
{
    std::swap(start, s.start);
    std::swap(end, s.end);
    return *this;
}

String::String(const char *s)
{
    construct(s, strlen(s) + 1);
}

String::String(int n, int base)
{
    start = new char[1024];
    end = start + 1024;
    memset(start, 0, 1024);
    auto err = std::to_chars(start, end, n, base);
    if (err.ec != std::errc())
        fprintf(stderr, "warning: couldn't convert %d to string\n", n);
}

String::~String()
{
    delete[] start;
}

void String::construct(const char *s, unsigned int len)
{
    start = new char[len];
    std::memcpy(start, s, len);
    end = start + len;
    end[-1] = '\0';
}

String concat(const char *a, size_t la, const char *b, size_t lb)
{
    char *r = new char[la + lb + 1];
    std::memcpy(r, a, la);
    std::memcpy(r + la, b, lb);
    r[la+lb] = '\0';
    String s;
    s.construct(r, la+lb+1);
    return s;
}

String operator+(const String &lhs, const String &rhs)
{
    return concat(lhs.data(), lhs.length(), rhs.data(), rhs.length());
}

String operator+(const String &lhs, const char *rhs)
{
    return concat(lhs.data(), lhs.length(), rhs, std::strlen(rhs));
}

String operator+(const char *lhs, const String &rhs)
{
    return concat(lhs, std::strlen(lhs), rhs.data(), rhs.length());
}

void HardwareSerial::begin(int n) { }
void HardwareSerial::println(const String &msg) { printf("%s\n", msg.data()); }
void HardwareSerial::println(const char *msg)   { printf("%s\n", msg); }
void HardwareSerial::println(int n)             { printf("%d\n", n); }

HardwareSerial Serial;

void pinMode(uint8_t pin, uint8_t value) { }
int digitalRead(uint8_t pin)                  { return board.components[board.ports[pin]]->digital_read(); }
int analogRead(uint8_t pin)                   { return board.components[board.ports[pin]]->analog_read(); }
void digitalWrite(uint8_t pin, uint8_t value) { board.components[board.ports[pin]]->digital_write(value); }
void analogWrite(uint8_t pin, uint8_t value)  { board.components[board.ports[pin]]->analog_write(value); }

unsigned long millis()
{
    return SDL_GetTicks();
}

void delay(unsigned long ms)
{
    update();
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

namespace arduino_sdl {

void start(const char *title, int width, int height)
{
    SDL.init(title, width, height);
    std::srand(std::time(nullptr));

    load_gfx("led_green.bmp", {32, 32});
    load_gfx("led_red.bmp",   {32, 32});
    load_gfx("button.bmp",    {32, 32});
    load_gfx("pot.bmp",       {32, 32});
}

void loop()
{
    setup();
    while (SDL.running) {
        update();
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

void connect_led(int pin, int x, int y, LedColor color) { connect_component<LED>(pin, vec2{x,y}, color); }
void connect_button(int pin, int x, int y)              { connect_component<Button>(pin, vec2{x,y}); }
void connect_potentiometer(int pin, int x, int y)       { connect_component<Potentiometer>(pin, vec2{x,y}); }

} // namespace arduino_sdl
