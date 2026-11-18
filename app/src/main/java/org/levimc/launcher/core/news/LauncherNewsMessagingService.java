package org.levimc.launcher.core.news;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

/* Modified by EoN Launcher V2: retain the legacy news service type without Firebase. */
public final class LauncherNewsMessagingService extends Service {
    public void onNewToken(String token) {
    }

    public void onMessageReceived(Object message) {
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
