#include "game_hooks.hpp"

bool setupDone = false;

HMODULE GetCurrentModule() {
  HMODULE hModule = NULL;
  GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    (LPCTSTR)GetCurrentModule, &hModule);

  return hModule;
}

template <typename T> T *offset_from_base(void *struct_ptr, int addr) {
  return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(struct_ptr) + addr);
}

std::vector<std::string> explode(std::string &string, char separator) {
  std::stringstream segmentstream(string);
  std::string segmented;
  std::vector<std::string> splitlist;

  while (std::getline(segmentstream, segmented, separator)) {
    splitlist.push_back(segmented);
  }

  return splitlist;
}

void (__thiscall *PlayLayer_levelComplete_O)(void *);
void __fastcall PlayLayer_levelComplete_H(void *self) {
  auto do_record_actions = *offset_from_base<bool>(self, 0x429);
  auto playback_mode = *offset_from_base<bool>(self, 0x470);

  if (do_record_actions && !playback_mode) {
    auto record_string = *offset_from_base<std::string>(self, 0x430);

    if (!record_string.empty()) {
        auto current_level = *offset_from_base<GJGameLevel *>(self, 0x488);        
        auto compressed_string = cocos2d::ZipUtils::compressString(record_string, false, 0xB);

        current_level->recordString = compressed_string;
    }
  }
    
  return PlayLayer_levelComplete_O(self);
}

// vectorcall passes floats as a parameter, thus we use it here
void (__vectorcall *PlayLayer_updateReplay_O)(void *, float);
void __fastcall PlayLayer_updateReplay_H(void *self) {    
  // but then we extract it manually (fun!)
  float real_timeOffset = 0.0;
  __asm
  {
    MOVSS real_timeOffset, XMM1
  }

  auto verified_chk = offset_from_base<bool>(self, 0x534);
  *verified_chk = true;
  
  return PlayLayer_updateReplay_O(self, real_timeOffset);
}

// a "recreation" of setupReplay found on android devices
void PlayLayer_setupReplay(cocos2d::CCLayer *self, std::string replay) {
    auto playback_mode = offset_from_base<bool>(self, 0x470);
    *playback_mode = true;

    // this has to be false. it should be false by default.
    auto disable_replay_clicks = offset_from_base<bool>(self, 0x471);
    *disable_replay_clicks = false;

    auto replay_inputs = offset_from_base<cocos2d::CCArray *>(self, 0x448);

    // we don't know if there is a replay loaded or not, assume not
    auto new_inputs = cocos2d::CCArray::create();
    
    auto parsed_replay_data = explode(replay, ';');
    
    for (const auto& parsed_data : parsed_replay_data)
    {
        auto string_action = cocos2d::CCString::create(parsed_data);
        new_inputs->addObject(string_action);
    }

    new_inputs->retain();

    *replay_inputs = new_inputs;
}

bool (__thiscall *PlayLayer_init_O)(cocos2d::CCLayer *, GJGameLevel *);
bool __fastcall PlayLayer_init_H(cocos2d::CCLayer *self, void * _edx, GJGameLevel *level) {
  auto status = PlayLayer_init_O(self, level);
  
  if (status) {
    if (!level->recordString.empty()) {
        auto decompressed_string = cocos2d::ZipUtils::decompressString(level->recordString, false, 0xB);
        
        PlayLayer_setupReplay(self, decompressed_string);
        
        // "safety"
        auto label = cocos2d::CCLabelBMFont::create("Playback", "bigFont.fnt");
        self->addChild(label, 50);
        
        label->setScale(0.25f);
        label->setOpacity(80);
        
        label->setPosition(22.0f, 8.0f);

        auto in_testmode = offset_from_base<bool>(self, 0x494);
        *in_testmode = true;
    } else {
        auto do_record_actions = offset_from_base<bool>(self, 0x429);
        *do_record_actions = true;
    }
    
    auto playback_mode = offset_from_base<bool>(self, 0x470);
    auto disable_replay_clicks = offset_from_base<bool>(self, 0x471);
    auto do_record_actions = offset_from_base<bool>(self, 0x429);
  }

  return status;
}

// no need to export this, not putting in .h
struct game_hook {
  void *orig_addr;
  void *hook_fn;
  void **orig_fn;
};

#define CREATE_HOOK(ADDRESS, NAME)                                             \
  {                                                                            \
    reinterpret_cast<void *>(ADDRESS), reinterpret_cast<void *>(&(NAME##_H)),  \
        reinterpret_cast<void **>(&(NAME##_O))                                 \
  }

#define CREATE_GD_HOOK(ADDRESS, NAME)                                          \
  CREATE_HOOK(offset_from_base<void>(gd_handle, ADDRESS), NAME)

void doTheHook() {
  if (auto status = MH_Initialize(); status != MH_OK) {
    return;
  }

  HMODULE gd_handle = GetModuleHandleA("GeometryDash.exe");
  if (!gd_handle) {
    return;
  }

  // wall of hooks
  std::array<game_hook, 3> hooks{{
      CREATE_GD_HOOK(0x20AF40, PlayLayer_updateReplay),
      CREATE_GD_HOOK(0x1FB780, PlayLayer_init),
      CREATE_GD_HOOK(0x1FD3D0, PlayLayer_levelComplete),
  }};

  for (const auto &hook : hooks) {
    MH_CreateHook(hook.orig_addr, hook.hook_fn, hook.orig_fn);
    MH_EnableHook(hook.orig_addr);
  }
}
