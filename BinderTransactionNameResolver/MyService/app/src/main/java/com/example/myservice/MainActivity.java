package com.example.myservice;
import android.app.Activity;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;

public class MainActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //启动service
        //Intent service = new Intent(MainActivity.this, MyService.class);
        //MainActivity.this.startForegroundService(service);


        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            MainActivity.this.startForegroundService(new Intent(MainActivity.this, MyService.class));
        } else {
            MainActivity.this.startService(new Intent(MainActivity.this, MyService.class));
        }

    }

    //8.0以上手机需要添加此代码才能正常运行
    public void onResume() {
        super.onResume();
        finish();
    }
}