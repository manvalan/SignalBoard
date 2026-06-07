#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

class NVSManager {
  private:
    String nameSpace;

  public:
    // Costruttore: ogni istanza gestisce un compartimento stagno (namespace)
    NVSManager(String ns) {
        nameSpace = ns;
    }

    // ==========================================
    // METODI DI SCRITTURA
    // ==========================================
    void writeString(const char* key, String value) {
        Preferences prefs;
        prefs.begin(nameSpace.c_str(), false);
        prefs.putString(key, value);
        prefs.end();
    }

    void writeInt(const char* key, int value) {
        Preferences prefs;
        prefs.begin(nameSpace.c_str(), false);
        prefs.putInt(key, value);
        prefs.end();
    }

    void writeBool(const char* key, bool value) {
        Preferences prefs;
        prefs.begin(nameSpace.c_str(), false);
        prefs.putBool(key, value);
        prefs.end();
    }

    // ==========================================
    // METODI DI LETTURA
    // ==========================================
    String readString(const char* key, String defaultValue = "") {
        Preferences prefs;
        prefs.begin(nameSpace.c_str(), true); // true = sola lettura, molto più veloce e sicuro
        String value = prefs.getString(key, defaultValue);
        prefs.end();
        return value;
    }

    int readInt(const char* key, int defaultValue = 0) {
        Preferences prefs;
        prefs.begin(nameSpace.c_str(), true);
        int value = prefs.getInt(key, defaultValue);
        prefs.end();
        return value;
    }

    bool readBool(const char* key, bool defaultValue = false) {
        Preferences prefs;
        prefs.begin(nameSpace.c_str(), true);
        bool value = prefs.getBool(key, defaultValue);
        prefs.end();
        return value;
    }

    // ==========================================
    // METODI DI MANUTENZIONE
    // ==========================================
    
    // Elimina una singola chiave
    void removeKey(const char* key) {
        Preferences prefs;
        prefs.begin(nameSpace.c_str(), false);
        prefs.remove(key);
        prefs.end();
    }

    // Pialla completamente tutto il namespace (utile per i factory reset)
    void clearAll() {
        Preferences prefs;
        prefs.begin(nameSpace.c_str(), false);
        prefs.clear();
        prefs.end();
        Serial.printf("NVSManager: Namespace '%s' azzerato.\n", nameSpace.c_str());
    }
};

#endif