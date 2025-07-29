// SPDX-License-Identifier: GPL-2.0-only

cfg_input_configurations g_cfg_input_configs;
std::string g_input_config_override;

extern void report_fatal_error(std::string_view _text, bool is_html = false, bool include_help_text = true)
{
    aps3e_log.fatal("%s",_text.data());
    LOGE("%s",_text.data());
}

bool is_input_allowed(){
    return true;
}
extern void check_microphone_permissions()
{
}
#ifndef __ANDROID__
void enable_display_sleep()
{
}

void disable_display_sleep()
{
}
#endif
extern void qt_events_aware_op(int repeat_duration_ms, std::function<bool()> wrapped_op)
{
    ensure(wrapped_op);

    {
        while (!wrapped_op())
        {
            if (repeat_duration_ms == 0)
            {
                std::this_thread::yield();
            }
            else if (thread_ctrl::get_current())
            {
                thread_ctrl::wait_for(repeat_duration_ms * 1000);
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(repeat_duration_ms));
            }
        }
    }
}

extern std::shared_ptr<CPUDisAsm> make_disasm(const cpu_thread* cpu, shared_ptr<cpu_thread> handle)
{
    if (!handle)
    {
        switch (cpu->get_class())
        {
            case thread_class::ppu: handle = idm::get_unlocked<named_thread<ppu_thread>>(cpu->id); break;
            case thread_class::spu: handle = idm::get_unlocked<named_thread<spu_thread>>(cpu->id); break;
            default: break;
        }
    }

    std::shared_ptr<CPUDisAsm> result;

    switch (cpu->get_class())
    {
        case thread_class::ppu: result = std::make_shared<PPUDisAsm>(cpu_disasm_mode::interpreter, vm::g_sudo_addr); break;
        case thread_class::spu: result = std::make_shared<SPUDisAsm>(cpu_disasm_mode::interpreter, static_cast<const spu_thread*>(cpu)->ls); break;
        case thread_class::rsx: result = std::make_shared<RSXDisAsm>(cpu_disasm_mode::interpreter, vm::g_sudo_addr, 0, cpu); break;
        default: return result;
    }

    result->set_cpu_handle(std::move(handle));
    return result;
}

template <>
void fmt_class_string<cheat_type>::format(std::string& out, u64 arg)
{
    format_enum(out, arg, [](cheat_type value)
    {
        switch (value)
        {
            case cheat_type::unsigned_8_cheat: return "Unsigned 8 bits";
            case cheat_type::unsigned_16_cheat: return "Unsigned 16 bits";
            case cheat_type::unsigned_32_cheat: return "Unsigned 32 bits";
            case cheat_type::unsigned_64_cheat: return "Unsigned 64 bits";
            case cheat_type::signed_8_cheat: return "Signed 8 bits";
            case cheat_type::signed_16_cheat: return "Signed 16 bits";
            case cheat_type::signed_32_cheat: return "Signed 32 bits";
            case cheat_type::signed_64_cheat: return "Signed 64 bits";
            case cheat_type::max: break;
        }

        return unknown;
    });
}

template <>
void fmt_class_string<std::chrono::sys_time<typename std::chrono::system_clock::duration>>::format(std::string& out, u64 arg)
{
PR;
//const std::time_t dateTime = std::chrono::system_clock::to_time_t(get_object(arg));
//out += date_time::fmt_time("%Y-%m-%dT%H:%M:%S", dateTime);
}

class android_gs_frame:public GSFrameBase
{
public:
    ANativeWindow*& wnd;
    int& width;
    int& height;

    android_gs_frame(ANativeWindow*& wnd,int& w,int& h):wnd(wnd),width(w),height(h){PR;
        g_fxo->need<utils::video_provider>();
    }
    ~android_gs_frame(){PR;}

    void close(){PR;}
    void reset(){PR;}
    bool shown(){PR;return false;}
    void hide(){PR;}
    void show(){PR;}
    void toggle_fullscreen(){PR;}

    void delete_context(draw_context_t ctx){PR;}
    draw_context_t make_context(){PR;return nullptr;}
    void set_current(draw_context_t ctx){PR;}
    void flip(draw_context_t ctx, bool skip_frame = false){/*PR;*/}
    int client_width(){/*PR;*/return this->width;}
    int client_height(){/*PR;*/return this->height;}
    f64 client_display_rate(){PR;return 60.0;}
    bool has_alpha(){PR;return true;}

    display_handle_t handle() const{PR;return reinterpret_cast<void*>(this->wnd);}

    bool can_consume_frame() const
    {PR;
        utils::video_provider& video_provider = g_fxo->get<utils::video_provider>();
        return video_provider.can_consume_frame();
    }

    void present_frame(std::vector<u8>& data, u32 pitch, u32 width, u32 height, bool is_bgra) const
    {PR;
        utils::video_provider& video_provider = g_fxo->get<utils::video_provider>();
        video_provider.present_frame(data, pitch, width, height, is_bgra);
    }

    void take_screenshot(std::vector<u8>&& sshot_data, u32 sshot_width, u32 sshot_height, bool is_bgra) {PR;}
    //void take_screenshot(const std::vector<u8> sshot_data, u32 sshot_width, u32 sshot_height, bool is_bgra){PR;}
};

class android_music_handler:public music_handler_base
{
public:
    android_music_handler(){PR;}
    ~android_music_handler(){PR;}

    void stop(){PR;m_state = CELL_MUSIC_PB_STATUS_STOP;}
    void pause(){PR;m_state = CELL_MUSIC_PB_STATUS_PAUSE;}
    void play(const std::string& path){PR;m_state = CELL_MUSIC_PB_STATUS_PLAY;}
    void fast_forward(const std::string& path){PR;m_state = CELL_MUSIC_PB_STATUS_FASTFORWARD;}
    void fast_reverse(const std::string& path){PR;m_state = CELL_MUSIC_PB_STATUS_FASTREVERSE;}
    void set_volume(f32 volume){PR;}
    f32 get_volume() const{PR;return 0;}
};

static const char* localized_string_keys[]={
#define WRAP(x) #x
#include "Emu/localized_string_id.inc"
#undef WRAP
};
static std::string localized_strings[sizeof(localized_string_keys)/sizeof(localized_string_keys[0])];
__attribute__((constructor)) static void init_localized_strings()
{

    constexpr int n=sizeof(localized_string_keys)/sizeof(localized_string_keys[0]);
    for(int i=0;i<n;i++){
        const char* key_id = localized_string_keys[i];
        localized_strings[i]= getenv(key_id)?:"?";
    }
}

class android_save_dialog:public SaveDialogBase{
public:
    s32 ShowSaveDataList(std::vector<SaveDataEntry>& save_entries, s32 focused, u32 op, vm::ptr<CellSaveDataListSet> listSet, bool enable_overlay) override
    {
        LOGI("ShowSaveDataList(save_entries=%d, focused=%d, op=0x%x, listSet=*0x%x, enable_overlay=%d)", save_entries.size(), focused, op, listSet, enable_overlay);

        // TODO: Implement proper error checking in savedata_op?
        const bool use_end = sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_BEGIN, 0) >= 0;

        if (!use_end)
        {
            LOGE("ShowSaveDataList(): Not able to notify DRAWING_BEGIN callback because one has already been sent!");
        }

        // TODO: Install native shell as an Emu callback
        if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
        {
            LOGI("ShowSaveDataList: Showing native UI dialog");

            const s32 result = manager->create<rsx::overlays::save_dialog>()->show(save_entries, focused, op, listSet, enable_overlay);
            if (result != rsx::overlays::user_interface::selection_code::error)
            {
                LOGI("ShowSaveDataList: Native UI dialog returned with selection %d", result);
                if (use_end) sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_END, 0);
                return result;
            }
            LOGE("ShowSaveDataList: Native UI dialog returned error");
        }

        //if (!Emu.HasGui())
        {
            LOGI("ShowSaveDataList(): Aborting: Emulation has no GUI attached");
            if (use_end) sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_END, 0);
            return -2;
        }

        // Fall back to front-end GUI
        /*cellSaveData.notice("ShowSaveDataList(): Using fallback GUI");
        atomic_t<s32> selection = 0;

        input::SetIntercepted(true);

        Emu.BlockingCallFromMainThread([&]()
                                       {
                                           save_data_list_dialog sdid(save_entries, focused, op, listSet);
                                           sdid.exec();
                                           selection = sdid.GetSelection();
                                       });

        input::SetIntercepted(false);

        if (use_end) sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_END, 0);

        return selection.load();*/
    }
    ~android_save_dialog(){}
};

class android_trophy_notification:public TrophyNotificationBase
{
public:
    s32 ShowTrophyNotification(const SceNpTrophyDetails& trophy, const std::vector<uchar>& trophy_icon_buffer) override
    {
        if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
        {
            // Allow adding more than one trophy notification. The notification class manages scheduling
            auto popup = std::make_shared<rsx::overlays::trophy_notification>();
            return manager->add(popup, false)->show(trophy, trophy_icon_buffer);
        }

        //if (!Emu.HasGui())
        {
            return 0;
        }
    }
    ~android_trophy_notification(){}
};

class dummy_msg_dialog:public MsgDialogBase
{
public:

    dummy_msg_dialog()=default;
    ~dummy_msg_dialog(){}

    void Create(const std::string& msg, const std::string& title) override
    {
        PRE("dummy_msg_dialog::Create");
        state = MsgDialogState::Open;
        Close(true);
        if(type.button_type.unshifted()==CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO)
            on_close(CELL_MSGDIALOG_BUTTON_YES);
        else if(type.button_type.unshifted()==CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK)
            on_close(CELL_MSGDIALOG_BUTTON_OK);

    }
    void Close(bool success) override
    {
        PRE("dummy_msg_dialog::Close");
    }
    void SetMsg(const std::string& msg) override
    {
        PRE("dummy_msg_dialog::SetMsg");
    }
    void ProgressBarSetMsg(u32 progressBarIndex, const std::string& msg) override
    {
        PRE("dummy_msg_dialog::ProgressBarSetMsg");
    }
    void ProgressBarReset(u32 progressBarIndex) override
    {
        PRE("dummy_msg_dialog::ProgressBarReset");
    }
    void ProgressBarInc(u32 progressBarIndex, u32 delta) override
    {
        PRE("dummy_msg_dialog::ProgressBarInc");
    }
    void ProgressBarSetValue(u32 progressBarIndex, u32 value) override
    {
        PRE("dummy_msg_dialog::ProgressBarSetValue");
    }
    void ProgressBarSetLimit(u32 index, u32 limit) override
    {
        PRE("dummy_msg_dialog::ProgressBarSetLimit");
    }
};

class dummy_osk_dialog:public OskDialogBase{
public:

    dummy_osk_dialog()=default;
    ~dummy_osk_dialog(){}

    void Create(const OskDialogBase::osk_params& params) override
    {
        PRE("dummy_osk_dialog::Create");
        state = OskDialogState::Open;
        //FIXME
        on_osk_close(CELL_OSKDIALOG_CLOSE_CANCEL);
    }
    void Clear(bool clear_all_data) override
    {
        PRE("dummy_osk_dialog::Clear");
    }
    void Insert(const std::u16string& text) override
    {
        PRE("dummy_osk_dialog::Insert");
    }
    void SetText(const std::u16string& text) override
    {
        PRE("dummy_osk_dialog::SetText");
    }
    void Close(s32 status) override
    {
        PRE("dummy_osk_dialog::Close");
    }
};

class dummy_send_message_dialog:public SendMessageDialogBase
{
public:
    error_code Exec(message_data& msg_data, std::set<std::string>& npids) override
    {
        PRE("dummy_send_message_dialog::Exec");
        return CELL_CANCEL;
    }
    void callback_handler(rpcn::NotificationType ntype, const std::string& username, bool status) override
    {
        PRE("dummy_send_message_dialog::callback_handler");
    }
};

class dummy_recv_message_dialog:public RecvMessageDialogBase
{
public:
    error_code Exec(SceNpBasicMessageMainType type, SceNpBasicMessageRecvOptions options, SceNpBasicMessageRecvAction& recv_result, u64& chosen_msg_id) override
    {
        PRE("dummy_recv_message_dialog::Exec");
        return CELL_CANCEL;
    }
    void callback_handler(const shared_ptr<std::pair<std::string, message_data>> new_msg, u64 msg_id) override
    {
        PRE("dummy_recv_message_dialog::callback_handler");
    }
};


extern std::string rp3_get_config_dir(){
    return std::string(getenv("APS3E_DATA_DIR"))+"/config/";
}

extern std::string rp3_get_cache_dir(){
    return std::string (getenv("APS3E_DATA_DIR"))+"/cache/";
}

extern VkPhysicalDeviceLimits get_physical_device_limits(){
    static const VkPhysicalDeviceLimits r=[]{
        VkApplicationInfo appinfo = {};
        appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appinfo.pNext = nullptr;
        appinfo.pApplicationName = "aps3e-cfg-test";
        appinfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appinfo.pEngineName = "nul";
        appinfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appinfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo inst_create_info = {};
        inst_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        inst_create_info.pApplicationInfo = &appinfo;

        VkInstance inst;
        if (vkCreateInstance(&inst_create_info, nullptr, &inst)!= VK_SUCCESS) {
            __android_log_print(ANDROID_LOG_FATAL, LOG_TAG,"%s : %d",__func__,__LINE__);
            aps3e_log.fatal("%s : %d",__func__,__LINE__);
        }

        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(inst, &physicalDeviceCount, nullptr);
        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        vkEnumeratePhysicalDevices(inst, &physicalDeviceCount, physicalDevices.data());

        assert(physicalDeviceCount==1);

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevices[0], &deviceProperties);

        vkDestroyInstance(inst, nullptr);

        return deviceProperties.limits;

    }();

    return r;
}

extern bool vk_limit_max_vertex_output_components_le_64(){
    return get_physical_device_limits().maxVertexOutputComponents<=64;
}

extern bool cfg_vertex_buffer_upload_mode_use_buffer_view(){
    static const bool r=[]{
        switch(g_cfg.video.vertex_buffer_upload_mode){
            case vertex_buffer_upload_mode::buffer_view:
                return true;
            case vertex_buffer_upload_mode::buffer:
                return false;
            case vertex_buffer_upload_mode::_auto:
                return get_physical_device_limits().maxTexelBufferElements>=64*1024*1024;//>=64M
        }
    }();
    return r;
}

extern const std::unordered_map<rsx::overlays::language_class,std::string>& cfg_font_files(){

    static const auto r=[]->std::unordered_map<rsx::overlays::language_class,std::string>{
        case_lab:
        switch(g_cfg.misc.font_file_selection) {
            case font_file_selection::from_firmware:
                return {
                        {rsx::overlays::language_class::default_, "SCE-PS3-VR-R-LATIN.TTF"},
                        {rsx::overlays::language_class::cjk_base, "SCE-PS3-SR-R-JPN.TTF"},
                        {rsx::overlays::language_class::hangul,   "SCE-PS3-YG-R-KOR.TTF"}
                };

            case font_file_selection::from_os:
                return {
                        {rsx::overlays::language_class::default_, "/system/fonts/Roboto-Regular.ttf"},
                        {rsx::overlays::language_class::cjk_base, "/system/fonts/NotoSansCJK-Regular.ttc"},
                        {rsx::overlays::language_class::hangul,   "/system/fonts/NotoSansCJK-Regular.ttc"}
                };
            case font_file_selection::custom:
                std::string custom_font_file_path = g_cfg.misc.custom_font_file_path.to_string();
                if (!custom_font_file_path.empty() &&
                    std::filesystem::exists(custom_font_file_path))
                    return {
                            {rsx::overlays::language_class::default_, custom_font_file_path},
                            {rsx::overlays::language_class::cjk_base, custom_font_file_path},
                            {rsx::overlays::language_class::hangul,   custom_font_file_path}
                    };
                else {
                    //Android 15+
                    g_cfg.misc.font_file_selection.set(
                            atoi(getenv("APS3E_ANDROID_API_VERSION")) >= 35
                            ? font_file_selection::from_firmware : font_file_selection::from_os);
                    goto case_lab;
                }
        };

        return {};
    }();

    return r;
}