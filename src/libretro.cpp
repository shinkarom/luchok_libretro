#include <stdio.h>
#include <iostream>
#include <random>
#include <cassert>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include <stdio.h>
#if defined(_WIN32) && !defined(_XBOX)
#include <windows.h>
#endif
#include "libretro.h"

extern "C" {
	#include "lua.h"
	#include "lauxlib.h"
	#include "lualib.h"
}

constexpr uint32_t ON_COLOR = 0xFFFFFFFF;
constexpr uint32_t OFF_COLOR = 0x00000000;

constexpr auto screenWidth = 64;
constexpr auto screenHeight = 32;
constexpr auto screenTotalPixels = screenWidth * screenHeight;
constexpr auto audioSampleRate = 44100;

constexpr int SQUARE_WAVE_FREQUENCY = 440;
constexpr short AUDIO_VOLUME = 28000;

static const char* VBLANK_FUNCTION = "vblank";
static const char* CLS_FUNCTION = "cls";
static const char* DRAW_FUNCTION = "draw";
static const char* RND_FUNCTION = "rnd";
static const char* KEY_PRESSED_FUNCTION = "key_pressed";
static const char* KEY_RELEASED_FUNCTION = "key_released";
static const char* BCD_FUNCTION = "bcd";
static const char* GET_SPRITE_FUNCTION = "get_sprite";
static const char* DELAY_TIMER_VARIABLE = "delay_timer";
static const char* SOUND_TIMER_VARIABLE = "sound_timer";

const int keys[16] = {RETROK_x, RETROK_1, RETROK_2, RETROK_3,
	RETROK_q, RETROK_w ,RETROK_e, RETROK_a,
	RETROK_s, RETROK_d, RETROK_z, RETROK_c,
	RETROK_4, RETROK_r, RETROK_f, RETROK_v
};

const int font[16][5] = {
    {0xF0, 0x90, 0x90, 0x90, 0xF0}, //0
    {0x20, 0x60, 0x20, 0x20, 0x70}, //1
    {0xF0, 0x10, 0xF0, 0x80, 0xF0}, //2
    {0xF0, 0x10, 0xF0, 0x10, 0xF0}, //3
    {0x90, 0x90, 0xF0, 0x10, 0x10}, //4
    {0xF0, 0x80, 0xF0, 0x10, 0xF0}, //5
    {0xF0, 0x80, 0xF0, 0x90, 0xF0}, //6
    {0xF0, 0x10, 0x20, 0x40, 0x40}, //7
    {0xF0, 0x90, 0xF0, 0x90, 0xF0}, //8
    {0xF0, 0x90, 0xF0, 0x10, 0xF0}, //9
    {0xF0, 0x90, 0xF0, 0x90, 0x90}, //A
    {0xE0, 0x90, 0xE0, 0x90, 0xE0}, //B
    {0xF0, 0x80, 0x80, 0x80, 0xF0}, //C
    {0xE0, 0x90, 0x90, 0x90, 0xE0}, //D
    {0xF0, 0x80, 0xF0, 0x80, 0xF0}, //E
    {0xF0, 0x80, 0xF0, 0x80, 0x80}, //F
    };

static uint32_t *frame_buf;
static struct retro_log_callback logging;
static retro_log_printf_t log_cb;
static retro_environment_t environ_cb;
static bool use_audio_cb;
static float last_aspect;
static float last_sample_rate;
char retro_base_directory[4096];
char retro_game_path[4096];

int delay_timer;
int sound_timer;
bool playing;

std::default_random_engine generator{static_cast<long unsigned int>(time(0))};

lua_State* lua;

bool pressed_keys[16];
bool released_keys[16];

bool isKeyPressed(int value){
    assert(value >=0 && value <= 15);
    return pressed_keys[value];
}

bool isKeyReleased(int value){
    assert(value >=0 && value <= 15);
    return released_keys[value];
}

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   (void)level;
   va_list va;
   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

int luaErrorHandler(lua_State *L) {
	//log_cb(RETRO_LOG_ERROR, lua_tostring(L, -1));
	std::cout<<lua_tostring(L, -1);
	environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, nullptr);
	return 1;
}

void luaCallVBlank() {
	lua_pushcfunction(lua, luaErrorHandler);
	lua_getglobal(lua, VBLANK_FUNCTION);
	if(lua_isfunction(lua, -1)) {
		lua_pcall(lua, 0, 0, -2);
	} else {
		log_cb(RETRO_LOG_ERROR, "No function vblank()");
		environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, nullptr);
	}
}

void cls() {
	for(int i = 0; i < screenTotalPixels; i++) {
		frame_buf[i] = OFF_COLOR;
	}
}

int luaCls(lua_State* L) {
	if(lua_gettop(lua) != 0) {
		luaL_error(lua, "cls() expects no arguments");
	}
	
	cls();
	
	return 0;
}

bool getPixel(int x, int y){
    return frame_buf[y * screenWidth + x] == ON_COLOR;
}

void setPixel(int x, int y, bool value){
    frame_buf[y * screenWidth + x] = value ? ON_COLOR : OFF_COLOR;
}

bool drawByte(int value, int x,  int y) {
	 bool result = false;
    for(int i = 0; i < 8; i++){
        bool thisBit = (value >> (7 - i)) & 1;
        auto thisColor = thisBit ? ON_COLOR : OFF_COLOR;

        int new_x = (x + i) % screenWidth;
        if(new_x < 0){
            new_x = screenWidth + new_x;
        }

        int new_y = y % screenHeight;
        if(new_y < 0){
            new_y = screenHeight + new_y;
        }

        //std::cout<<x<<" "<<y<<" "<<new_x<<" "<<new_y<<" "<<std::endl;

        bool currentBit = getPixel(new_x, new_y);
        if(!result && currentBit && thisBit)
        {
            result = true;
        }

        bool newBit = currentBit ^ thisBit;
        setPixel(new_x, new_y, newBit);
        //std::cout << x<< " " << y << " " 
        //    << currentBit << " " << thisBit << " " << 
        //    newBit << " " << result << std::endl;
    }
    return result;
}

int luaDraw(lua_State* L) {
	    int start, len;
    if(lua_gettop(lua) == 5){
        start = luaL_checkinteger(lua, 4);
        len = luaL_checkinteger(lua, 5);
        luaL_argcheck(lua, (start >= 1), 4, "draw() start must be larger than 0");
        luaL_argcheck(lua, (len >= 0 && len <= screenHeight), 5, "cls() lenmust be between 0 and 31");
    }
    else if(lua_gettop(lua) == 3){
        start = 1;
        len = screenHeight;
    }
    else{
        luaL_error(lua, "cls() expecting either 3 or 5 arguments");
    }
    
    bool result = false;
    int x = luaL_checkinteger(lua, 2);
    int y = luaL_checkinteger(lua, 3);

   // std::cout<<"drawing at "<<x<<" "<<y<<std::endl;

    luaL_argcheck(lua, lua_istable(lua, 1), 1, "cls() argument must be a table");
    luaL_argcheck(lua, (x >= 0 && x < screenWidth), 2, "cls() x must be between 0 and 63");
    luaL_argcheck(lua, (y >= 0 && y < screenHeight), 3, "cls() y must be between 0 and 31");

    for(int i = start; i < start + len; i++){
        lua_geti(lua, 1, i);
        if(lua_isnil(lua, -1)){
            lua_pop(lua, 2);
            break;
        }
        int b = luaL_checkinteger(lua, -1);
        if(b < 0 || b > 255)
        {
            luaL_error(lua, "can't draw %d, must be between 0 and 255", b);
        }
        if(!result && drawByte(b, x, y+i-1)){
            result = true;
        }
        lua_pop(lua, 1);
    }
    lua_pop(lua, 1);
    lua_pushboolean(lua, result);
    return 1;
}

int luaRnd(lua_State *L) {
	if(lua_gettop(lua) != 1){
        luaL_error(lua, "rnd() expecting 1 argument");
    }

    int value = luaL_checkinteger(lua, -1);
    lua_pop(lua, 1);

    luaL_argcheck(lua, (value >=0 && value <= 255), 1, "rnd() valuemust be between 0 and 255");

    std::uniform_int_distribution<int> distribution(0, value - 1);
    distribution(generator);
    int r = distribution(generator);
    int new_num = r % value;
    //std::cout<<"generated "<<new_num<<std::endl;
    lua_pushinteger(lua, new_num);
    return 1;
}

int luaGetSprite(lua_State *L) {
	if(lua_gettop(lua) != 1){
        luaL_error(lua, "get_sprite() expecting 1 argument");
    }

    int value = luaL_checkinteger(lua, -1);
    luaL_argcheck(lua, (value >= 0 && value <= 15), 1, "get_sprite() argument must be between 0 and 15");
    lua_pop(lua, 1);
    lua_createtable(lua, 5, 0);
    for(int i = 1; i <= 5; i++){
        lua_pushinteger(lua, font[value][i-1]);
        lua_seti(lua, -2, i);
    }
    return 1;
}

int luaBcd(lua_State *L) {
	 if(lua_gettop(lua) != 1){
        luaL_error(lua, "rnd() expecting 1 argument");
    }

    int value = luaL_checkinteger(lua, -1);
    luaL_argcheck(lua, (value >= 0 && value <= 255), 1, "rnd() argument must be between 0 and 255");
    lua_pop(lua, 1);
    lua_newtable(lua);
    lua_pushinteger(lua, value / 100);
    lua_seti(lua, -2, 1);
    lua_pushinteger(lua, (value / 10) % 10);
    lua_seti(lua, -2, 2);
    lua_pushinteger(lua, value % 10);
    lua_seti(lua, -2, 3);
    return 1;
}

int luaKeyPressed(lua_State* L){
    if(lua_gettop(lua) != 1){
        luaL_error(lua, "key_pressed() expecting 1 argument");
    }

    int result = 0;
    int value = luaL_checkinteger(lua, -1);
    luaL_argcheck(lua, (value >= 0 && value <= 15), 1, "key_pressed() argument must be between 0 and 15");
    lua_pop(lua, 1);
    lua_pushboolean(lua, isKeyPressed(value));
    return 1;
}

int luaKeyReleased(lua_State* L){
    if(lua_gettop(lua) != 1){
        luaL_error(lua, "key_released() expecting 1 argument");
    }

    int result = 0;
    int value = luaL_checkinteger(lua, -1);
    luaL_argcheck(lua, (value >= 0 && value <= 15), 1, "key_released() argument must be between 0 and 15");
    lua_pop(lua, 1);
    lua_pushboolean(lua, isKeyReleased(value));
    return 1;
}

void luaCreate() {
	lua = luaL_newstate();
	luaL_openlibs(lua);
	
	lua_register(lua, CLS_FUNCTION, luaCls);
	lua_register(lua, DRAW_FUNCTION, luaDraw);
	lua_register(lua, RND_FUNCTION, luaRnd);
    lua_register(lua, KEY_PRESSED_FUNCTION, luaKeyPressed);
    lua_register(lua, KEY_RELEASED_FUNCTION, luaKeyReleased);
    lua_register(lua, BCD_FUNCTION, luaBcd);
    lua_register(lua, GET_SPRITE_FUNCTION, luaGetSprite);
}

void luaDelete() {
	lua_close(lua);
}

void processDelayTimer() {
	lua_getglobal(lua, DELAY_TIMER_VARIABLE);
    delay_timer = lua_tointeger(lua, -1);
    if(delay_timer < 0) {
		delay_timer = 0;
	} else if(delay_timer > 255) {
		delay_timer = 255;
	}
	
    lua_pop(lua, 1);

    if(delay_timer > 0){
        delay_timer--;
    }

   // std::cout<<"delay_timer "<<delay_timer<<std::endl;
    lua_pushinteger(lua, delay_timer);
    lua_setglobal(lua, DELAY_TIMER_VARIABLE);
}

void processSoundTimer() {
	 lua_getglobal(lua, SOUND_TIMER_VARIABLE);
    sound_timer = lua_tointeger(lua, -1);
	
    if(sound_timer < 0) {
		sound_timer = 0;
	} else if(sound_timer > 255) {
		sound_timer = 255;
	}
	
    lua_pop(lua, 1);

    if(sound_timer > 0){
        sound_timer--;
    }
	
    playing = sound_timer > 0;
  // std::cout<<"sound_timer "<<sound_timer<<" paused "<<paused<<std::endl;
    lua_pushinteger(lua, sound_timer);
    lua_setglobal(lua, SOUND_TIMER_VARIABLE);
}

void retro_init(void)
{
   frame_buf = new uint32_t[screenTotalPixels];
   memset(frame_buf,0,screenTotalPixels*sizeof(uint32_t));
   const char *dir = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
   {
      snprintf(retro_base_directory, sizeof(retro_base_directory), "%s", dir);
   }
   
   luaCreate();
   
}

void retro_deinit(void)
{
   delete[] frame_buf;
   luaDelete();
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
  // log_cb(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "Luchok fantasy console";
   info->library_version  = "1.0";
   info->need_fullpath    = false;
   info->valid_extensions = "luchok";
}

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   float aspect                = 0.0f;
   float sampling_rate         = audioSampleRate*1.0f;


   info->geometry.base_width   = screenWidth;
   info->geometry.base_height  = screenHeight;
   info->geometry.max_width    = screenWidth;
   info->geometry.max_height   = screenHeight;
   info->geometry.aspect_ratio = aspect;

   last_aspect                 = aspect;
   last_sample_rate            = sampling_rate;
}

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = fallback_log;

   static const struct retro_controller_description controllers[] = {
      { "Keypad", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_KEYBOARD, 0) },
   };

   static const struct retro_controller_info ports[] = {
      { controllers, 1 },
      { NULL, 0 },
   };

   cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_reset(void)
{

}

static void update_input(void)
{

}


static void check_variables(void)
{

}


static void keyboard_cb(bool down, unsigned keycode,
      uint32_t character, uint16_t mod)
{
   for(int i=0; i < 16; i++) {
	   int key = keys[i];
		pressed_keys[i] = (keycode == key) && down;
		released_keys[i] = (keycode == key) && !down;
   }
}

static void audio_callback(void)
{
	static unsigned phase;
	int period = audioSampleRate / SQUARE_WAVE_FREQUENCY;
   for (unsigned i = 0; i < audioSampleRate / 60; i++, phase++)
   {
      int16_t val = ((phase % period) < (period / 2))
            ? AUDIO_VOLUME : 0;
	if(playing) {
		audio_cb(val, val);
	} else {
		audio_cb(0,0);
	}
      
   }

   phase %= 100;
}

static void audio_set_state(bool enable)
{
   (void)enable;
}

void retro_run(void)
{
	processDelayTimer();
	processSoundTimer();
	luaCallVBlank();
	
	video_cb(frame_buf, screenWidth, screenHeight, screenWidth*sizeof(uint32_t));
	
   for(int i=0; i < 16; i++) {
	   int key = keys[i];
		pressed_keys[i] = false;
		released_keys[i] = false;
   }
   
   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables();
}

bool retro_load_game(const struct retro_game_info *info)
{
   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
      { 0 },
   };

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
      return false;
   }

   snprintf(retro_game_path, sizeof(retro_game_path), "%s", info->path);
   struct retro_audio_callback audio_cb = { audio_callback, audio_set_state };
   use_audio_cb = environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK, &audio_cb);
	
	struct retro_keyboard_callback cb = { keyboard_cb };
   environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);
	
   check_variables();

	if(luaL_dostring(lua, (const char*)(info->data)) != LUA_OK) {
		return false;
	}

   (void)info;
   return true;
}

void retro_unload_game(void)
{

}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   return false;
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void *data_, size_t size)
{
   return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
   return false;
}

void *retro_get_memory_data(unsigned id)
{
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   (void)id;
   return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

