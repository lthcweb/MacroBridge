/*
 * LogitechKeyMap.cpp  —  罗技键名映射实现
 */
#include "LogitechKeyMap.h"
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace LogitechKeyMap {

struct Entry { const char* lua; AIR::AIRKey key; };

// 正向表：罗技键名（小写）→ AIRKey
static const Entry kFwd[] = {
    // 字母
    {"a",AIR::AIRKey::KEY_A},{"b",AIR::AIRKey::KEY_B},{"c",AIR::AIRKey::KEY_C},
    {"d",AIR::AIRKey::KEY_D},{"e",AIR::AIRKey::KEY_E},{"f",AIR::AIRKey::KEY_F},
    {"g",AIR::AIRKey::KEY_G},{"h",AIR::AIRKey::KEY_H},{"i",AIR::AIRKey::KEY_I},
    {"j",AIR::AIRKey::KEY_J},{"k",AIR::AIRKey::KEY_K},{"l",AIR::AIRKey::KEY_L},
    {"m",AIR::AIRKey::KEY_M},{"n",AIR::AIRKey::KEY_N},{"o",AIR::AIRKey::KEY_O},
    {"p",AIR::AIRKey::KEY_P},{"q",AIR::AIRKey::KEY_Q},{"r",AIR::AIRKey::KEY_R},
    {"s",AIR::AIRKey::KEY_S},{"t",AIR::AIRKey::KEY_T},{"u",AIR::AIRKey::KEY_U},
    {"v",AIR::AIRKey::KEY_V},{"w",AIR::AIRKey::KEY_W},{"x",AIR::AIRKey::KEY_X},
    {"y",AIR::AIRKey::KEY_Y},{"z",AIR::AIRKey::KEY_Z},
    // 数字
    {"0",AIR::AIRKey::KEY_0},{"1",AIR::AIRKey::KEY_1},{"2",AIR::AIRKey::KEY_2},
    {"3",AIR::AIRKey::KEY_3},{"4",AIR::AIRKey::KEY_4},{"5",AIR::AIRKey::KEY_5},
    {"6",AIR::AIRKey::KEY_6},{"7",AIR::AIRKey::KEY_7},{"8",AIR::AIRKey::KEY_8},
    {"9",AIR::AIRKey::KEY_9},
    // 小键盘
    {"numpad0",AIR::AIRKey::KEY_NUMPAD0},{"numpad1",AIR::AIRKey::KEY_NUMPAD1},
    {"numpad2",AIR::AIRKey::KEY_NUMPAD2},{"numpad3",AIR::AIRKey::KEY_NUMPAD3},
    {"numpad4",AIR::AIRKey::KEY_NUMPAD4},{"numpad5",AIR::AIRKey::KEY_NUMPAD5},
    {"numpad6",AIR::AIRKey::KEY_NUMPAD6},{"numpad7",AIR::AIRKey::KEY_NUMPAD7},
    {"numpad8",AIR::AIRKey::KEY_NUMPAD8},{"numpad9",AIR::AIRKey::KEY_NUMPAD9},
    {"numpad.",AIR::AIRKey::KEY_NUMPAD_DOT},
    {"numpad+",AIR::AIRKey::KEY_NUMPAD_ADD},{"numpad-",AIR::AIRKey::KEY_NUMPAD_SUB},
    {"numpad*",AIR::AIRKey::KEY_NUMPAD_MUL},{"numpad/",AIR::AIRKey::KEY_NUMPAD_DIV},
    {"numpadenter",AIR::AIRKey::KEY_NUMPAD_ENTER},{"numlock",AIR::AIRKey::KEY_NUMLOCK},
    // 功能键
    {"f1",AIR::AIRKey::KEY_F1}, {"f2",AIR::AIRKey::KEY_F2}, {"f3",AIR::AIRKey::KEY_F3},
    {"f4",AIR::AIRKey::KEY_F4}, {"f5",AIR::AIRKey::KEY_F5}, {"f6",AIR::AIRKey::KEY_F6},
    {"f7",AIR::AIRKey::KEY_F7}, {"f8",AIR::AIRKey::KEY_F8}, {"f9",AIR::AIRKey::KEY_F9},
    {"f10",AIR::AIRKey::KEY_F10},{"f11",AIR::AIRKey::KEY_F11},{"f12",AIR::AIRKey::KEY_F12},
    // 修饰键（罗技用 l/r 前缀）
    {"lctrl",AIR::AIRKey::KEY_LCTRL}, {"rctrl",AIR::AIRKey::KEY_RCTRL},
    {"lshift",AIR::AIRKey::KEY_LSHIFT},{"rshift",AIR::AIRKey::KEY_RSHIFT},
    {"lalt",AIR::AIRKey::KEY_LALT},   {"ralt",AIR::AIRKey::KEY_RALT},
    {"lwin",AIR::AIRKey::KEY_LWIN},   {"rwin",AIR::AIRKey::KEY_RWIN},
    // 导航
    {"up",AIR::AIRKey::KEY_UP},   {"down",AIR::AIRKey::KEY_DOWN},
    {"left",AIR::AIRKey::KEY_LEFT},{"right",AIR::AIRKey::KEY_RIGHT},
    {"home",AIR::AIRKey::KEY_HOME},{"end",AIR::AIRKey::KEY_END},
    {"pageup",AIR::AIRKey::KEY_PGUP},  {"pagedown",AIR::AIRKey::KEY_PGDN},
    {"insert",AIR::AIRKey::KEY_INSERT},{"delete",AIR::AIRKey::KEY_DELETE},
    // 编辑
    {"return",AIR::AIRKey::KEY_ENTER},    {"backspace",AIR::AIRKey::KEY_BACKSPACE},
    {"tab",AIR::AIRKey::KEY_TAB},          {"space",AIR::AIRKey::KEY_SPACE},
    {"escape",AIR::AIRKey::KEY_ESCAPE},    {"capslock",AIR::AIRKey::KEY_CAPSLOCK},
    {"scrolllock",AIR::AIRKey::KEY_SCROLLLOCK},{"pause",AIR::AIRKey::KEY_PAUSE},
    {"printscreen",AIR::AIRKey::KEY_PRINTSCREEN},
    // 符号
    {"`",AIR::AIRKey::KEY_BACKTICK},{"-",AIR::AIRKey::KEY_MINUS},
    {"=",AIR::AIRKey::KEY_EQUALS}, {"[",AIR::AIRKey::KEY_LBRACKET},
    {"]",AIR::AIRKey::KEY_RBRACKET},{"\\",AIR::AIRKey::KEY_BACKSLASH},
    {";",AIR::AIRKey::KEY_SEMICOLON},{"'",AIR::AIRKey::KEY_QUOTE},
    {",",AIR::AIRKey::KEY_COMMA},  {".",AIR::AIRKey::KEY_DOT},
    {"/",AIR::AIRKey::KEY_SLASH},
};

// 反向表：AIRKey → 罗技键名
static const std::pair<AIR::AIRKey,const char*> kRev[] = {
    {AIR::AIRKey::KEY_A,"a"},{AIR::AIRKey::KEY_B,"b"},{AIR::AIRKey::KEY_C,"c"},
    {AIR::AIRKey::KEY_D,"d"},{AIR::AIRKey::KEY_E,"e"},{AIR::AIRKey::KEY_F,"f"},
    {AIR::AIRKey::KEY_G,"g"},{AIR::AIRKey::KEY_H,"h"},{AIR::AIRKey::KEY_I,"i"},
    {AIR::AIRKey::KEY_J,"j"},{AIR::AIRKey::KEY_K,"k"},{AIR::AIRKey::KEY_L,"l"},
    {AIR::AIRKey::KEY_M,"m"},{AIR::AIRKey::KEY_N,"n"},{AIR::AIRKey::KEY_O,"o"},
    {AIR::AIRKey::KEY_P,"p"},{AIR::AIRKey::KEY_Q,"q"},{AIR::AIRKey::KEY_R,"r"},
    {AIR::AIRKey::KEY_S,"s"},{AIR::AIRKey::KEY_T,"t"},{AIR::AIRKey::KEY_U,"u"},
    {AIR::AIRKey::KEY_V,"v"},{AIR::AIRKey::KEY_W,"w"},{AIR::AIRKey::KEY_X,"x"},
    {AIR::AIRKey::KEY_Y,"y"},{AIR::AIRKey::KEY_Z,"z"},
    {AIR::AIRKey::KEY_0,"0"},{AIR::AIRKey::KEY_1,"1"},{AIR::AIRKey::KEY_2,"2"},
    {AIR::AIRKey::KEY_3,"3"},{AIR::AIRKey::KEY_4,"4"},{AIR::AIRKey::KEY_5,"5"},
    {AIR::AIRKey::KEY_6,"6"},{AIR::AIRKey::KEY_7,"7"},{AIR::AIRKey::KEY_8,"8"},
    {AIR::AIRKey::KEY_9,"9"},
    {AIR::AIRKey::KEY_NUMPAD0,"numpad0"},{AIR::AIRKey::KEY_NUMPAD1,"numpad1"},
    {AIR::AIRKey::KEY_NUMPAD2,"numpad2"},{AIR::AIRKey::KEY_NUMPAD3,"numpad3"},
    {AIR::AIRKey::KEY_NUMPAD4,"numpad4"},{AIR::AIRKey::KEY_NUMPAD5,"numpad5"},
    {AIR::AIRKey::KEY_NUMPAD6,"numpad6"},{AIR::AIRKey::KEY_NUMPAD7,"numpad7"},
    {AIR::AIRKey::KEY_NUMPAD8,"numpad8"},{AIR::AIRKey::KEY_NUMPAD9,"numpad9"},
    {AIR::AIRKey::KEY_NUMPAD_DOT,"numpad."},{AIR::AIRKey::KEY_NUMPAD_ADD,"numpad+"},
    {AIR::AIRKey::KEY_NUMPAD_SUB,"numpad-"},{AIR::AIRKey::KEY_NUMPAD_MUL,"numpad*"},
    {AIR::AIRKey::KEY_NUMPAD_DIV,"numpad/"},{AIR::AIRKey::KEY_NUMPAD_ENTER,"numpadenter"},
    {AIR::AIRKey::KEY_NUMLOCK,"numlock"},
    {AIR::AIRKey::KEY_F1,"f1"},{AIR::AIRKey::KEY_F2,"f2"},{AIR::AIRKey::KEY_F3,"f3"},
    {AIR::AIRKey::KEY_F4,"f4"},{AIR::AIRKey::KEY_F5,"f5"},{AIR::AIRKey::KEY_F6,"f6"},
    {AIR::AIRKey::KEY_F7,"f7"},{AIR::AIRKey::KEY_F8,"f8"},{AIR::AIRKey::KEY_F9,"f9"},
    {AIR::AIRKey::KEY_F10,"f10"},{AIR::AIRKey::KEY_F11,"f11"},{AIR::AIRKey::KEY_F12,"f12"},
    {AIR::AIRKey::KEY_CTRL,"lctrl"},{AIR::AIRKey::KEY_LCTRL,"lctrl"},
    {AIR::AIRKey::KEY_RCTRL,"rctrl"},{AIR::AIRKey::KEY_SHIFT,"lshift"},
    {AIR::AIRKey::KEY_LSHIFT,"lshift"},{AIR::AIRKey::KEY_RSHIFT,"rshift"},
    {AIR::AIRKey::KEY_ALT,"lalt"},{AIR::AIRKey::KEY_LALT,"lalt"},
    {AIR::AIRKey::KEY_RALT,"ralt"},{AIR::AIRKey::KEY_WIN,"lwin"},
    {AIR::AIRKey::KEY_LWIN,"lwin"},{AIR::AIRKey::KEY_RWIN,"rwin"},
    {AIR::AIRKey::KEY_UP,"up"},{AIR::AIRKey::KEY_DOWN,"down"},
    {AIR::AIRKey::KEY_LEFT,"left"},{AIR::AIRKey::KEY_RIGHT,"right"},
    {AIR::AIRKey::KEY_HOME,"home"},{AIR::AIRKey::KEY_END,"end"},
    {AIR::AIRKey::KEY_PGUP,"pageup"},{AIR::AIRKey::KEY_PGDN,"pagedown"},
    {AIR::AIRKey::KEY_INSERT,"insert"},{AIR::AIRKey::KEY_DELETE,"delete"},
    {AIR::AIRKey::KEY_ENTER,"return"},{AIR::AIRKey::KEY_BACKSPACE,"backspace"},
    {AIR::AIRKey::KEY_TAB,"tab"},{AIR::AIRKey::KEY_SPACE,"space"},
    {AIR::AIRKey::KEY_ESCAPE,"escape"},{AIR::AIRKey::KEY_CAPSLOCK,"capslock"},
    {AIR::AIRKey::KEY_SCROLLLOCK,"scrolllock"},{AIR::AIRKey::KEY_PAUSE,"pause"},
    {AIR::AIRKey::KEY_PRINTSCREEN,"printscreen"},
    {AIR::AIRKey::KEY_BACKTICK,"`"},{AIR::AIRKey::KEY_MINUS,"-"},
    {AIR::AIRKey::KEY_EQUALS,"="},{AIR::AIRKey::KEY_LBRACKET,"["},
    {AIR::AIRKey::KEY_RBRACKET,"]"},{AIR::AIRKey::KEY_BACKSLASH,"\\"},
    {AIR::AIRKey::KEY_SEMICOLON,";"},{AIR::AIRKey::KEY_QUOTE,"'"},
    {AIR::AIRKey::KEY_COMMA,","},{AIR::AIRKey::KEY_DOT,"."},{AIR::AIRKey::KEY_SLASH,"/"},
};

static const std::unordered_map<std::string,AIR::AIRKey>& fwdMap(){
    static auto m=[](){std::unordered_map<std::string,AIR::AIRKey> t;t.reserve(std::size(kFwd));for(auto& e:kFwd)t.emplace(e.lua,e.key);return t;}();return m;}
static const std::unordered_map<AIR::AIRKey,std::string>& revMap(){
    static auto m=[](){std::unordered_map<AIR::AIRKey,std::string> t;t.reserve(std::size(kRev));for(auto&[k,v]:kRev)t.emplace(k,v);return t;}();return m;}

AIR::AIRKey LuaKeyToAIR(const std::string& k){
    std::string lo=k; std::transform(lo.begin(),lo.end(),lo.begin(),[](unsigned char c){return (char)std::tolower(c);});
    auto it=fwdMap().find(lo); return it!=fwdMap().end()?it->second:AIR::AIRKey::KEY_UNKNOWN;}

std::string AIRKeyToLua(AIR::AIRKey key){
    if(key==AIR::AIRKey::KEY_UNKNOWN) return {};
    auto it=revMap().find(key); return it!=revMap().end()?it->second:std::string{};}

AIR::AIRKey LuaMouseBtnToAIR(int btn){
    switch(btn){
    case 1: return AIR::AIRKey::KEY_MOUSE_LEFT;
    case 2: return AIR::AIRKey::KEY_MOUSE_RIGHT;
    case 3: return AIR::AIRKey::KEY_MOUSE_MIDDLE;
    case 4: return AIR::AIRKey::KEY_MOUSE_X1;
    case 5: return AIR::AIRKey::KEY_MOUSE_X2;
    default:return AIR::AIRKey::KEY_MOUSE_LEFT;}
}

int AIRKeyToLuaMouseBtn(AIR::AIRKey key){
    switch(key){
    case AIR::AIRKey::KEY_MOUSE_LEFT:   return 1;
    case AIR::AIRKey::KEY_MOUSE_RIGHT:  return 2;
    case AIR::AIRKey::KEY_MOUSE_MIDDLE: return 3;
    case AIR::AIRKey::KEY_MOUSE_X1:     return 4;
    case AIR::AIRKey::KEY_MOUSE_X2:     return 5;
    default:                             return 0;}
}

} // namespace LogitechKeyMap
