/*
 * RazerKeyMap.cpp  —  雷蛇键名映射实现
 */

#include "RazerKeyMap.h"
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace RazerKeyMap {

// ============================================================================
//  映射表：雷蛇键名（小写）→ AIRKey
//  支持两种格式：
//    1. Windows 虚拟键名：VK_LBUTTON、VK_F1、VK_RETURN ...
//    2. 简短键名：a、f1、enter、lshift ...
// ============================================================================

struct Entry { const char* name; AIR::AIRKey key; };

static const Entry kForward[] = {

    // ── 鼠标按键（VK 格式）──────────────────────────────────────────────────
    { "vk_lbutton",  AIR::AIRKey::KEY_MOUSE_LEFT   },
    { "vk_rbutton",  AIR::AIRKey::KEY_MOUSE_RIGHT  },
    { "vk_mbutton",  AIR::AIRKey::KEY_MOUSE_MIDDLE },
    { "vk_xbutton1", AIR::AIRKey::KEY_MOUSE_X1     },
    { "vk_xbutton2", AIR::AIRKey::KEY_MOUSE_X2     },

    // ── 字母（VK 格式：VK_A ~ VK_Z）────────────────────────────────────────
    { "vk_a", AIR::AIRKey::KEY_A }, { "vk_b", AIR::AIRKey::KEY_B },
    { "vk_c", AIR::AIRKey::KEY_C }, { "vk_d", AIR::AIRKey::KEY_D },
    { "vk_e", AIR::AIRKey::KEY_E }, { "vk_f", AIR::AIRKey::KEY_F },
    { "vk_g", AIR::AIRKey::KEY_G }, { "vk_h", AIR::AIRKey::KEY_H },
    { "vk_i", AIR::AIRKey::KEY_I }, { "vk_j", AIR::AIRKey::KEY_J },
    { "vk_k", AIR::AIRKey::KEY_K }, { "vk_l", AIR::AIRKey::KEY_L },
    { "vk_m", AIR::AIRKey::KEY_M }, { "vk_n", AIR::AIRKey::KEY_N },
    { "vk_o", AIR::AIRKey::KEY_O }, { "vk_p", AIR::AIRKey::KEY_P },
    { "vk_q", AIR::AIRKey::KEY_Q }, { "vk_r", AIR::AIRKey::KEY_R },
    { "vk_s", AIR::AIRKey::KEY_S }, { "vk_t", AIR::AIRKey::KEY_T },
    { "vk_u", AIR::AIRKey::KEY_U }, { "vk_v", AIR::AIRKey::KEY_V },
    { "vk_w", AIR::AIRKey::KEY_W }, { "vk_x", AIR::AIRKey::KEY_X },
    { "vk_y", AIR::AIRKey::KEY_Y }, { "vk_z", AIR::AIRKey::KEY_Z },

    // ── 字母（简短格式）────────────────────────────────────────────────────
    { "a", AIR::AIRKey::KEY_A }, { "b", AIR::AIRKey::KEY_B },
    { "c", AIR::AIRKey::KEY_C }, { "d", AIR::AIRKey::KEY_D },
    { "e", AIR::AIRKey::KEY_E }, { "f", AIR::AIRKey::KEY_F },
    { "g", AIR::AIRKey::KEY_G }, { "h", AIR::AIRKey::KEY_H },
    { "i", AIR::AIRKey::KEY_I }, { "j", AIR::AIRKey::KEY_J },
    { "k", AIR::AIRKey::KEY_K }, { "l", AIR::AIRKey::KEY_L },
    { "m", AIR::AIRKey::KEY_M }, { "n", AIR::AIRKey::KEY_N },
    { "o", AIR::AIRKey::KEY_O }, { "p", AIR::AIRKey::KEY_P },
    { "q", AIR::AIRKey::KEY_Q }, { "r", AIR::AIRKey::KEY_R },
    { "s", AIR::AIRKey::KEY_S }, { "t", AIR::AIRKey::KEY_T },
    { "u", AIR::AIRKey::KEY_U }, { "v", AIR::AIRKey::KEY_V },
    { "w", AIR::AIRKey::KEY_W }, { "x", AIR::AIRKey::KEY_X },
    { "y", AIR::AIRKey::KEY_Y }, { "z", AIR::AIRKey::KEY_Z },

    // ── 数字（VK_0 ~ VK_9）──────────────────────────────────────────────────
    { "vk_0", AIR::AIRKey::KEY_0 }, { "vk_1", AIR::AIRKey::KEY_1 },
    { "vk_2", AIR::AIRKey::KEY_2 }, { "vk_3", AIR::AIRKey::KEY_3 },
    { "vk_4", AIR::AIRKey::KEY_4 }, { "vk_5", AIR::AIRKey::KEY_5 },
    { "vk_6", AIR::AIRKey::KEY_6 }, { "vk_7", AIR::AIRKey::KEY_7 },
    { "vk_8", AIR::AIRKey::KEY_8 }, { "vk_9", AIR::AIRKey::KEY_9 },
    { "0", AIR::AIRKey::KEY_0 }, { "1", AIR::AIRKey::KEY_1 },
    { "2", AIR::AIRKey::KEY_2 }, { "3", AIR::AIRKey::KEY_3 },
    { "4", AIR::AIRKey::KEY_4 }, { "5", AIR::AIRKey::KEY_5 },
    { "6", AIR::AIRKey::KEY_6 }, { "7", AIR::AIRKey::KEY_7 },
    { "8", AIR::AIRKey::KEY_8 }, { "9", AIR::AIRKey::KEY_9 },

    // ── 功能键 ──────────────────────────────────────────────────────────────
    { "vk_f1",  AIR::AIRKey::KEY_F1  }, { "vk_f2",  AIR::AIRKey::KEY_F2  },
    { "vk_f3",  AIR::AIRKey::KEY_F3  }, { "vk_f4",  AIR::AIRKey::KEY_F4  },
    { "vk_f5",  AIR::AIRKey::KEY_F5  }, { "vk_f6",  AIR::AIRKey::KEY_F6  },
    { "vk_f7",  AIR::AIRKey::KEY_F7  }, { "vk_f8",  AIR::AIRKey::KEY_F8  },
    { "vk_f9",  AIR::AIRKey::KEY_F9  }, { "vk_f10", AIR::AIRKey::KEY_F10 },
    { "vk_f11", AIR::AIRKey::KEY_F11 }, { "vk_f12", AIR::AIRKey::KEY_F12 },
    { "vk_f13", AIR::AIRKey::KEY_F13 }, { "vk_f14", AIR::AIRKey::KEY_F14 },
    { "vk_f15", AIR::AIRKey::KEY_F15 }, { "vk_f16", AIR::AIRKey::KEY_F16 },
    { "vk_f17", AIR::AIRKey::KEY_F17 }, { "vk_f18", AIR::AIRKey::KEY_F18 },
    { "vk_f19", AIR::AIRKey::KEY_F19 }, { "vk_f20", AIR::AIRKey::KEY_F20 },
    { "vk_f21", AIR::AIRKey::KEY_F21 }, { "vk_f22", AIR::AIRKey::KEY_F22 },
    { "vk_f23", AIR::AIRKey::KEY_F23 }, { "vk_f24", AIR::AIRKey::KEY_F24 },
    { "f1",  AIR::AIRKey::KEY_F1  }, { "f2",  AIR::AIRKey::KEY_F2  },
    { "f3",  AIR::AIRKey::KEY_F3  }, { "f4",  AIR::AIRKey::KEY_F4  },
    { "f5",  AIR::AIRKey::KEY_F5  }, { "f6",  AIR::AIRKey::KEY_F6  },
    { "f7",  AIR::AIRKey::KEY_F7  }, { "f8",  AIR::AIRKey::KEY_F8  },
    { "f9",  AIR::AIRKey::KEY_F9  }, { "f10", AIR::AIRKey::KEY_F10 },
    { "f11", AIR::AIRKey::KEY_F11 }, { "f12", AIR::AIRKey::KEY_F12 },

    // ── 修饰键 ──────────────────────────────────────────────────────────────
    { "vk_control", AIR::AIRKey::KEY_CTRL   },
    { "vk_lcontrol",AIR::AIRKey::KEY_LCTRL  },
    { "vk_rcontrol",AIR::AIRKey::KEY_RCTRL  },
    { "vk_shift",   AIR::AIRKey::KEY_SHIFT  },
    { "vk_lshift",  AIR::AIRKey::KEY_LSHIFT },
    { "vk_rshift",  AIR::AIRKey::KEY_RSHIFT },
    { "vk_menu",    AIR::AIRKey::KEY_ALT    }, // Windows VK_MENU = Alt
    { "vk_lmenu",   AIR::AIRKey::KEY_LALT   },
    { "vk_rmenu",   AIR::AIRKey::KEY_RALT   },
    { "vk_lwin",    AIR::AIRKey::KEY_LWIN   },
    { "vk_rwin",    AIR::AIRKey::KEY_RWIN   },
    { "lctrl",      AIR::AIRKey::KEY_LCTRL  },
    { "rctrl",      AIR::AIRKey::KEY_RCTRL  },
    { "lshift",     AIR::AIRKey::KEY_LSHIFT },
    { "rshift",     AIR::AIRKey::KEY_RSHIFT },
    { "lalt",       AIR::AIRKey::KEY_LALT   },
    { "ralt",       AIR::AIRKey::KEY_RALT   },

    // ── 编辑键 ──────────────────────────────────────────────────────────────
    { "vk_return",   AIR::AIRKey::KEY_ENTER     },
    { "vk_back",     AIR::AIRKey::KEY_BACKSPACE },
    { "vk_tab",      AIR::AIRKey::KEY_TAB       },
    { "vk_space",    AIR::AIRKey::KEY_SPACE     },
    { "vk_escape",   AIR::AIRKey::KEY_ESCAPE    },
    { "vk_capital",  AIR::AIRKey::KEY_CAPSLOCK  },
    { "vk_scroll",   AIR::AIRKey::KEY_SCROLLLOCK},
    { "vk_pause",    AIR::AIRKey::KEY_PAUSE     },
    { "vk_snapshot", AIR::AIRKey::KEY_PRINTSCREEN},
    { "enter",       AIR::AIRKey::KEY_ENTER     },
    { "backspace",   AIR::AIRKey::KEY_BACKSPACE },
    { "tab",         AIR::AIRKey::KEY_TAB       },
    { "space",       AIR::AIRKey::KEY_SPACE     },
    { "escape",      AIR::AIRKey::KEY_ESCAPE    },
    { "esc",         AIR::AIRKey::KEY_ESCAPE    },
    { "capslock",    AIR::AIRKey::KEY_CAPSLOCK  },

    // ── 导航键 ──────────────────────────────────────────────────────────────
    { "vk_up",    AIR::AIRKey::KEY_UP     },
    { "vk_down",  AIR::AIRKey::KEY_DOWN   },
    { "vk_left",  AIR::AIRKey::KEY_LEFT   },
    { "vk_right", AIR::AIRKey::KEY_RIGHT  },
    { "vk_home",  AIR::AIRKey::KEY_HOME   },
    { "vk_end",   AIR::AIRKey::KEY_END    },
    { "vk_prior", AIR::AIRKey::KEY_PGUP   }, // VK_PRIOR = Page Up
    { "vk_next",  AIR::AIRKey::KEY_PGDN   }, // VK_NEXT  = Page Down
    { "vk_insert",AIR::AIRKey::KEY_INSERT },
    { "vk_delete",AIR::AIRKey::KEY_DELETE },
    { "up",    AIR::AIRKey::KEY_UP     },
    { "down",  AIR::AIRKey::KEY_DOWN   },
    { "left",  AIR::AIRKey::KEY_LEFT   },
    { "right", AIR::AIRKey::KEY_RIGHT  },
    { "home",  AIR::AIRKey::KEY_HOME   },
    { "end",   AIR::AIRKey::KEY_END    },
    { "pgup",  AIR::AIRKey::KEY_PGUP   },
    { "pgdn",  AIR::AIRKey::KEY_PGDN   },
    { "ins",   AIR::AIRKey::KEY_INSERT },
    { "del",   AIR::AIRKey::KEY_DELETE },

    // ── 小键盘 ──────────────────────────────────────────────────────────────
    { "vk_numpad0",   AIR::AIRKey::KEY_NUMPAD0     },
    { "vk_numpad1",   AIR::AIRKey::KEY_NUMPAD1     },
    { "vk_numpad2",   AIR::AIRKey::KEY_NUMPAD2     },
    { "vk_numpad3",   AIR::AIRKey::KEY_NUMPAD3     },
    { "vk_numpad4",   AIR::AIRKey::KEY_NUMPAD4     },
    { "vk_numpad5",   AIR::AIRKey::KEY_NUMPAD5     },
    { "vk_numpad6",   AIR::AIRKey::KEY_NUMPAD6     },
    { "vk_numpad7",   AIR::AIRKey::KEY_NUMPAD7     },
    { "vk_numpad8",   AIR::AIRKey::KEY_NUMPAD8     },
    { "vk_numpad9",   AIR::AIRKey::KEY_NUMPAD9     },
    { "vk_decimal",   AIR::AIRKey::KEY_NUMPAD_DOT  },
    { "vk_add",       AIR::AIRKey::KEY_NUMPAD_ADD  },
    { "vk_subtract",  AIR::AIRKey::KEY_NUMPAD_SUB  },
    { "vk_multiply",  AIR::AIRKey::KEY_NUMPAD_MUL  },
    { "vk_divide",    AIR::AIRKey::KEY_NUMPAD_DIV  },
    { "vk_numlock",   AIR::AIRKey::KEY_NUMLOCK     },

    // ── 媒体键 ──────────────────────────────────────────────────────────────
    { "vk_media_play_pause", AIR::AIRKey::KEY_MEDIA_PLAY_PAUSE },
    { "vk_media_stop",       AIR::AIRKey::KEY_MEDIA_STOP       },
    { "vk_media_next_track", AIR::AIRKey::KEY_MEDIA_NEXT       },
    { "vk_media_prev_track", AIR::AIRKey::KEY_MEDIA_PREV       },
    { "vk_volume_up",        AIR::AIRKey::KEY_VOLUME_UP        },
    { "vk_volume_down",      AIR::AIRKey::KEY_VOLUME_DOWN      },
    { "vk_volume_mute",      AIR::AIRKey::KEY_VOLUME_MUTE      },
};

// 反向表：AIRKey → 雷蛇标准 VK 键名
static const std::pair<AIR::AIRKey, const char*> kReverse[] = {
    { AIR::AIRKey::KEY_MOUSE_LEFT,   "VK_LBUTTON"  },
    { AIR::AIRKey::KEY_MOUSE_RIGHT,  "VK_RBUTTON"  },
    { AIR::AIRKey::KEY_MOUSE_MIDDLE, "VK_MBUTTON"  },
    { AIR::AIRKey::KEY_MOUSE_X1,     "VK_XBUTTON1" },
    { AIR::AIRKey::KEY_MOUSE_X2,     "VK_XBUTTON2" },

    { AIR::AIRKey::KEY_A, "VK_A" }, { AIR::AIRKey::KEY_B, "VK_B" },
    { AIR::AIRKey::KEY_C, "VK_C" }, { AIR::AIRKey::KEY_D, "VK_D" },
    { AIR::AIRKey::KEY_E, "VK_E" }, { AIR::AIRKey::KEY_F, "VK_F" },
    { AIR::AIRKey::KEY_G, "VK_G" }, { AIR::AIRKey::KEY_H, "VK_H" },
    { AIR::AIRKey::KEY_I, "VK_I" }, { AIR::AIRKey::KEY_J, "VK_J" },
    { AIR::AIRKey::KEY_K, "VK_K" }, { AIR::AIRKey::KEY_L, "VK_L" },
    { AIR::AIRKey::KEY_M, "VK_M" }, { AIR::AIRKey::KEY_N, "VK_N" },
    { AIR::AIRKey::KEY_O, "VK_O" }, { AIR::AIRKey::KEY_P, "VK_P" },
    { AIR::AIRKey::KEY_Q, "VK_Q" }, { AIR::AIRKey::KEY_R, "VK_R" },
    { AIR::AIRKey::KEY_S, "VK_S" }, { AIR::AIRKey::KEY_T, "VK_T" },
    { AIR::AIRKey::KEY_U, "VK_U" }, { AIR::AIRKey::KEY_V, "VK_V" },
    { AIR::AIRKey::KEY_W, "VK_W" }, { AIR::AIRKey::KEY_X, "VK_X" },
    { AIR::AIRKey::KEY_Y, "VK_Y" }, { AIR::AIRKey::KEY_Z, "VK_Z" },

    { AIR::AIRKey::KEY_0, "VK_0" }, { AIR::AIRKey::KEY_1, "VK_1" },
    { AIR::AIRKey::KEY_2, "VK_2" }, { AIR::AIRKey::KEY_3, "VK_3" },
    { AIR::AIRKey::KEY_4, "VK_4" }, { AIR::AIRKey::KEY_5, "VK_5" },
    { AIR::AIRKey::KEY_6, "VK_6" }, { AIR::AIRKey::KEY_7, "VK_7" },
    { AIR::AIRKey::KEY_8, "VK_8" }, { AIR::AIRKey::KEY_9, "VK_9" },

    { AIR::AIRKey::KEY_F1,  "VK_F1"  }, { AIR::AIRKey::KEY_F2,  "VK_F2"  },
    { AIR::AIRKey::KEY_F3,  "VK_F3"  }, { AIR::AIRKey::KEY_F4,  "VK_F4"  },
    { AIR::AIRKey::KEY_F5,  "VK_F5"  }, { AIR::AIRKey::KEY_F6,  "VK_F6"  },
    { AIR::AIRKey::KEY_F7,  "VK_F7"  }, { AIR::AIRKey::KEY_F8,  "VK_F8"  },
    { AIR::AIRKey::KEY_F9,  "VK_F9"  }, { AIR::AIRKey::KEY_F10, "VK_F10" },
    { AIR::AIRKey::KEY_F11, "VK_F11" }, { AIR::AIRKey::KEY_F12, "VK_F12" },
    { AIR::AIRKey::KEY_F13, "VK_F13" }, { AIR::AIRKey::KEY_F14, "VK_F14" },
    { AIR::AIRKey::KEY_F15, "VK_F15" }, { AIR::AIRKey::KEY_F16, "VK_F16" },

    { AIR::AIRKey::KEY_CTRL,    "VK_CONTROL" },
    { AIR::AIRKey::KEY_LCTRL,   "VK_LCONTROL"},
    { AIR::AIRKey::KEY_RCTRL,   "VK_RCONTROL"},
    { AIR::AIRKey::KEY_SHIFT,   "VK_SHIFT"   },
    { AIR::AIRKey::KEY_LSHIFT,  "VK_LSHIFT"  },
    { AIR::AIRKey::KEY_RSHIFT,  "VK_RSHIFT"  },
    { AIR::AIRKey::KEY_ALT,     "VK_MENU"    },
    { AIR::AIRKey::KEY_LALT,    "VK_LMENU"   },
    { AIR::AIRKey::KEY_RALT,    "VK_RMENU"   },
    { AIR::AIRKey::KEY_LWIN,    "VK_LWIN"    },
    { AIR::AIRKey::KEY_RWIN,    "VK_RWIN"    },

    { AIR::AIRKey::KEY_ENTER,       "VK_RETURN"   },
    { AIR::AIRKey::KEY_BACKSPACE,   "VK_BACK"     },
    { AIR::AIRKey::KEY_TAB,         "VK_TAB"      },
    { AIR::AIRKey::KEY_SPACE,       "VK_SPACE"    },
    { AIR::AIRKey::KEY_ESCAPE,      "VK_ESCAPE"   },
    { AIR::AIRKey::KEY_CAPSLOCK,    "VK_CAPITAL"  },
    { AIR::AIRKey::KEY_SCROLLLOCK,  "VK_SCROLL"   },
    { AIR::AIRKey::KEY_PAUSE,       "VK_PAUSE"    },
    { AIR::AIRKey::KEY_PRINTSCREEN, "VK_SNAPSHOT" },

    { AIR::AIRKey::KEY_UP,     "VK_UP"     },
    { AIR::AIRKey::KEY_DOWN,   "VK_DOWN"   },
    { AIR::AIRKey::KEY_LEFT,   "VK_LEFT"   },
    { AIR::AIRKey::KEY_RIGHT,  "VK_RIGHT"  },
    { AIR::AIRKey::KEY_HOME,   "VK_HOME"   },
    { AIR::AIRKey::KEY_END,    "VK_END"    },
    { AIR::AIRKey::KEY_PGUP,   "VK_PRIOR"  },
    { AIR::AIRKey::KEY_PGDN,   "VK_NEXT"   },
    { AIR::AIRKey::KEY_INSERT, "VK_INSERT" },
    { AIR::AIRKey::KEY_DELETE, "VK_DELETE" },

    { AIR::AIRKey::KEY_NUMPAD0,      "VK_NUMPAD0"  },
    { AIR::AIRKey::KEY_NUMPAD1,      "VK_NUMPAD1"  },
    { AIR::AIRKey::KEY_NUMPAD2,      "VK_NUMPAD2"  },
    { AIR::AIRKey::KEY_NUMPAD3,      "VK_NUMPAD3"  },
    { AIR::AIRKey::KEY_NUMPAD4,      "VK_NUMPAD4"  },
    { AIR::AIRKey::KEY_NUMPAD5,      "VK_NUMPAD5"  },
    { AIR::AIRKey::KEY_NUMPAD6,      "VK_NUMPAD6"  },
    { AIR::AIRKey::KEY_NUMPAD7,      "VK_NUMPAD7"  },
    { AIR::AIRKey::KEY_NUMPAD8,      "VK_NUMPAD8"  },
    { AIR::AIRKey::KEY_NUMPAD9,      "VK_NUMPAD9"  },
    { AIR::AIRKey::KEY_NUMPAD_DOT,   "VK_DECIMAL"  },
    { AIR::AIRKey::KEY_NUMPAD_ADD,   "VK_ADD"      },
    { AIR::AIRKey::KEY_NUMPAD_SUB,   "VK_SUBTRACT" },
    { AIR::AIRKey::KEY_NUMPAD_MUL,   "VK_MULTIPLY" },
    { AIR::AIRKey::KEY_NUMPAD_DIV,   "VK_DIVIDE"   },
    { AIR::AIRKey::KEY_NUMLOCK,      "VK_NUMLOCK"  },

    { AIR::AIRKey::KEY_MEDIA_PLAY_PAUSE, "VK_MEDIA_PLAY_PAUSE" },
    { AIR::AIRKey::KEY_MEDIA_STOP,       "VK_MEDIA_STOP"       },
    { AIR::AIRKey::KEY_MEDIA_NEXT,       "VK_MEDIA_NEXT_TRACK" },
    { AIR::AIRKey::KEY_MEDIA_PREV,       "VK_MEDIA_PREV_TRACK" },
    { AIR::AIRKey::KEY_VOLUME_UP,        "VK_VOLUME_UP"        },
    { AIR::AIRKey::KEY_VOLUME_DOWN,      "VK_VOLUME_DOWN"      },
    { AIR::AIRKey::KEY_VOLUME_MUTE,      "VK_VOLUME_MUTE"      },
};

// ── 懒加载哈希表 ─────────────────────────────────────────────────────────────

static const std::unordered_map<std::string, AIR::AIRKey>& fwdMap() {
    static auto m = []() {
        std::unordered_map<std::string, AIR::AIRKey> t;
        t.reserve(std::size(kForward));
        for (auto& e : kForward) t.emplace(e.name, e.key);
        return t;
    }();
    return m;
}

static const std::unordered_map<AIR::AIRKey, std::string>& revMap() {
    static auto m = []() {
        std::unordered_map<AIR::AIRKey, std::string> t;
        t.reserve(std::size(kReverse));
        for (auto& [k, v] : kReverse) t.emplace(k, v);
        return t;
    }();
    return m;
}

// ============================================================================
//  公开函数
// ============================================================================

AIR::AIRKey RazerKeyToAIR(const std::string& razerKey)
{
    std::string low = razerKey;
    std::transform(low.begin(), low.end(), low.begin(),
                   [](unsigned char c){ return (char)std::tolower(c); });
    auto it = fwdMap().find(low);
    return (it != fwdMap().end()) ? it->second : AIR::AIRKey::KEY_UNKNOWN;
}

std::string AIRKeyToRazer(AIR::AIRKey key)
{
    auto it = revMap().find(key);
    return (it != revMap().end()) ? it->second : std::string{};
}

AIR::AIRKey RazerMouseButtonToAIR(int btn)
{
    // 雷蛇鼠标按键编号：1=左 2=右 3=中 4=X1 5=X2
    switch (btn) {
    case 1: return AIR::AIRKey::KEY_MOUSE_LEFT;
    case 2: return AIR::AIRKey::KEY_MOUSE_RIGHT;
    case 3: return AIR::AIRKey::KEY_MOUSE_MIDDLE;
    case 4: return AIR::AIRKey::KEY_MOUSE_X1;
    case 5: return AIR::AIRKey::KEY_MOUSE_X2;
    default:return AIR::AIRKey::KEY_MOUSE_LEFT;
    }
}

int AIRKeyToRazerMouseButton(AIR::AIRKey key)
{
    switch (key) {
    case AIR::AIRKey::KEY_MOUSE_LEFT:   return 1;
    case AIR::AIRKey::KEY_MOUSE_RIGHT:  return 2;
    case AIR::AIRKey::KEY_MOUSE_MIDDLE: return 3;
    case AIR::AIRKey::KEY_MOUSE_X1:     return 4;
    case AIR::AIRKey::KEY_MOUSE_X2:     return 5;
    default:                             return 1;
    }
}

} // namespace RazerKeyMap
