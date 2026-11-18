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

        Log.i(TAG, "Staged built-in native mod for instance " + version.directoryName);
        return true;
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
