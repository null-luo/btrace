package com.example.myservice;

import android.annotation.SuppressLint;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Objects;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MyService extends Service {
    private static final int NOTIFICATION_ID = 1;
    private static final String channelId = "my_channel_id";
    private static final String channelName = "my_channel_name";

    private Map<Integer, String> getMethodNames(String serviceName) {
        Map<Integer, String> methodNames = new HashMap<>();
        try {
            Class<?> aClass = Class.forName(serviceName + "$Stub");
            Field[] fields = aClass.getDeclaredFields();

            @SuppressLint("PrivateApi") Class<?> bClass = Class.forName("com.android.internal.os.BinderTransactionNameResolver");
            Method method = bClass.getMethod("getMethodName", Class.class, int.class);
            Object instance = bClass.newInstance();
            for (int i = 1; i <= fields.length; i++) {
                String transactionName = (String) method.invoke(instance,aClass,i);
                if(transactionName != null){
                    //Log.d("null-luo", transactionName);
                    methodNames.put(i, transactionName);
                }
            }
        } catch (ClassNotFoundException e) {
            return methodNames;
        } catch (InvocationTargetException | InstantiationException | IllegalAccessException |
                NoSuchMethodException e) {
            throw new RuntimeException(e);
        }
        return methodNames;
    }

    private Notification createNotification() {
        Notification.Builder builder;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            // 创建通知渠道
            NotificationChannel channel = new NotificationChannel(channelId, channelName, NotificationManager.IMPORTANCE_LOW);
            NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            manager.createNotificationChannel(channel);

            builder = new Notification.Builder(this, channelId);
        } else {
            builder = new Notification.Builder(this);
        }

        // 设置通知内容
        builder.setContentTitle("My Service")
                .setContentText("Service is running in foreground")
                .setSmallIcon(R.drawable.my_ic_notification)
                .setAutoCancel(false)
                .setOngoing(true);

        return builder.build();
    }

    public MyService() {
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.e("null-luo", "MyService onBind");
        return null;
    }

    @Override
    public void onCreate() {
        Log.v("null-luo", "MyService onCreate");
        super.onCreate();
    }

    @Override
    public void onStart(Intent intent, int startId) {
        Log.v("null-luo", "MyService onStart");
        super.onStart(intent, startId);
    }

    @SuppressLint("ForegroundServiceType")
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.v("null-luo", "MyService onStartCommand");

        try {
            Notification notification = createNotification();
            startForeground(NOTIFICATION_ID, notification);

            HashMap<String, Map<Integer, String>> resultMap = new HashMap<>();;
            InputStream is = getAssets().open("service.list");
            BufferedReader reader = new BufferedReader(new InputStreamReader(is));
            Pattern pattern = Pattern.compile(".*\\[(.*)]");
            String line;
            while ((line = reader.readLine()) != null) {
                Matcher matcher = pattern.matcher(line);
                if (matcher.find()) {
                    String serviceName = matcher.group(1);
                    if(!Objects.equals(serviceName, "")){
                        Map<Integer, String> serviceMap = getMethodNames(serviceName);
                        if(serviceMap!=null && !serviceMap.isEmpty()){
                            resultMap.put(serviceName, serviceMap);
                        }
                    }
                }
            }

            JSONObject root = new JSONObject();
            Iterator<Map.Entry<String, Map<Integer, String>>> it = resultMap.entrySet().iterator();
            while (it.hasNext()) {
                Map.Entry<String, Map<Integer, String>> entry = it.next();
                String key = entry.getKey();
                Map<Integer, String> innerMap = entry.getValue();
                JSONObject subObject = new JSONObject();
                Iterator<Map.Entry<Integer, String>> innerIt = innerMap.entrySet().iterator();
                while (innerIt.hasNext()) {
                    Map.Entry<Integer, String> innerEntry = innerIt.next();
                    subObject.put(String.valueOf(innerEntry.getKey()), innerEntry.getValue());
                }
                root.put(key, subObject);
            }
            String jsonString = root.toString(2);
            //System.out.println(jsonString);

            FileOutputStream fos = null;
            fos = openFileOutput("methods.json", Context.MODE_PRIVATE);
            fos.write(jsonString.getBytes());

        } catch (IOException | JSONException e) {
            throw new RuntimeException(e);
        }

        return super.onStartCommand(intent, flags, startId);
    }
}
