package org.levimc.launcher.core.discord;

import android.app.Application;

public final class DiscordPresence {
    private DiscordPresence() {
    }

    // Modified by EON Launcher V2: the optional Discord SDK is unavailable in clean source builds.
    public static void initialize(Application application) {
    }

    public static boolean isConfigured() {
        return false;
    }

    public static void refresh() {
    }

    public static void shutdown() {
    }
}
