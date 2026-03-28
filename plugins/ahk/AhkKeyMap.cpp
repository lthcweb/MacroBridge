/*
 * AhkKeyMap.cpp  —  AIRKey ↔ AHK v2 键名映射实现
 * AHK v2 键名参考：https://www.autohotkey.com/docs/v2/KeyList.htm
 */
#include "AhkKeyMap.h"
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace AhkKeyMap {

    struct Entry { const char* ahk; AIR::AIRKey key; };

    // 正向表（AHK小写名 → AIRKey），含常见别名
    static const Entry kFwd[] = {
        {"lbutton",AIR::AIRKey::KEY_MOUSE_LEFT},  {"rbutton",AIR::AIRKey::KEY_MOUSE_RIGHT},
        {"mbutton",AIR::AIRKey::KEY_MOUSE_MIDDLE},{"xbutton1",AIR::AIRKey::KEY_MOUSE_X1},
        {"xbutton2",AIR::AIRKey::KEY_MOUSE_X2},
        {"a",AIR::AIRKey::KEY_A},{"b",AIR::AIRKey::KEY_B},{"c",AIR::AIRKey::KEY_C},
        {"d",AIR::AIRKey::KEY_D},{"e",AIR::AIRKey::KEY_E},{"f",AIR::AIRKey::KEY_F},
        {"g",AIR::AIRKey::KEY_G},{"h",AIR::AIRKey::KEY_H},{"i",AIR::AIRKey::KEY_I},
        {"j",AIR::AIRKey::KEY_J},{"k",AIR::AIRKey::KEY_K},{"l",AIR::AIRKey::KEY_L},
        {"m",AIR::AIRKey::KEY_M},{"n",AIR::AIRKey::KEY_N},{"o",AIR::AIRKey::KEY_O},
        {"p",AIR::AIRKey::KEY_P},{"q",AIR::AIRKey::KEY_Q},{"r",AIR::AIRKey::KEY_R},
        {"s",AIR::AIRKey::KEY_S},{"t",AIR::AIRKey::KEY_T},{"u",AIR::AIRKey::KEY_U},
        {"v",AIR::AIRKey::KEY_V},{"w",AIR::AIRKey::KEY_W},{"x",AIR::AIRKey::KEY_X},
        {"y",AIR::AIRKey::KEY_Y},{"z",AIR::AIRKey::KEY_Z},
        {"0",AIR::AIRKey::KEY_0},{"1",AIR::AIRKey::KEY_1},{"2",AIR::AIRKey::KEY_2},
        {"3",AIR::AIRKey::KEY_3},{"4",AIR::AIRKey::KEY_4},{"5",AIR::AIRKey::KEY_5},
        {"6",AIR::AIRKey::KEY_6},{"7",AIR::AIRKey::KEY_7},{"8",AIR::AIRKey::KEY_8},
        {"9",AIR::AIRKey::KEY_9},
        {"numpad0",AIR::AIRKey::KEY_NUMPAD0},{"numpad1",AIR::AIRKey::KEY_NUMPAD1},
        {"numpad2",AIR::AIRKey::KEY_NUMPAD2},{"numpad3",AIR::AIRKey::KEY_NUMPAD3},
        {"numpad4",AIR::AIRKey::KEY_NUMPAD4},{"numpad5",AIR::AIRKey::KEY_NUMPAD5},
        {"numpad6",AIR::AIRKey::KEY_NUMPAD6},{"numpad7",AIR::AIRKey::KEY_NUMPAD7},
        {"numpad8",AIR::AIRKey::KEY_NUMPAD8},{"numpad9",AIR::AIRKey::KEY_NUMPAD9},
        {"numpaddot",AIR::AIRKey::KEY_NUMPAD_DOT},  {"numpaddel",AIR::AIRKey::KEY_NUMPAD_DOT},
        {"numpadadd",AIR::AIRKey::KEY_NUMPAD_ADD},  {"numpadsub",AIR::AIRKey::KEY_NUMPAD_SUB},
        {"numpadmult",AIR::AIRKey::KEY_NUMPAD_MUL}, {"numpaddiv",AIR::AIRKey::KEY_NUMPAD_DIV},
        {"numpadenter",AIR::AIRKey::KEY_NUMPAD_ENTER},{"numlock",AIR::AIRKey::KEY_NUMLOCK},
        {"f1",AIR::AIRKey::KEY_F1},  {"f2",AIR::AIRKey::KEY_F2},  {"f3",AIR::AIRKey::KEY_F3},
        {"f4",AIR::AIRKey::KEY_F4},  {"f5",AIR::AIRKey::KEY_F5},  {"f6",AIR::AIRKey::KEY_F6},
        {"f7",AIR::AIRKey::KEY_F7},  {"f8",AIR::AIRKey::KEY_F8},  {"f9",AIR::AIRKey::KEY_F9},
        {"f10",AIR::AIRKey::KEY_F10},{"f11",AIR::AIRKey::KEY_F11},{"f12",AIR::AIRKey::KEY_F12},
        {"f13",AIR::AIRKey::KEY_F13},{"f14",AIR::AIRKey::KEY_F14},{"f15",AIR::AIRKey::KEY_F15},
        {"f16",AIR::AIRKey::KEY_F16},{"f17",AIR::AIRKey::KEY_F17},{"f18",AIR::AIRKey::KEY_F18},
        {"f19",AIR::AIRKey::KEY_F19},{"f20",AIR::AIRKey::KEY_F20},{"f21",AIR::AIRKey::KEY_F21},
        {"f22",AIR::AIRKey::KEY_F22},{"f23",AIR::AIRKey::KEY_F23},{"f24",AIR::AIRKey::KEY_F24},
        {"ctrl",AIR::AIRKey::KEY_CTRL},    {"control",AIR::AIRKey::KEY_CTRL},
        {"lctrl",AIR::AIRKey::KEY_LCTRL}, {"lcontrol",AIR::AIRKey::KEY_LCTRL},
        {"rctrl",AIR::AIRKey::KEY_RCTRL}, {"rcontrol",AIR::AIRKey::KEY_RCTRL},
        {"shift",AIR::AIRKey::KEY_SHIFT},  {"lshift",AIR::AIRKey::KEY_LSHIFT},
        {"rshift",AIR::AIRKey::KEY_RSHIFT},
        {"alt",AIR::AIRKey::KEY_ALT},{"lalt",AIR::AIRKey::KEY_LALT},{"ralt",AIR::AIRKey::KEY_RALT},
        {"lwin",AIR::AIRKey::KEY_LWIN},{"rwin",AIR::AIRKey::KEY_RWIN},
        {"up",AIR::AIRKey::KEY_UP},{"down",AIR::AIRKey::KEY_DOWN},
        {"left",AIR::AIRKey::KEY_LEFT},{"right",AIR::AIRKey::KEY_RIGHT},
        {"home",AIR::AIRKey::KEY_HOME},{"end",AIR::AIRKey::KEY_END},
        {"pgup",AIR::AIRKey::KEY_PGUP},  {"pgdn",AIR::AIRKey::KEY_PGDN},
        {"ins",AIR::AIRKey::KEY_INSERT}, {"insert",AIR::AIRKey::KEY_INSERT},
        {"del",AIR::AIRKey::KEY_DELETE}, {"delete",AIR::AIRKey::KEY_DELETE},
        {"enter",AIR::AIRKey::KEY_ENTER},{"return",AIR::AIRKey::KEY_ENTER},
        {"backspace",AIR::AIRKey::KEY_BACKSPACE},{"bs",AIR::AIRKey::KEY_BACKSPACE},
        {"tab",AIR::AIRKey::KEY_TAB},{"space",AIR::AIRKey::KEY_SPACE},
        {"esc",AIR::AIRKey::KEY_ESCAPE},{"escape",AIR::AIRKey::KEY_ESCAPE},
        {"capslock",AIR::AIRKey::KEY_CAPSLOCK},{"scrolllock",AIR::AIRKey::KEY_SCROLLLOCK},
        {"pause",AIR::AIRKey::KEY_PAUSE},{"printscreen",AIR::AIRKey::KEY_PRINTSCREEN},
        {"`",AIR::AIRKey::KEY_BACKTICK},{"-",AIR::AIRKey::KEY_MINUS},
        {"=",AIR::AIRKey::KEY_EQUALS},{"[",AIR::AIRKey::KEY_LBRACKET},
        {"]",AIR::AIRKey::KEY_RBRACKET},{"\\",AIR::AIRKey::KEY_BACKSLASH},
        {";",AIR::AIRKey::KEY_SEMICOLON},{"'",AIR::AIRKey::KEY_QUOTE},
        {",",AIR::AIRKey::KEY_COMMA},{".",AIR::AIRKey::KEY_DOT},{"/",AIR::AIRKey::KEY_SLASH},
        {"media_play_pause",AIR::AIRKey::KEY_MEDIA_PLAY_PAUSE},
        {"media_stop",AIR::AIRKey::KEY_MEDIA_STOP},
        {"media_next",AIR::AIRKey::KEY_MEDIA_NEXT},
        {"media_prev",AIR::AIRKey::KEY_MEDIA_PREV},
        {"volume_up",AIR::AIRKey::KEY_VOLUME_UP},
        {"volume_down",AIR::AIRKey::KEY_VOLUME_DOWN},
        {"volume_mute",AIR::AIRKey::KEY_VOLUME_MUTE},
    };

    // 反向表（AIRKey → AHK规范名）
    static const std::pair<AIR::AIRKey, const char*> kRev[] = {
        {AIR::AIRKey::KEY_MOUSE_LEFT,"LButton"},{AIR::AIRKey::KEY_MOUSE_RIGHT,"RButton"},
        {AIR::AIRKey::KEY_MOUSE_MIDDLE,"MButton"},{AIR::AIRKey::KEY_MOUSE_X1,"XButton1"},
        {AIR::AIRKey::KEY_MOUSE_X2,"XButton2"},
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
        {AIR::AIRKey::KEY_NUMPAD0,"Numpad0"},{AIR::AIRKey::KEY_NUMPAD1,"Numpad1"},
        {AIR::AIRKey::KEY_NUMPAD2,"Numpad2"},{AIR::AIRKey::KEY_NUMPAD3,"Numpad3"},
        {AIR::AIRKey::KEY_NUMPAD4,"Numpad4"},{AIR::AIRKey::KEY_NUMPAD5,"Numpad5"},
        {AIR::AIRKey::KEY_NUMPAD6,"Numpad6"},{AIR::AIRKey::KEY_NUMPAD7,"Numpad7"},
        {AIR::AIRKey::KEY_NUMPAD8,"Numpad8"},{AIR::AIRKey::KEY_NUMPAD9,"Numpad9"},
        {AIR::AIRKey::KEY_NUMPAD_DOT,"NumpadDot"},{AIR::AIRKey::KEY_NUMPAD_ADD,"NumpadAdd"},
        {AIR::AIRKey::KEY_NUMPAD_SUB,"NumpadSub"},{AIR::AIRKey::KEY_NUMPAD_MUL,"NumpadMult"},
        {AIR::AIRKey::KEY_NUMPAD_DIV,"NumpadDiv"},{AIR::AIRKey::KEY_NUMPAD_ENTER,"NumpadEnter"},
        {AIR::AIRKey::KEY_NUMLOCK,"NumLock"},
        {AIR::AIRKey::KEY_F1,"F1"},{AIR::AIRKey::KEY_F2,"F2"},{AIR::AIRKey::KEY_F3,"F3"},
        {AIR::AIRKey::KEY_F4,"F4"},{AIR::AIRKey::KEY_F5,"F5"},{AIR::AIRKey::KEY_F6,"F6"},
        {AIR::AIRKey::KEY_F7,"F7"},{AIR::AIRKey::KEY_F8,"F8"},{AIR::AIRKey::KEY_F9,"F9"},
        {AIR::AIRKey::KEY_F10,"F10"},{AIR::AIRKey::KEY_F11,"F11"},{AIR::AIRKey::KEY_F12,"F12"},
        {AIR::AIRKey::KEY_F13,"F13"},{AIR::AIRKey::KEY_F14,"F14"},{AIR::AIRKey::KEY_F15,"F15"},
        {AIR::AIRKey::KEY_F16,"F16"},{AIR::AIRKey::KEY_F17,"F17"},{AIR::AIRKey::KEY_F18,"F18"},
        {AIR::AIRKey::KEY_F19,"F19"},{AIR::AIRKey::KEY_F20,"F20"},{AIR::AIRKey::KEY_F21,"F21"},
        {AIR::AIRKey::KEY_F22,"F22"},{AIR::AIRKey::KEY_F23,"F23"},{AIR::AIRKey::KEY_F24,"F24"},
        {AIR::AIRKey::KEY_CTRL,"Ctrl"},{AIR::AIRKey::KEY_LCTRL,"LCtrl"},
        {AIR::AIRKey::KEY_RCTRL,"RCtrl"},{AIR::AIRKey::KEY_SHIFT,"Shift"},
        {AIR::AIRKey::KEY_LSHIFT,"LShift"},{AIR::AIRKey::KEY_RSHIFT,"RShift"},
        {AIR::AIRKey::KEY_ALT,"Alt"},{AIR::AIRKey::KEY_LALT,"LAlt"},
        {AIR::AIRKey::KEY_RALT,"RAlt"},{AIR::AIRKey::KEY_WIN,"LWin"},
        {AIR::AIRKey::KEY_LWIN,"LWin"},{AIR::AIRKey::KEY_RWIN,"RWin"},
        {AIR::AIRKey::KEY_UP,"Up"},{AIR::AIRKey::KEY_DOWN,"Down"},
        {AIR::AIRKey::KEY_LEFT,"Left"},{AIR::AIRKey::KEY_RIGHT,"Right"},
        {AIR::AIRKey::KEY_HOME,"Home"},{AIR::AIRKey::KEY_END,"End"},
        {AIR::AIRKey::KEY_PGUP,"PgUp"},{AIR::AIRKey::KEY_PGDN,"PgDn"},
        {AIR::AIRKey::KEY_INSERT,"Ins"},{AIR::AIRKey::KEY_DELETE,"Del"},
        {AIR::AIRKey::KEY_ENTER,"Enter"},{AIR::AIRKey::KEY_BACKSPACE,"BackSpace"},
        {AIR::AIRKey::KEY_TAB,"Tab"},{AIR::AIRKey::KEY_SPACE,"Space"},
        {AIR::AIRKey::KEY_ESCAPE,"Esc"},{AIR::AIRKey::KEY_CAPSLOCK,"CapsLock"},
        {AIR::AIRKey::KEY_SCROLLLOCK,"ScrollLock"},{AIR::AIRKey::KEY_PAUSE,"Pause"},
        {AIR::AIRKey::KEY_PRINTSCREEN,"PrintScreen"},
        {AIR::AIRKey::KEY_BACKTICK,"`"},{AIR::AIRKey::KEY_MINUS,"-"},
        {AIR::AIRKey::KEY_EQUALS,"="},{AIR::AIRKey::KEY_LBRACKET,"["},
        {AIR::AIRKey::KEY_RBRACKET,"]"},{AIR::AIRKey::KEY_BACKSLASH,"\\"},
        {AIR::AIRKey::KEY_SEMICOLON,";"},{AIR::AIRKey::KEY_QUOTE,"'"},
        {AIR::AIRKey::KEY_COMMA,","},{AIR::AIRKey::KEY_DOT,"."},{AIR::AIRKey::KEY_SLASH,"/"},
        {AIR::AIRKey::KEY_MEDIA_PLAY_PAUSE,"Media_Play_Pause"},
        {AIR::AIRKey::KEY_MEDIA_STOP,"Media_Stop"},
        {AIR::AIRKey::KEY_MEDIA_NEXT,"Media_Next"},{AIR::AIRKey::KEY_MEDIA_PREV,"Media_Prev"},
        {AIR::AIRKey::KEY_VOLUME_UP,"Volume_Up"},{AIR::AIRKey::KEY_VOLUME_DOWN,"Volume_Down"},
        {AIR::AIRKey::KEY_VOLUME_MUTE,"Volume_Mute"},
    };

    static const std::unordered_map<std::string, AIR::AIRKey>& fwdMap() {
        static auto m = []() {
            std::unordered_map<std::string, AIR::AIRKey> t; t.reserve(std::size(kFwd));
            for (auto& e : kFwd) t.emplace(e.ahk, e.key); return t; }();
            return m;
    }
    static const std::unordered_map<AIR::AIRKey, std::string>& revMap() {
        static auto m = []() {
            std::unordered_map<AIR::AIRKey, std::string> t; t.reserve(std::size(kRev));
            for (auto& [k, v] : kRev) t.emplace(k, v); return t; }();
            return m;
    }

    AIR::AIRKey AhkToAIR(const std::string& ahkName) {
        std::string lo = ahkName;
        std::transform(lo.begin(), lo.end(), lo.begin(), [](unsigned char c) {return (char)std::tolower(c);});
        auto it = fwdMap().find(lo);
        return it != fwdMap().end() ? it->second : AIR::AIRKey::KEY_UNKNOWN;
    }
    std::string AIRToAhk(AIR::AIRKey key) {
        if (key == AIR::AIRKey::KEY_UNKNOWN) return {};
        auto it = revMap().find(key);
        return it != revMap().end() ? it->second : std::string{};
    }

} // namespace AhkKeyMap