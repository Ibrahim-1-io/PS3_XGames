package aenu.aps3e;

import android.app.ComponentCaller;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import static aenu.aps3e.MainActivity.getFileNameFromUri;

import kotlin.contracts.Returns;

public class QuickStartActivity extends AppCompatActivity {

    static final String ACTION_REENTRY="aenu.intent.action.REENTRY_QUISK_START";

    List<LinearLayout> layout_list;
    ProgressBar progress;
    int page;

    //ProgressTask progress_task;
    Emulator.Config config;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if(!ACTION_REENTRY.equals(getIntent().getAction())&&Application.get_default_config_file().exists()){
            goto_main_activity();
            return;
        }

        getSupportActionBar().setTitle(R.string.welcome);
        setContentView(R.layout.activity_quick_start);
        MainActivity.mk_dirs();
        try{config=Emulator.Config.open_config_from_string(load_default_config_str());}catch (Exception e){}
        init_layout_list();
        select_layout(0);
    }

    @Override
    protected void onDestroy() {
        if(config!=null) {
            config.close_config();
            config = null;
        }
        /*if(progress_task!=null){
            progress_task.force_close();
            progress_task=null;
        }*/

        super.onDestroy();
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != RESULT_OK || data == null) return;

        Uri uri = data.getData();

        if(uri==null) return;

        if(requestCode == MainActivity.REQUEST_SELECT_ISO_DIR){
            MainActivity.save_pref_iso_dir(this,uri);
            return;
        }

        String file_name = getFileNameFromUri(uri);

        switch (requestCode) {
            case MainActivity.REQUEST_INSTALL_FIRMWARE:
                (/*progress_task = */new ProgressTask(this)
                        .set_progress_message(getString(R.string.installing_firmware))
                        .set_failed_task(new ProgressTask.UI_Task() {
                            @Override
                            public void run() {
                                Toast.makeText(QuickStartActivity.this, getString(R.string.msg_failed), Toast.LENGTH_LONG).show();
                            }
                        })
                        .set_done_task(new ProgressTask.UI_Task() {
                                           @Override
                                           public void run() {
                                               try {
                                                   MainActivity.firmware_installed_file().createNewFile();
                                               } catch (IOException e) {
                                               }
                                               select_layout(page);
                                           }
                                       }
                        )).call(new ProgressTask.Task() {

                    @Override
                    public void run(ProgressTask task) {
                        int pfd;
                        try {
                            ParcelFileDescriptor pfd_ = getContentResolver().openFileDescriptor(uri, "r");
                            pfd = pfd_.detachFd();
                            pfd_.close();

                        } catch (Exception e) {
                            task.task_handler.sendEmptyMessage(ProgressTask.TASK_FAILED);
                            return;
                        }
                        MainActivity.firmware_installed_file().delete();
                        int result = Emulator.get.install_firmware(pfd) ? ProgressTask.TASK_DONE : ProgressTask.TASK_FAILED;
                        task.task_handler.sendEmptyMessage(result);
                    }
                });
                return;
            case EmulatorSettings.REQUEST_CODE_SELECT_CUSTOM_FONT:
                if (file_name.endsWith(".ttf") || file_name.endsWith(".ttc") || file_name.endsWith(".otf")) {
                    install_custom_font(uri);
                }
                return;

            case EmulatorSettings.REQUEST_CODE_SELECT_CUSTOM_DRIVER:
                if (file_name.endsWith(".zip")) {
                    install_custom_driver_from_zip(uri);
                } else if (file_name.endsWith(".so")) {
                    install_custom_driver_from_lib(uri);
                }
                return;
        }
}



    void init_layout_list(){
        layout_list=new ArrayList<>();
        layout_list.add(findViewById(R.id.quick_start_page_welcome));
        layout_list.add(findViewById(R.id.quick_start_page_install_firmware));
        layout_list.add(findViewById(R.id.quick_start_page_select_iso_dir));
        layout_list.add(findViewById(R.id.quick_start_page_install_font));
        if(Emulator.get.support_custom_driver()) {
            layout_list.add(findViewById(R.id.quick_start_page_install_gpu_driver));
            config.save_config_entry(EmulatorSettings.Video$Vulkan$Use_Custom_Driver,"true");
        }
        else
            findViewById(R.id.quick_start_page_install_gpu_driver).setVisibility(View.GONE);

        layout_list.add(findViewById(R.id.quick_start_page_config_modify));

        for(int i=0;i<layout_list.size();i++){
            layout_list.get(i).setVisibility(View.GONE);
        }

        progress=findViewById(android.R.id.progress);
        progress.setMax(layout_list.size());

        findViewById(R.id.prev_step).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(page>0){
                    select_layout(page-1);
                }
            }
        });
        findViewById(R.id.next_step).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(page==layout_list.size()-1){
                    goto_main_activity();
                }else{
                    select_layout(page+1);
                }
            }
        });
    }

    void select_layout(int pos){
        for(int i=0;i<layout_list.size();i++){
            layout_list.get(i).setVisibility(View.GONE);
        }

        page=pos;

        layout_list.get(page).setVisibility(View.VISIBLE);
        progress.setProgress(page+1);

        if(page==0) findViewById(R.id.prev_step).setVisibility(View.GONE);
        else findViewById(R.id.prev_step).setVisibility(View.VISIBLE);

        if(page==layout_list.size()-1) ((Button)findViewById(R.id.next_step)).setText(R.string.finish);
        else ((Button)findViewById(R.id.next_step)).setText(R.string.next_step);

        findViewById(R.id.next_step).setEnabled(true);

        update_layout(layout_list.get(page));
    }

    void update_layout(LinearLayout  layout){
        if(layout.getId()==R.id.quick_start_page_welcome){
            ((TextView)layout.findViewById(R.id.welcome_text1))
                    .setText(String.format(getString(R.string.welcome_content),getString(R.string.app_name)));
            ((TextView)layout.findViewById(R.id.welcome_text2))
                    .setText(String.format(getString(R.string.welcome_content2),getString(R.string.next_step)));
        }
        else if(layout.getId()==R.id.quick_start_page_install_firmware){
            int btn_text_res_id=MainActivity.firmware_installed_file().exists()?R.string.ps3_firmware_installed:R.string.select_ps3_firmware;
            ((Button)findViewById(R.id.install_firmware_btn)).setText(btn_text_res_id);
            ((Button)findViewById(R.id.next_step)).setEnabled(MainActivity.firmware_installed_file().exists());
            ((Button)findViewById(R.id.install_firmware_btn)).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    MainActivity.request_file_select(QuickStartActivity.this,MainActivity.REQUEST_INSTALL_FIRMWARE);
                }
            });
        }
        else if(layout.getId()==R.id.quick_start_page_select_iso_dir){
            int btn_text_res_id=MainActivity.load_pref_iso_dir(this)==null?R.string.set_iso_dir:R.string.ps3_iso_dir_is_set;
            ((Button)findViewById(R.id.select_ps3_iso_dir_btn)).setText(btn_text_res_id);
            ((Button)findViewById(R.id.select_ps3_iso_dir_btn)).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    MainActivity.request_iso_dir_select(QuickStartActivity.this);
                }
            });
        }
        else if(layout.getId()==R.id.quick_start_page_install_font){

            final String[] font_select_list_values=getResources().getStringArray(R.array.miscellaneous_font_file_selection_values);
            final String[] font_select_list=getResources().getStringArray(R.array.miscellaneous_font_file_selection_entries);

            if(((RadioGroup)findViewById(R.id.font_select_list)).getChildCount()==0)
            {
                ((RadioGroup)findViewById(R.id.font_select_list)).removeAllViews();
                for(int i=0;i<font_select_list.length;i++){
                    RadioButton btn=new RadioButton(this);
                    btn.setId(i);
                    btn.setText(font_select_list[i]);
                    RadioGroup.LayoutParams lp=new RadioGroup.LayoutParams(RadioGroup.LayoutParams.MATCH_PARENT,RadioGroup.LayoutParams.WRAP_CONTENT);
                    ((RadioGroup)findViewById(R.id.font_select_list)).addView(btn,lp);
                }
                ((RadioGroup)findViewById(R.id.font_select_list)).setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(RadioGroup group, int checkedId) {
                        String font_sel=font_select_list_values[checkedId];
                        config.save_config_entry(EmulatorSettings.Miscellaneous$Font_File_Selection,font_sel);
                        findViewById(R.id.custom_font_path).setEnabled(checkedId!=0);
                        findViewById(R.id.select_custom_font_list).setEnabled(checkedId!=0);
                        select_layout(page);
                    }
                });

                ((RadioGroup)findViewById(R.id.font_select_list)).check(1);
            }

            final String font_path_prefix=getString(R.string.custom_font_path_prefix);

            String font_path=config.load_config_entry(EmulatorSettings.Miscellaneous$Custom_Font_File_Path);
            ((TextView)findViewById(R.id.custom_font_path)).setText(font_path_prefix+font_path);

            if(font_path.isEmpty()){
                if(((RadioGroup)findViewById(R.id.font_select_list)).getCheckedRadioButtonId()==1)
                //if(config.load_config_entry(EmulatorSettings.Miscellaneous$Font_File_Selection)
                //        .equals(font_select_list_values[1]))
                    findViewById(R.id.next_step).setEnabled(false);
            }

            ArrayList<String> font_list=new ArrayList<>();
            {
                File[] files=Application.get_custom_font_dir().listFiles();
                if(files==null||files.length==0){
                    font_list.add(getString(R.string.emulator_settings_miscellaneous_custom_font_file_path_dialog_add_hint));
                }
                else{
                    for(File f:files){
                        font_list.add(f.getName());
                    }
                    font_list.add(getString(R.string.emulator_settings_miscellaneous_custom_font_file_path_dialog_add_hint));
                }
            }

            ((ListView)findViewById(R.id.select_custom_font_list)).setAdapter(new ArrayAdapter<String>(this
                    ,android.R.layout.simple_list_item_1,font_list));
            ((ListView)findViewById(R.id.select_custom_font_list)).setOnItemClickListener(new AdapterView.OnItemClickListener() {
                @Override
                public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                    if(position==font_list.size()-1){
                        MainActivity.request_file_select(QuickStartActivity.this,EmulatorSettings.REQUEST_CODE_SELECT_CUSTOM_FONT);
                    }
                    else {
                        File font_file=new File(Application.get_custom_font_dir(),font_list.get(position));
                        config.save_config_entry(EmulatorSettings.Miscellaneous$Custom_Font_File_Path,font_file.getAbsolutePath());
                        select_layout(page);
                    }
                }
            });
        }
        else if(layout.getId()==R.id.quick_start_page_install_gpu_driver){
            ((CheckBox)findViewById(R.id.enable_custom_gpu_driver)).setOnCheckedChangeListener(new CheckBox.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                    config.save_config_entry(EmulatorSettings.Video$Vulkan$Use_Custom_Driver,Boolean.toString(isChecked));
                    select_layout(page);
                }
            });
            ((CheckBox)findViewById(R.id.enable_custom_gpu_driver)).setChecked(Boolean.valueOf(
                    config.load_config_entry(EmulatorSettings.Video$Vulkan$Use_Custom_Driver)));

            final boolean enable_gpu_driver=((CheckBox)findViewById(R.id.enable_custom_gpu_driver)).isChecked();
            findViewById(R.id.custom_gpu_driver_path).setEnabled(enable_gpu_driver);
            findViewById(R.id.select_custom_gpu_driver_list).setEnabled(enable_gpu_driver);

            String gpu_driver_path_prefix=getString(R.string.custom_gpu_driver_path_prefix);
            String gpu_driver_path=config.load_config_entry(EmulatorSettings.Video$Vulkan$Custom_Driver_Library_Path);
            ((TextView)findViewById(R.id.custom_gpu_driver_path)).setText(gpu_driver_path_prefix+gpu_driver_path);
            if(enable_gpu_driver&&gpu_driver_path.isEmpty()){
                findViewById(R.id.next_step).setEnabled(false);
            }

            ArrayList<String> gpu_driver_list=new ArrayList<>();
            {
                File[] files=Application.get_custom_driver_dir().listFiles();
                if(files==null||files.length==0){
                    gpu_driver_list.add(getString(R.string.emulator_settings_video_vulkan_custom_driver_library_path_dialog_add_hint));
                }
                else{
                    for(File f:files){
                        gpu_driver_list.add(f.getName());
                    }
                    gpu_driver_list.add(getString(R.string.emulator_settings_video_vulkan_custom_driver_library_path_dialog_add_hint));
                }
            }
            ((ListView)findViewById(R.id.select_custom_gpu_driver_list)).setAdapter(new ArrayAdapter<String>(this
                    ,android.R.layout.simple_list_item_1,gpu_driver_list));
            ((ListView)findViewById(R.id.select_custom_gpu_driver_list)).setOnItemClickListener(new AdapterView.OnItemClickListener() {
                @Override
                public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                    if(position==gpu_driver_list.size()-1){
                        MainActivity.request_file_select(QuickStartActivity.this,EmulatorSettings.REQUEST_CODE_SELECT_CUSTOM_DRIVER);
                    }
                    else {
                        File driver_file=new File(Application.get_custom_driver_dir(),gpu_driver_list.get(position));
                        config.save_config_entry(EmulatorSettings.Video$Vulkan$Custom_Driver_Library_Path,driver_file.getAbsolutePath());
                        select_layout(page);
                    }
                }
            });

        }
        else if(layout.getId()==R.id.quick_start_page_config_modify){
            if(!Boolean.valueOf(config.load_config_entry(EmulatorSettings.Video$Vulkan$Use_Custom_Driver))){
                if(Emulator.get.get_vulkan_physical_dev_list()[0].contains("Adreno")){
                    config.save_config_entry(EmulatorSettings.Video$Texture_Upload_Mode,"CPU");
                    config.save_config_entry(EmulatorSettings.Video$Use_BGRA_Format,"false");
                    config.save_config_entry(EmulatorSettings.Video$Force_Convert_Texture,"true");
                }
            }

            boolean fix_llvm_cpu_cfg=false;
            String fix_llvm_cpu_val=null;
            {
                String llvm_cpu = Emulator.get.get_native_llvm_cpu_list()[0];

                if (llvm_cpu.equals("cortex-a510")
                        || llvm_cpu.equals("cortex-a710")
                        || llvm_cpu.equals("cortex-x2")
                        || llvm_cpu.equals("cortex-a715")
                        || llvm_cpu.equals("cortex-x3")
                        || llvm_cpu.equals("cortex-a520")
                        || llvm_cpu.equals("cortex-a720")
                        || llvm_cpu.equals("cortex-a725")
                        || llvm_cpu.equals("cortex-x4")
                        || llvm_cpu.equals("cortex-a925")
                ) {
                    fix_llvm_cpu_cfg = true;
                    fix_llvm_cpu_val = "cortex-x1";
                }
            }

            if(fix_llvm_cpu_cfg)
                config.save_config_entry(EmulatorSettings.Core$Use_LLVM_CPU,fix_llvm_cpu_val);
        }
    }

    String load_default_config_str(){
        return new String(Application.load_assets_file(
                this,"config/config.yml"));
    }

    void goto_main_activity(){
        if(config!=null){
            String config_str=config.close_config();
            try {
                FileOutputStream default_cfg_strm = new FileOutputStream(Application.get_default_config_file());
                default_cfg_strm.write(config_str.getBytes());
                default_cfg_strm.close();
                config=null;
            }catch (Exception e){
                Toast.makeText(this,e.getMessage(),Toast.LENGTH_LONG).show();
                if(Application.get_default_config_file().exists())
                    Application.get_default_config_file().delete();
                return;
            }
        }
        startActivity(new Intent(this,MainActivity.class));
        finish();
    }
    boolean install_custom_driver_from_zip(Uri uri){
        try {
            ParcelFileDescriptor pfd = getContentResolver().openFileDescriptor(uri, "r");
            FileInputStream fis = new FileInputStream(pfd.getFileDescriptor());
            ZipInputStream zis = new ZipInputStream(fis);
            for (ZipEntry ze = zis.getNextEntry(); ze != null; ze = zis.getNextEntry()) {
                if (ze.getName().endsWith(".so")) {
                    //String lib_path=new File(Application.get_custom_driver_dir() , ze.getName()).getAbsolutePath();
                    String lib_name=MainActivity.getFileNameFromUri(uri);
                    lib_name=lib_name.substring(0,lib_name.lastIndexOf('.'));
                    lib_name=lib_name+".so";
                    String lib_path=new File(Application.get_custom_driver_dir() , lib_name).getAbsolutePath();
                    FileOutputStream lib_os = new FileOutputStream(lib_path);
                    try {
                        byte[] buffer = new byte[16384];
                        int n;
                        while ((n = zis.read(buffer)) != -1)
                            lib_os.write(buffer, 0, n);
                        lib_os.close();
                        //fragment.setup_costom_driver_library_path(lib_path);
                        config.save_config_entry(EmulatorSettings.Video$Vulkan$Custom_Driver_Library_Path,lib_path);
                        select_layout(page);
                        zis.closeEntry();
                        break;
                    } catch (Exception e) {
                        Toast.makeText(this, e.toString(), Toast.LENGTH_SHORT).show();
                    }
                }
                zis.closeEntry();
            }
            zis.close();
            fis.close();
            pfd.close();
            return true;
        }
        catch (Exception e){
            Toast.makeText(this,e.toString(),Toast.LENGTH_SHORT).show();
            return false;
        }
    }

    boolean install_custom_driver_from_lib(Uri uri){
        try {
            ParcelFileDescriptor pfd = getContentResolver().openFileDescriptor(uri, "r");
            FileInputStream lib_is = new FileInputStream(pfd.getFileDescriptor());
            String lib_path=new File(Application.get_custom_driver_dir() , MainActivity.getFileNameFromUri(uri)).getAbsolutePath();
            FileOutputStream lib_os = new FileOutputStream(lib_path);

            byte[] buffer = new byte[16384];
            int n;
            while ((n = lib_is.read(buffer)) != -1)
                lib_os.write(buffer, 0, n);
            lib_os.close();
            //fragment.setup_costom_driver_library_path(lib_path);
            config.save_config_entry(EmulatorSettings.Video$Vulkan$Custom_Driver_Library_Path,lib_path);
            select_layout(page);
            lib_is.close();
            pfd.close();
            return true;
        } catch (Exception e) {
            Toast.makeText(this, e.toString(), Toast.LENGTH_SHORT).show();
            return false;
        }
    }

    boolean install_custom_font(Uri uri){
        try {
            ParcelFileDescriptor pfd = getContentResolver().openFileDescriptor(uri, "r");
            FileInputStream font_is = new FileInputStream(pfd.getFileDescriptor());
            String font_path=new File(Application.get_custom_font_dir(), MainActivity.getFileNameFromUri(uri)).getAbsolutePath();
            FileOutputStream font_os = new FileOutputStream(font_path);

            byte[] buffer = new byte[16384];
            int n;
            while ((n = font_is.read(buffer)) != -1)
                font_os.write(buffer, 0, n);
            font_os.close();
            //fragment.setup_costom_font_path(font_path);
            config.save_config_entry(EmulatorSettings.Miscellaneous$Custom_Font_File_Path,font_path);
            select_layout(page);
            font_is.close();
            pfd.close();
            return true;
        } catch (Exception e) {
            Toast.makeText(this, e.toString(), Toast.LENGTH_SHORT).show();
            return false;
        }
    }
}
