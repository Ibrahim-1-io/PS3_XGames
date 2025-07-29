// SPDX-License-Identifier: WTFPL

#include <jni.h>
#include <assert.h>
#include <string.h>
#include <android/log.h>
#include <vector>
#include <android/native_window_jni.h>

#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.h>

#include <linux/prctl.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <thread>
#include <string_view>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include <stb_truetype.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#pragma clang diagnostic ignored "-Wsign-compare"

#include "util/logs.hpp"
#include "util/video_provider.h"

#include "Input/product_info.h"
#include "Input/pad_thread.h"

#include "Utilities/cheat_info.h"
#include "Emu/System.h"
#include "Emu/Io/interception.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/Cell/lv2/sys_usbd.h"

#include "Emu/IdManager.h"

#include "Emu/RSX/RSXDisAsm.h"
#include "Emu/Cell/PPUAnalyser.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUDisAsm.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"

#include "Emu/Io/Null/NullPadHandler.h"

#include "rpcs3_version.h"
#include "Emu/IdManager.h"
#include "Emu/VFS.h"
#include "Emu/vfs_config.h"
#include "Emu/system_utils.hpp"
#include "Emu/system_config.h"

#include "Emu/system_progress.hpp"
#include "Emu/Cell/Modules/cellGem.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"
#include "Emu/Cell/Modules/cellOskDialog.h"
#include "Emu/Cell/Modules/sceNp.h"

#include "Crypto/unpkg.h"
#include "Crypto/unself.h"
#include "Crypto/unzip.h"
#include "Crypto/decrypt_binaries.h"


#include "Loader/iso.h"
#include "Loader/PUP.h"
#include "Loader/TAR.h"
#include "Loader/PSF.h"
#include "Loader/mself.hpp"

#include "Utilities/File.h"
#include "Utilities/Thread.h"
#include "util/sysinfo.hpp"
#include "util/serialization_ext.hpp"

#include "Emu/Io/PadHandler.h"

#include "Emu/Io/Null/null_music_handler.h"
#include "Emu/Io/Null/null_camera_handler.h"
#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "Emu/Io/Null/NullMouseHandler.h"
#include "Emu/Io/KeyboardHandler.h"
#include "Emu/Io/MouseHandler.h"

#include "Emu/Audio/Null/NullAudioBackend.h"
#include "Emu/Audio/Cubeb/CubebBackend.h"
#include "Emu/Audio/Null/null_enumerator.h"
#include "Emu/Audio/Cubeb/cubeb_enumerator.h"

#include "Emu/RSX/GSFrameBase.h"

#include "Emu/Io/music_handler_base.h"

#include "Input/raw_mouse_handler.h"

#include "Emu/RSX/VK/VKGSRender.h"

#include "Input/sdl_pad_handler.h"

#include "Emu/Cell/Modules/cellSaveData.h"
#include "Emu/Cell/Modules/sceNpTrophy.h"

#include "Emu/Cell/Modules/cellMusic.h"

#include "Emu/RSX/Overlays/HomeMenu/overlay_home_menu.h"
#include "Emu/RSX/Overlays/overlay_message.h"

#include "yaml-cpp/yaml.h"

#include "Emu/RSX/Overlays/overlay_save_dialog.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "Emu/RSX/Overlays/overlay_trophy_notification.h"
#include "Emu/RSX/Overlays/overlay_fonts.h"

#include "cpuinfo.h"
#include "meminfo.h"

#define LOG_TAG "aps3e_native"

#if 1

#define LOGI(...) {}
#define LOGW(...) {}
#define LOGE(...) {}
#define PR {}
#define PRE(f) {}

#else

#define LOGI(...) {      \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG,"%s : %d",__func__,__LINE__);\
	__android_log_print(ANDROID_LOG_INFO, LOG_TAG,__VA_ARGS__);\
}

#define LOGW(...) {      \
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG,"%s : %d",__func__,__LINE__);\
	__android_log_print(ANDROID_LOG_WARN, LOG_TAG,__VA_ARGS__);\
}

#define LOGE(...) {      \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,"%s : %d",__func__,__LINE__);\
	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG,__VA_ARGS__);\
}

#define PR {__android_log_print(ANDROID_LOG_WARN, LOG_TAG,"%s : %d",__func__,__LINE__);aps3e_log.notice("%s : %d",__func__,__LINE__);}
#define PRE(f) {__android_log_print(ANDROID_LOG_WARN, LOG_TAG,"EmuCallbacks::%s : %d",f,__LINE__);aps3e_log.notice("EmuCallbacks::%s : %d",f,__LINE__);}

#endif

LOG_CHANNEL(aps3e_log);
LOG_CHANNEL(sys_log, "SYS");

#include "aps3e_key_handler.cpp"

#include "aps3e_pad_thread.cpp"

#include "pt.cpp"

#include "aps3e_rp3_impl.cpp"

#include "aps3e_emu.cpp"

#include "aps3e_util.cpp"

static void j_setup_game_info(JNIEnv* env,jobject self,jobject meta_info ){

        jclass clsMetaInfo = env->FindClass("aenu/aps3e/Emulator$MetaInfo");
        jfieldID f_eboot = env->GetFieldID(clsMetaInfo, "eboot_path", "Ljava/lang/String;");
        jfieldID f_iso_fd  = env->GetFieldID(clsMetaInfo, "iso_fd", "I");
        jfieldID f_id = env->GetFieldID(clsMetaInfo, "serial", "Ljava/lang/String;");
        jstring f_eboot_v = (jstring) env->GetObjectField(meta_info, f_eboot );
        jstring f_id_v = (jstring) env->GetObjectField(meta_info, f_id);

    const char *id = env->GetStringUTFChars(f_id_v, nullptr);

    aps3e_emu::game_id = std::string(id);

        if(f_eboot_v!=NULL) {
            const char *eboot = env->GetStringUTFChars(f_eboot_v, nullptr);
            aps3e_emu::eboot_path = std::string(eboot);

            env->ReleaseStringUTFChars(f_eboot_v, eboot);
        }else{
            aps3e_emu::iso_fd = env->GetIntField(meta_info, f_iso_fd);
        }

    env->ReleaseStringUTFChars(f_id_v, id);

}

static void j_boot(JNIEnv* env,jobject self){
	LOGW("f_init_pre  ");
	aps3e_log.notice("f_init_pre  ");

	LOGW("ANativeWindow %d %d",aps3e_emu::w,aps3e_emu::h);

	std::thread yy(aps3e_emu::g_emu_thr);
	yy.detach();

	LOGW("f_init_end");
	aps3e_log.notice("f_init_end");

}

static void j_change_surface(JNIEnv* env,jobject self,jint w,jint h){

    aps3e_emu::w=w;
    aps3e_emu::h=h;
}

static void j_setup_surface(JNIEnv* env,jobject self,jobject surface){

    if(aps3e_emu::wnd){
    ANativeWindow_release(aps3e_emu::wnd);
        aps3e_emu::wnd=nullptr;}
    if(surface) {
        aps3e_emu::wnd=ANativeWindow_fromSurface(env,surface);
        aps3e_emu::w=ANativeWindow_getWidth(aps3e_emu::wnd);
        aps3e_emu::h=ANativeWindow_getHeight(aps3e_emu::wnd);
    }
}

static void j_key_event(JNIEnv* env,jobject self,jint key_code,jboolean pressed,jint value){
	pthread_mutex_lock(&aps3e_emu::key_event_mutex);
	{
		auto* pad_thr=g_fxo->try_get<named_thread<pad_thread>>();
        if(pad_thr){
            auto xx=pad_thr->get_handlers().at(pad_handler::keyboard);
            std::shared_ptr<AndroidVirtualPadHandler> padh=std::dynamic_pointer_cast<AndroidVirtualPadHandler>(xx);
            padh->Key(static_cast<u32>(key_code), static_cast<bool>(pressed),value);
        }
	}
	pthread_mutex_unlock(&aps3e_emu::key_event_mutex);
}

static void j_pause(JNIEnv* env,jobject self){

    pthread_mutex_lock(&aps3e_emu::emu_mutex);
    aps3e_emu::emu_status=aps3e_emu::EmuThr::STATUS_REQUEST_PAUSE;
	while(aps3e_emu::emu_status==aps3e_emu::EmuThr::STATUS_REQUEST_PAUSE)
        pthread_cond_wait(&aps3e_emu::emu_cond,&aps3e_emu::emu_mutex);
    pthread_mutex_unlock(&aps3e_emu::emu_mutex);
}

static void j_resume(JNIEnv* env,jobject self){
    pthread_mutex_lock(&aps3e_emu::emu_mutex);
    aps3e_emu::emu_status=aps3e_emu::EmuThr::STATUS_REQUEST_RESUME;
    while(aps3e_emu::emu_status==aps3e_emu::EmuThr::STATUS_REQUEST_RESUME)
        pthread_cond_wait(&aps3e_emu::emu_cond,&aps3e_emu::emu_mutex);
    pthread_mutex_unlock(&aps3e_emu::emu_mutex);
}

static jboolean j_is_running(JNIEnv* env,jobject self){
    return aps3e_emu::emu_status==aps3e_emu::EmuThr::STATUS_RUNNING;
}

static jboolean j_is_paused(JNIEnv* env,jobject self){
    return aps3e_emu::emu_status==aps3e_emu::EmuThr::STATUS_PAUSED;
}

static void j_quit(JNIEnv* env,jobject self){
    pthread_mutex_lock(&aps3e_emu::emu_mutex);
    aps3e_emu::emu_status=aps3e_emu::EmuThr::STATUS_REQUEST_STOP;
    while(aps3e_emu::emu_status==aps3e_emu::EmuThr::STATUS_REQUEST_STOP)
        pthread_cond_wait(&aps3e_emu::emu_cond,&aps3e_emu::emu_mutex);
    pthread_mutex_unlock(&aps3e_emu::emu_mutex);
}

/*
static jboolean j_install_firmware(JNIEnv* env,jobject self,jstring pup_path){
	jboolean is_copy=false;
	const char* path=env->GetStringUTFChars(pup_path,&is_copy);
	jboolean result= aps3e_util::install_firmware(path);
    env->ReleaseStringUTFChars(pup_path,path);
    return result;
}*/
static jboolean j_install_firmware(JNIEnv* env,jobject self,jint pup_fd){
    //jboolean is_copy=false;
    //const char* path=env->GetStringUTFChars(pup_path,&is_copy);
    jboolean result= aps3e_util::install_firmware(pup_fd);
    //env->ReleaseStringUTFChars(pup_path,path);
    return result;
}

/*
static jboolean j_install_pkg(JNIEnv* env,jobject self,jstring pkg_path){
	jboolean is_copy=false;
	const char* path=env->GetStringUTFChars(pkg_path,&is_copy);
    jboolean result= aps3e_util::install_pkg(path);
    env->ReleaseStringUTFChars(pkg_path,path);
	return result;
}*/

static jboolean j_install_pkg(JNIEnv* env,jobject self,jint pkg_fd){
    //jboolean is_copy=false;
    //const char* path=env->GetStringUTFChars(pkg_path,&is_copy);
    jboolean result= aps3e_util::install_pkg(pkg_fd);
    //env->ReleaseStringUTFChars(pkg_path,path);
    return result;
}

static jobject j_meta_info_from_dir(JNIEnv* env,jobject self,jstring jdir_path){

    auto fetch_psf_path=[](const std::string& dir_path){
        std::string sub_paths[]={
                "/PARAM.SFO","/PS3_GAME/PARAM.SFO"
        };
        for(std::string& sub_path:sub_paths){
            std::string psf_path=dir_path+sub_path;
            if(std::filesystem::exists(psf_path))
                return psf_path;
        }
        return std::string{};
    };

auto fetch_eboot_path=[](const std::string& dir_path){
    std::string sub_paths[]=
            {
                    "/EBOOT.BIN",
                    "/USRDIR/EBOOT.BIN",
                    "/USRDIR/ISO.BIN.EDAT",
                    "/PS3_GAME/USRDIR/EBOOT.BIN",
            };
    for(std::string& sub_path:sub_paths){
        std::string eboot_path=dir_path+sub_path;
        if(std::filesystem::exists(eboot_path))
            return eboot_path;
    }
    return std::string{};
};

    jboolean is_copy=false;
    const char* _dir_path=env->GetStringUTFChars(jdir_path,&is_copy);
    const std::string dir_path=_dir_path;
    env->ReleaseStringUTFChars(jdir_path, _dir_path);

    psf::registry psf=psf::load_object(fetch_psf_path(dir_path));
#if 1
    for(const auto& [key,value]:psf){
        switch (value.type()) {
            case psf::format::array:

            LOGW("key %s is array!",key.c_str());
                break;
            case psf::format::string:
            LOGW("key %s svalue %s",key.c_str(),value.as_string().c_str());
                break;
            case psf::format::integer:
            LOGW("key %s ivalue %d",key.c_str(),value.as_integer());
                break;
        }
    }
#endif
    jclass cls_MetaInfo=env->FindClass("aenu/aps3e/Emulator$MetaInfo");
    jmethodID mid_MetaInfo_ctor=env->GetMethodID(cls_MetaInfo,"<init>","()V");
    jfieldID fid_MetaInfo_eboot_path=env->GetFieldID(cls_MetaInfo,"eboot_path","Ljava/lang/String;");
    jfieldID fid_MetaInfo_name=env->GetFieldID(cls_MetaInfo,"name","Ljava/lang/String;");
    jfieldID fid_MetaInfo_serial=env->GetFieldID(cls_MetaInfo,"serial","Ljava/lang/String;");
    jfieldID fid_MetaInfo_category=env->GetFieldID(cls_MetaInfo,"category","Ljava/lang/String;");
    jfieldID fid_MetaInfo_version=env->GetFieldID(cls_MetaInfo,"version","Ljava/lang/String;");
    jfieldID fid_MetaInfo_decrypt=env->GetFieldID(cls_MetaInfo,"decrypt","Z");
    jfieldID  fid_MetaInfo_resolution=env->GetFieldID(cls_MetaInfo,"resolution","I");
    jfieldID  fid_MetaInfo_sound_format=env->GetFieldID(cls_MetaInfo,"sound_format","I");

    jobject meta_info=env->NewObject(cls_MetaInfo,mid_MetaInfo_ctor);

    std::string v;
    env->SetObjectField(meta_info,fid_MetaInfo_eboot_path,env->NewStringUTF((v=fetch_eboot_path(dir_path)).c_str()));
    env->SetBooleanField(meta_info,fid_MetaInfo_decrypt,aps3e_util::allow_eboot_decrypt(fs::file(v)));
    env->SetObjectField(meta_info,fid_MetaInfo_name,env->NewStringUTF((v=psf::get_string(psf,"TITLE","")).c_str()));
    env->SetObjectField(meta_info,fid_MetaInfo_serial,env->NewStringUTF((v=psf::get_string(psf,"TITLE_ID","")).c_str()));
    env->SetObjectField(meta_info,fid_MetaInfo_category,env->NewStringUTF((v=psf::get_string(psf,"CATEGORY","??")).c_str()));
    env->SetObjectField(meta_info,fid_MetaInfo_version,env->NewStringUTF((v=psf::get_string(psf,"APP_VER","")).c_str()));
    env->SetIntField(meta_info,fid_MetaInfo_resolution,psf::get_integer(psf,"RESOLUTION",0));
    env->SetIntField(meta_info,fid_MetaInfo_sound_format,psf::get_integer(psf,"SOUND_FORMAT",0));
    return meta_info;
}


static jobject j_meta_info_from_iso(JNIEnv* env,jobject self,jint fd,jstring jiso_uri_path){

    jclass cls_MetaInfo=env->FindClass("aenu/aps3e/Emulator$MetaInfo");
    jmethodID mid_MetaInfo_ctor=env->GetMethodID(cls_MetaInfo,"<init>","()V");
    jfieldID fid_MetaInfo_iso_uri=env->GetFieldID(cls_MetaInfo,"iso_uri","Ljava/lang/String;");
    jfieldID fid_MetaInfo_name=env->GetFieldID(cls_MetaInfo,"name","Ljava/lang/String;");
    jfieldID fid_MetaInfo_serial=env->GetFieldID(cls_MetaInfo,"serial","Ljava/lang/String;");
    jfieldID fid_MetaInfo_category=env->GetFieldID(cls_MetaInfo,"category","Ljava/lang/String;");
    jfieldID fid_MetaInfo_version=env->GetFieldID(cls_MetaInfo,"version","Ljava/lang/String;");
    jfieldID fid_MetaInfo_icon=env->GetFieldID(cls_MetaInfo,"icon","[B");
    jfieldID fid_MetaInfo_decrypt=env->GetFieldID(cls_MetaInfo,"decrypt","Z");
    jfieldID  fid_MetaInfo_resolution=env->GetFieldID(cls_MetaInfo,"resolution","I");
    jfieldID  fid_MetaInfo_sound_format=env->GetFieldID(cls_MetaInfo,"sound_format","I");

    std::unique_ptr<iso_fs> iso=iso_fs::from_fd(fd);
    if(!iso->load()) {
        LOGW("Failed to load iso");
        return NULL;
    }

    if(!iso->exists(":PS3_GAME/USRDIR/EBOOT.BIN")) {
        LOGW("EBOOT.BIN not found")
        return NULL;
    }

    std::vector<uint8_t> psf_data=iso->get_data_tiny(":PS3_GAME/PARAM.SFO");
    if(psf_data.empty()) {
        LOGW("Failed to load PARAM.SFO");
        return NULL;
    }

    psf::registry psf=psf::load_object(fs::file(psf_data.data(),psf_data.size()),"PS3_GAME/PARAM.SFO"sv);

    jobject meta_info=env->NewObject(cls_MetaInfo,mid_MetaInfo_ctor);
    env->SetObjectField(meta_info,fid_MetaInfo_iso_uri,jiso_uri_path);
    std::string v;
    env->SetObjectField(meta_info,fid_MetaInfo_name,env->NewStringUTF((v=psf::get_string(psf,"TITLE","")).c_str()));
    env->SetObjectField(meta_info,fid_MetaInfo_serial,env->NewStringUTF((v=psf::get_string(psf,"TITLE_ID","")).c_str()));
    env->SetObjectField(meta_info,fid_MetaInfo_category,env->NewStringUTF((v=psf::get_string(psf,"CATEGORY","??")).c_str()));
    env->SetObjectField(meta_info,fid_MetaInfo_version,env->NewStringUTF((v=psf::get_string(psf,"APP_VER","")).c_str()));
    env->SetIntField(meta_info,fid_MetaInfo_resolution,psf::get_integer(psf,"RESOLUTION",0));
    env->SetIntField(meta_info,fid_MetaInfo_sound_format,psf::get_integer(psf,"SOUND_FORMAT",0));

    std::vector<uint8_t> icon_data=iso->get_data_tiny(":PS3_GAME/ICON0.PNG");
    if(!icon_data.empty()) {
        jbyteArray icon_array=env->NewByteArray(icon_data.size());
        env->SetByteArrayRegion(icon_array,0,icon_data.size(),reinterpret_cast<const jbyte*>(icon_data.data()));
        env->SetObjectField(meta_info,fid_MetaInfo_icon,icon_array);
    }

    env->SetBooleanField(meta_info,fid_MetaInfo_decrypt,aps3e_util::allow_eboot_decrypt(fs::file(*iso,":PS3_GAME/USRDIR/EBOOT.BIN")));

    return meta_info;
}

/*public native int get_cpu_core_count();
public native String get_cpu_name(int core_idx);
public native int get_cpu_max_mhz(int core_idx);*/
static jint j_get_cpu_core_count(JNIEnv* env,jobject self){
    return cpu_get_core_count();
}
static jstring j_get_cpu_name(JNIEnv* env,jobject self,jint core_idx){
    const std::string& name=cpu_get_processor_name(core_idx);
    jstring r= env->NewStringUTF(name.c_str());
    return r;
}
static jint j_get_cpu_max_mhz(JNIEnv* env,jobject self,jint core_idx){
    return cpu_get_max_mhz(core_idx);
}
#include "aps3e_config.cpp"

/*
static jboolean support_custom_driver(JNIEnv* env,jobject self){
    return access("/dev/kgsl-3d0",F_OK)==0;
}*/

int register_Emulator(JNIEnv* env){

	static const JNINativeMethod methods[] = {

            {"get_cpu_core_count", "()I",(void *)j_get_cpu_core_count},
            {"get_cpu_name", "(I)Ljava/lang/String;",(void *)j_get_cpu_name},
            {"get_cpu_max_mhz", "(I)I",(void *)j_get_cpu_max_mhz},
                    {"simple_device_info", "()Ljava/lang/String;",(void *)j_simple_device_info},
                    {"get_native_llvm_cpu_list", "()[Ljava/lang/String;",(void *)j_get_native_llvm_cpu_list},
            {"get_support_llvm_cpu_list", "()[Ljava/lang/String;",(void *)j_get_support_llvm_cpu_list},
            {"get_vulkan_physical_dev_list", "()[Ljava/lang/String;",(void *)j_get_vulkan_physical_dev_list},

            //{"support_custom_driver","()Z",(void *)support_custom_driver},

            { "generate_config_xml", "()Ljava/lang/String;", (void *) generate_config_xml },
            {"generate_strings_xml","()Ljava/lang/String;", (void *)generate_strings_xml},
            {"generate_java_string_arr","()Ljava/lang/String;", (void *)generate_java_string_arr},

        { "install_firmware", "(I)Z", (void *) j_install_firmware },
		//{ "meta_info_from_iso","(Ljava/lang/String;)Laenu/aps3e/Emulator$MetaInfo;",(void*)MetaInfo_from_iso},
		{ "meta_info_from_dir","(Ljava/lang/String;)Laenu/aps3e/Emulator$MetaInfo;",(void*)j_meta_info_from_dir},

            { "meta_info_from_iso","(ILjava/lang/String;)Laenu/aps3e/Emulator$MetaInfo;",(void*)j_meta_info_from_iso},

        //{ "setup_context", "(Landroid/content/Context;)V", (void *) j_setup_context },

        { "setup_game_info", "(Laenu/aps3e/Emulator$MetaInfo;)V", (void *) j_setup_game_info },
		{ "boot", "()V", (void *) j_boot },
		{ "pause", "()V", (void *) j_pause},
		{ "resume", "()V", (void *) j_resume},

        { "is_running", "()Z", (void *) j_is_running},
        { "is_paused", "()Z", (void *) j_is_paused},

		{ "quit", "()V", (void *) j_quit},
		{ "key_event", "(IZI)V", (void *) j_key_event},
		{ "install_pkg", "(I)Z", (void *) j_install_pkg },

        { "change_surface", "(II)V", (void *) j_change_surface },
        { "setup_surface", "(Landroid/view/Surface;)V", (void *) j_setup_surface },
    };

    jclass clazz = env->FindClass("aenu/aps3e/Emulator");
    return env->RegisterNatives(clazz,methods, sizeof(methods)/sizeof(methods[0]));

}

int register_Emulator_cfg(JNIEnv* env){

    static const JNINativeMethod methods[] = {

            { "native_open_config", "(Ljava/lang/String;)J", (void *) open_config_str },
            { "native_close_config", "(J)Ljava/lang/String;", (void *) close_config_str },
            { "native_open_config_file", "(Ljava/lang/String;)J", (void *) open_config_file },
            { "native_load_config_entry", "(JLjava/lang/String;)Ljava/lang/String;", (void *) load_config_entry },
            { "native_load_config_entry_ty_arr", "(JLjava/lang/String;)[Ljava/lang/String;", (void *) load_config_entry_ty_arr },
            { "native_save_config_entry", "(JLjava/lang/String;Ljava/lang/String;)V", (void *) save_config_entry },
            { "native_save_config_entry_ty_arr", "(JLjava/lang/String;[Ljava/lang/String;)V", (void *) save_config_entry_ty_arr },
            { "native_close_config_file", "(JLjava/lang/String;)V", (void *) close_config_file },
    };
    jclass clazz = env->FindClass("aenu/aps3e/Emulator$Config");
    return env->RegisterNatives(clazz,methods, sizeof(methods)/sizeof(methods[0]));
}

//esr ctx
  static const esr_context* find_esr_context(const ucontext_t* ctx)
    {
        u32 offset = 0;
        const auto& mctx = ctx->uc_mcontext;

        while ((offset + 4) < sizeof(mctx.__reserved))
        {
            const auto head = reinterpret_cast<const _aarch64_ctx*>(&mctx.__reserved[offset]);
            if (!head->magic)
            {
                // End of linked list
                return nullptr;
            }

            if (head->magic == ESR_MAGIC)
            {
                return reinterpret_cast<const esr_context*>(head);
            }

            offset += head->size;
        }

        return nullptr;
    }

	uint64_t find_esr(const ucontext_t* ctx){
		auto esr_ctx=find_esr_context(ctx);
		if(esr_ctx)return esr_ctx->esr;
		return -1;
	}

static void signal_handler(int /*sig*/, siginfo_t* info, void* uct) noexcept
{
	ucontext_t* ctx = static_cast<ucontext_t*>(uct);
	auto esr_ctx = find_esr_context(ctx);
    if(esr_ctx){
		LOGE("崩了 esr_ctx->esr %d[0x%x] ec %d[0x%x]",esr_ctx->esr,esr_ctx->esr,(esr_ctx->esr>>26)&0b111111,(esr_ctx->esr>>26)&0b111111)
	};
}

extern "C" __attribute__((visibility("default")))
jint JNI_OnLoad(JavaVM* vm, void* reserved){

    JNIEnv* env = NULL;
    int result=-1;

    LOGW("JNI_OnLoad ");
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("GetEnv failed");
        goto bail;
    }

	if (register_Emulator(env) != JNI_OK) {
			LOGE("register_Emulator failed");
			goto bail;
		}

    if (register_Emulator_cfg(env) != JNI_OK) {
			LOGE("register_Emulator_cfg failed");
			goto bail;
		}

    result = JNI_VERSION_1_6;

    LOGW("JNI_OnLoad OK");


    bail:
        return result;
}


#pragma clang diagnostic pop

