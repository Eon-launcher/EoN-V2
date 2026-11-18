package com.microsoft.xbox.service.notification;

import android.content.Context;
import android.util.Log;

import androidx.annotation.NonNull;

import com.mojang.minecraftpe.R;

import java.lang.reflect.Method;
import java.util.Collections;
import java.util.Map;

/* Modified by EoN Launcher V2: keep Xbox notification data types without Firebase. */
public class NotificationResult {
    public String body;
    public String data;
    public NotificationType notificationType;
    public String title;

    public NotificationResult(@NonNull Object remoteMessage, Context context) {
        Map<String, String> values = readData(remoteMessage);
        String type = values.get("type");
        if ("xbox_live_game_invite".equals(type)) {
            notificationType = NotificationType.Invite;
            title = context.getString(R.string.xbox_live_game_invite_title);
            body = context.getString(R.string.xbox_live_game_invite_body);
            Object notification = readNotification(remoteMessage);
            Object args = invoke(notification, "getBodyLocalizationArgs");
            if (args instanceof String[] && ((String[]) args).length >= 2) {
                body = String.format(body, ((String[]) args)[0], ((String[]) args)[1]);
            }
        } else if ("xbox_live_achievement_unlock".equals(type)) {
            notificationType = NotificationType.Achievement;
            Object notification = readNotification(remoteMessage);
            Object resolvedTitle = invoke(notification, "getTitle");
            Object resolvedBody = invoke(notification, "getBody");
            title = resolvedTitle instanceof String ? (String) resolvedTitle : null;
            body = resolvedBody instanceof String ? (String) resolvedBody : null;
        } else {
            notificationType = NotificationType.Unknown;
        }
        data = values.get("xbl");
    }

    @SuppressWarnings("unchecked")
    private static Map<String, String> readData(Object message) {
        if (message instanceof Map) {
            return (Map<String, String>) message;
        }
        Object value = invoke(message, "getData");
        return value instanceof Map ? (Map<String, String>) value : Collections.emptyMap();
    }

    private static Object invoke(Object target, String methodName) {
        if (target == null) {
            return null;
        }

        private static Object readNotification(Object message) {
            return message instanceof Map ? null : invoke(message, "getNotification");
        }
        try {
            Method method = target.getClass().getMethod(methodName);
            return method.invoke(target);
        } catch (ReflectiveOperationException error) {
            Log.w("XSAPI.Android", "Could not read notification field " + methodName, error);
            return null;
        }
    }

    public enum NotificationType {
        Achievement,
        Invite,
        Unknown
    }
}
