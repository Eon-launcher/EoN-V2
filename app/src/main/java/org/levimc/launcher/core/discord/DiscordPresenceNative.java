package org.levimc.launcher.core.discord;

/* Modified by EoN Launcher V2: preserve the legacy native bridge class without Discord runtime calls. */
final class DiscordPresenceNative {
    private DiscordPresenceNative() {}

    static long create(long applicationId) {
        return 0L;
    }

    static void update(long handle, String details, String state, long startedAt, String largeImage) {
    }

    static void tick(long handle) {
    }

    static void destroy(long handle) {
    }
}
