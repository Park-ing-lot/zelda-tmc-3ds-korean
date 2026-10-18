#include "platform_3ds.h"
#include "updater.h"
#include "port_second_screen_theme.h"

#include "port_audio.h"
#include "port_ppu.h"
#include "port_rom.h"
#include "port_runtime_config.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mbedtls/sha256.h>

#define APP_DIR "sdmc:/3ds/The Minish Cap 3DS Korean"
#define ROM_PATH_SIZE 512

extern void AgbMain(void);

static int PrepareStorage(void) {
    mkdir("sdmc:/3ds", 0777);
    if (mkdir(APP_DIR, 0777) != 0 && errno != EEXIST) return 0;
    return chdir(APP_DIR) == 0;
}

static int HasGbaExtension(const char* name) {
    const size_t length = strlen(name);
    if (length < 5) return 0;
    const char* ext = name + length - 4;
    return tolower((unsigned char)ext[0]) == '.' &&
           tolower((unsigned char)ext[1]) == 'g' &&
           tolower((unsigned char)ext[2]) == 'b' &&
           tolower((unsigned char)ext[3]) == 'a';
}

static int RomIsSupported(const char* path) {
    char gameCode[4] = {0};
    FILE* file = fopen(path, "rb");
    if (!file) return 0;
    int ok = fseek(file, 0xAC, SEEK_SET) == 0 && fread(gameCode, 1, sizeof(gameCode), file) == sizeof(gameCode) &&
             (memcmp(gameCode, "BZME", sizeof(gameCode)) == 0 || memcmp(gameCode, "BZMP", sizeof(gameCode)) == 0);
    if (!ok && memcmp(gameCode, "BZMJ", 4) == 0) {
        /* Only this analyzed Korean revision is supported. JP headers alone
         * cannot distinguish arbitrary translations or the retail Japanese ROM. */
        uint8_t data[4096], digest[32];
        char hex[65];
        size_t total = 0, count;
        mbedtls_sha256_context sha;
        mbedtls_sha256_init(&sha);
        int valid = fseek(file, 0, SEEK_SET) == 0 && mbedtls_sha256_starts_ret(&sha, 0) == 0;
        while (valid && (count = fread(data, 1, sizeof(data), file)) != 0) {
            total += count;
            if (total > PORT_KOREAN_ROM_SIZE) { valid = 0; break; }
            valid = mbedtls_sha256_update_ret(&sha, data, count) == 0;
        }
        valid = valid && !ferror(file) && total == PORT_KOREAN_ROM_SIZE &&
                mbedtls_sha256_finish_ret(&sha, digest) == 0;
        mbedtls_sha256_free(&sha);
        if (valid) {
            for (unsigned i = 0; i < sizeof(digest); ++i) snprintf(hex + i * 2, 3, "%02x", digest[i]);
            if (strcmp(hex, PORT_KOREAN_ROM_SHA256) == 0) ok = 2;
        }
    }
    fclose(file);
    return ok;
}

static int FindRom(char* out, size_t outSize) {
    DIR* dir = opendir(".");
    if (!dir) return 0;

    int foundGba = 0;
    int foundRetail = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        if (!HasGbaExtension(entry->d_name)) continue;

        struct stat info;
        if (stat(entry->d_name, &info) != 0 || !S_ISREG(info.st_mode)) continue;
        foundGba = 1;
        int supported = RomIsSupported(entry->d_name);
        if (!supported) continue;
        if (supported == 2 || !foundRetail) snprintf(out, outSize, "%s", entry->d_name);
        if (supported == 2) { closedir(dir); return 1; }
        foundRetail = 1;
    }

    closedir(dir);
    return foundRetail ? 1 : (foundGba ? -1 : 0);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    if (!Platform3DS_Init()) return 1;
    Platform3DS_ShowSplash();

    printf("The Minish Cap 3DS v" TMC_PORT_VERSION "\n\n");
    printf("agbplay: ipatix and contributors (LGPLv3).\nLicenses: romfs:/licenses/\n\n");
    printf("System: %s\n", Platform3DS_IsNew3DS() ? "New Nintendo 3DS" : "Nintendo 3DS");
    printf("Performance: %s\n",
           Platform3DS_IsNew3DS() ? "New 3DS full presentation"
                                  : "Old 3DS adaptive presentation skip (max 3)");
    printf("PPU worker core 1: %u%%\n", Platform3DS_Core1TimeLimit());
    printf("Extra New 3DS core: %s\n\n", Platform3DS_IsNew3DS() ? "enabled" : "unavailable");
    printf("Preparing storage...\n");
    if (!PrepareStorage()) {
        Platform3DS_ShowFatal("Storage error", "Could not open " APP_DIR ".");
        Platform3DS_Shutdown();
        return 1;
    }

    char romPath[ROM_PATH_SIZE];
    int romResult = FindRom(romPath, sizeof(romPath));
    if (romResult <= 0) {
        char message[512];
        if (romResult < 0) {
            snprintf(message, sizeof(message),
                     "None of the .gba files in:\n%s\n"
                     "is a supported Korean, USA or Europe ROM.\n\n"
                     "Korean SHA-1: 5dfc388f30bec4db3965632cca8fe1a97d9c9f79\n"
                     "Expected SHA-1:\nUSA: b4bd50e4131b027c334547b4524e2dbbd4227130\n"
                     "Europe: cff199b36ff173fb6faf152653d1bccf87c26fb7",
                     APP_DIR);
        } else {
            snprintf(message, sizeof(message),
                     "Copy your Korean, USA or Europe ROM to:\n%s\n\nAny .gba filename is accepted.\n\n"
                     "Korean SHA-1: 5dfc388f30bec4db3965632cca8fe1a97d9c9f79\n"
                     "Expected SHA-1:\nUSA: b4bd50e4131b027c334547b4524e2dbbd4227130\n"
                     "Europe: cff199b36ff173fb6faf152653d1bccf87c26fb7",
                     APP_DIR);
        }
        Platform3DS_ShowFatal(romResult < 0 ? "Unsupported ROM" : "ROM not found", message);
        Platform3DS_Shutdown();
        return 1;
    }

    FILE* rom = fopen(romPath, "rb");
    if (!rom) {
        Platform3DS_ShowFatal("ROM error", "Could not open the selected .gba file.");
        Platform3DS_Shutdown();
        return 1;
    }
    fclose(rom);

    printf("Loading ROM and tables...\n");
    Port_Config_Load("tmc3ds.ini");
    Port_LoadRom(romPath);
    Port_SecondScreenTheme_Ready();
    Port_PPU_Init(NULL);
    if (!Port_Audio_Init()) {
        printf("Warning: audio is unavailable.\n");
    }

    printf("Starting engine...\n");
    Platform3DS_EnterGameplayDisplay();
    Updater_Init(argc > 0 ? argv[0] : NULL);
    AgbMain();
    Updater_Shutdown();

    Port_PPU_Shutdown();
    Platform3DS_Shutdown();
    return 0;
}
