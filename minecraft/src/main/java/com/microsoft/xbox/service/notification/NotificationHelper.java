package com.microsoft.xbox.service.notification;

import android.content.Context;

import androidx.annotation.NonNull;

import org.jetbrains.annotations.Contract;

/* Modified by EoN Launcher V2: parse notification payloads without a Firebase dependency. */
public class NotificationHelper {
    @NonNull
    @Contract("_, _ -> new")
    public static NotificationResult tryParseXboxLiveNotification(
            @NonNull Object remoteMessage,
            Context context
    ) {
        return new NotificationResult(remoteMessage, context);
    }
}
