#include "game.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static char saveDirectory[256] = "sdmc:/3ds/nightfall";
static uint32_t checksum(const SaveRecord *r) {
    const unsigned char *bytes = (const unsigned char *)r;
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < offsetof(SaveRecord, checksum); i++)
        hash = (hash ^ bytes[i]) * 16777619u;
    return hash;
}
void saveSetDirectory(const char *directory) {
    snprintf(saveDirectory, sizeof saveDirectory, "%s", directory);
}
static void pathFor(char *path, size_t size, int slot, const char *suffix) {
    snprintf(path, size, "%s/slot%d%s", saveDirectory, slot + 1, suffix);
}
static bool readFile(const char *path, SaveRecord *r) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    bool ok = fread(r, 1, sizeof *r, f) == sizeof *r;
    fclose(f);
    if (!ok || r->magic != 0x4e495447 || r->version != 1 || r->checksum != checksum(r))
        return false;
    if (r->checkpoint != 0 && r->checkpoint != 2)
        return false;
    if (!memchr(r->name, 0, sizeof r->name) || !r->name[0] || r->visited >= 64 || r->warden > 1)
        return false;
    return true;
}
bool saveRead(int slot, SaveRecord *r) {
    if (slot < 0 || slot >= SAVE_SLOTS)
        return false;
    char path[300];
    pathFor(path, sizeof path, slot, ".sav");
    if (readFile(path, r))
        return true;
    pathFor(path, sizeof path, slot, ".bak");
    return readFile(path, r);
}
bool saveWrite(int slot, SaveRecord *r) {
    if (slot < 0 || slot >= SAVE_SLOTS)
        return false;
    mkdir("sdmc:/3ds", 0777);
    mkdir(saveDirectory, 0777);
    r->magic = 0x4e495447;
    r->version = 1;
    r->checksum = checksum(r);
    char path[300], temporary[300], backup[300];
    pathFor(path, sizeof path, slot, ".sav");
    pathFor(temporary, sizeof temporary, slot, ".tmp");
    pathFor(backup, sizeof backup, slot, ".bak");
    FILE *f = fopen(temporary, "wb");
    if (!f)
        return false;
    bool ok = fwrite(r, 1, sizeof *r, f) == sizeof *r;
    if (fflush(f))
        ok = false;
    if (fclose(f))
        ok = false;
    if (!ok) {
        remove(temporary);
        return false;
    }
    SaveRecord checked;
    if (!readFile(temporary, &checked)) {
        remove(temporary);
        return false;
    }
    // Keep a verified prior save; do not replace a good backup with a corrupt primary.
    if (readFile(path, &checked)) {
        remove(backup);
        if (rename(path, backup)) {
            remove(temporary);
            return false;
        }
    } else
        remove(path);
    if (rename(temporary, path)) {
        rename(backup, path);
        return false;
    }
    return true;
}
