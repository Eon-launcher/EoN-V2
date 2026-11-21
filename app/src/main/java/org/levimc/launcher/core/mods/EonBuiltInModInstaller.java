package org.levimc.launcher.core.mods;

import android.content.Context;
import android.util.Log;

import org.levimc.launcher.core.packages.InstalledPackages;
import org.levimc.launcher.core.packages.PackageService;
import org.levimc.launcher.core.versions.GameVersion;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;

public final class EonBuiltInModInstaller {
    public static final String MOD_ID = "eon";
    private static final String TAG = "EON";
    private static final String ENTRY_FILE = "libeon.so";
    private static final String DIAGNOSTICS_PREFS = "eon_diagnostics";
    private static final String ESP_LOG_PATH_KEY = "esp_log_path";
    private static final String MANIFEST = "{\n"
            + "  \"type\": \"preload-native\",\n"
            + "  \"name\": \"EON (Built-in)\",\n"
            + "  \"entry\": \"libeon.so\",\n"
            + "  \"author\": \"Built-in EON\",\n"
            + "  \"version\": \"1.0.0\",\n"
            + "  \"description\": \"EON native module framework\",\n"
            + "  \"minecraft_versions\": [\"1.26.60.4\"],\n"
            + "  \"built_in\": true\n"
            + "}\n";

    private EonBuiltInModInstaller() {
    }

    public static boolean install(Context context, GameVersion version) throws IOException {
        if (version == null || version.modsDir == null) {
            Log.w(TAG, "EON cannot be staged: this instance has no native mod directory");
            return false;
        }

        File source = new File(context.getApplicationInfo().nativeLibraryDir, ENTRY_FILE);
        if (!source.isFile() || !source.canRead()) {
            throw new IOException("Packaged EON native library is missing: " + source.getAbsolutePath());
        }

        File workspace = PackageService.get().workspaceDir(version);
        if (workspace.isDirectory()) {
            try {
                if (InstalledPackages.read(workspace).ownerOf(MOD_ID) != null) {
                    throw new IOException("The reserved EON mod ID is owned by an installed package");
                }
            } catch (PackageService.Failure error) {
                throw new IOException("Failed to verify ownership of the reserved EON mod ID", error);
            }
        }

        File modsDir = version.modsDir;
        if (!modsDir.isDirectory() && !modsDir.mkdirs()) {
            throw new IOException("Failed to create instance mod directory: " + modsDir.getAbsolutePath());
        }

        File packageDir = new File(modsDir, MOD_ID);
        if (!packageDir.isDirectory() && !packageDir.mkdirs()) {
            throw new IOException("Failed to create built-in EON directory: " + packageDir.getAbsolutePath());
        }

        File manifest = new File(packageDir, "manifest.json");
        if (manifest.exists()) {
            String existingManifest = new String(
                    Files.readAllBytes(manifest.toPath()),
                    java.nio.charset.StandardCharsets.UTF_8);
            if (!MANIFEST.equals(existingManifest)) {
                throw new IOException("The reserved EON mod directory is already in use: " + packageDir.getAbsolutePath());
            }
        } else {
            File[] existingFiles = packageDir.listFiles();
            if (existingFiles == null) {
                throw new IOException("Failed to list built-in EON directory: " + packageDir.getAbsolutePath());
            }
            for (File existingFile : existingFiles) {
                String name = existingFile.getName();
                if (!"manifest.json.tmp".equals(name) && !(ENTRY_FILE + ".tmp").equals(name)) {
                    throw new IOException("The reserved EON mod directory is already in use: " + packageDir.getAbsolutePath());
                }
            }
        }

        File manifestTemp = new File(packageDir, "manifest.json.tmp");
        try (FileOutputStream output = new FileOutputStream(manifestTemp)) {
            output.write(MANIFEST.getBytes(java.nio.charset.StandardCharsets.UTF_8));
            output.getFD().sync();
        }
        replaceFile(manifestTemp, manifest);

        File libraryTemp = new File(packageDir, ENTRY_FILE + ".tmp");
        File library = new File(packageDir, ENTRY_FILE);
        copyFile(source, libraryTemp);
        replaceFile(libraryTemp, library);

        File configDir = new File(packageDir, "config");
        File espLogFile = new File(configDir, "esp.log");
        if (!configDir.isDirectory() && !configDir.mkdirs()) {
            Log.w(TAG, "Failed to create the EON diagnostics directory");
        } else {
            String launchMarker = System.currentTimeMillis()
                    + " [INFO] EON staged for Minecraft "
                    + version.versionCode + "; awaiting native initialization.\n";
            try (FileOutputStream output = new FileOutputStream(espLogFile, false)) {
                output.write(launchMarker.getBytes(
                        java.nio.charset.StandardCharsets.UTF_8));
                output.getFD().sync();
            } catch (IOException error) {
                Log.w(TAG, "Failed to initialize the EON ESP diagnostics file",
                        error);
            }
        }
        boolean logPathSaved = context.getSharedPreferences(
                        DIAGNOSTICS_PREFS, Context.MODE_PRIVATE)
                .edit()
                .putString(ESP_LOG_PATH_KEY, espLogFile.getAbsolutePath())
                .commit();
        if (!logPathSaved) {
            Log.w(TAG, "Failed to save the EON ESP diagnostics path");
        }

        Log.i(TAG, "Staged built-in native mod for instance " + version.directoryName);
        return true;
    }

    public static File getEspLogFile(Context context) {
        String path = context.getSharedPreferences(
                        DIAGNOSTICS_PREFS, Context.MODE_PRIVATE)
                .getString(ESP_LOG_PATH_KEY, null);
        return path == null || path.trim().isEmpty() ? null : new File(path);
    }

    public static synchronized void appendEspDiagnostic(
            Context context, String level, String message) {
        File logFile = getEspLogFile(context);
        if (logFile == null) {
            return;
        }

        String line = System.currentTimeMillis() + " [" + level + "] "
                + message.replace('\n', ' ').replace('\r', ' ') + "\n";
        byte[] bytes = line.getBytes(java.nio.charset.StandardCharsets.UTF_8);
        if (bytes.length > 4096) {
            Log.w(TAG, "Skipped an oversized EON ESP diagnostic");
            return;
        }
        try {
            File parent = logFile.getParentFile();
            if (parent != null && !parent.isDirectory() && !parent.mkdirs()) {
                throw new IOException("Could not create diagnostics directory");
            }
            boolean append = logFile.length() + bytes.length <= 256 * 1024;
            try (FileOutputStream output = new FileOutputStream(logFile, append)) {
                output.write(bytes);
                output.getFD().sync();
            }
        } catch (IOException error) {
            Log.w(TAG, "Failed to append an EON ESP diagnostic", error);
        }
    }

    private static void copyFile(File source, File target) throws IOException {
        try (FileInputStream input = new FileInputStream(source);
             FileOutputStream output = new FileOutputStream(target)) {
            byte[] buffer = new byte[64 * 1024];
            int count;
            while ((count = input.read(buffer)) != -1) {
                output.write(buffer, 0, count);
            }
            output.getFD().sync();
        }
    }

    private static void replaceFile(File source, File target) throws IOException {
        try {
            Files.move(source.toPath(), target.toPath(), StandardCopyOption.REPLACE_EXISTING);
        } catch (IOException moveError) {
            if (target.exists() && !target.delete()) {
                throw new IOException("Failed to replace built-in EON file: " + target.getAbsolutePath(), moveError);
            }
            if (!source.renameTo(target)) {
                throw new IOException("Failed to install built-in EON file: " + target.getAbsolutePath(), moveError);
            }
        }
    }
}
