package aenu.preference;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.PreferenceViewHolder;

public class CheckBoxPreference extends androidx.preference.CheckBoxPreference{
    public CheckBoxPreference(@NonNull Context context) {
        super(context);
    }
    public CheckBoxPreference(@NonNull Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }
    public CheckBoxPreference(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }
    public CheckBoxPreference(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    public int get_title_color(){
        return title_color;
    }

    public void set_title_color(int color){
        title_color=color;
    }

    int title_color=0;
    @Override
    public void onBindViewHolder(@NonNull PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        TextView title_v=(TextView) holder.itemView.findViewById(android.R.id.title);
        if(title_v!=null&&title_color!=0)
            title_v.setTextColor(title_color);
    }
}
