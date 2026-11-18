package com.microsoft.xbox.service.notification;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

import com.mojang.minecraftpe.NotificationListenerService;

/* Modified by EoN Launcher V2: retain the legacy service class without Firebase. */
public class NotificationFirebaseMessagingService extends Service {
    public void onNewToken(String token) {
        NotificationListenerService.refreshDeviceRegistrationToken(this);
    }

    public void onMessageReceived(Object message) {
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
