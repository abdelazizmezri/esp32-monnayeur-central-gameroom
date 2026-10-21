#include "StorageService.h"
#include "AppConfig.h"
#include "CreditMath.h"

#include <Preferences.h>
#include <ArduinoJson.h>
#include <esp_system.h>
#include <vector>

static Preferences prefs;

namespace StorageService {
namespace {
constexpr size_t MAX_BACKUP = 24576;
constexpr size_t MAX_POSTS_JSON = 8192;

bool storageFail(AppState& state, const String& error) {
  state.creditStorageReady = false;
  state.creditStorageError = error;
  return false;
}

bool writeVerified(const char* ns, const char* key, const uint8_t* data, size_t size) {
  if (!prefs.begin(ns, false)) return false;
  bool ok = prefs.putBytes(key, data, size) == size;
  std::vector<uint8_t> check(size);
  ok = ok && prefs.getBytesLength(key) == size &&
       prefs.getBytes(key, check.data(), size) == size && !memcmp(data, check.data(), size);
  prefs.end();
  return ok;
}

bool removeKey(const char* ns, const char* key) {
  if (!prefs.begin(ns, false)) return false;
  bool ok = !prefs.isKey(key) || prefs.remove(key);
  prefs.end();
  return ok;
}

bool writeString(const char* ns, const char* key, const String& value) {
  if (!prefs.begin(ns, false)) return false;
  bool ok = prefs.putString(key, value) == value.length() && prefs.getString(key, "") == value;
  prefs.end();
  return ok;
}

bool writeJsonBlob(const char* key, const String& json) {
  if (json.isEmpty() || json.length() > MAX_BACKUP) return false;
  std::vector<uint8_t> data(json.length() + 6);
  memcpy(data.data(), "JSN4", 4);
  memcpy(data.data() + 6, json.c_str(), json.length());
  RevenueJournal::put(data.data() + 4, Money::crc16(data.data() + 6, json.length()), 2);
  return writeVerified("appcfg", key, data.data(), data.size());
}

// 0 = absent, -1 = invalide, 1 = valide. Ne jamais masquer une corruption.
int readJsonBlob(const char* key, String& json) {
  if (!prefs.begin("appcfg", true)) return 0;
  if (!prefs.isKey(key)) { prefs.end(); return 0; }
  size_t size = prefs.getBytesLength(key);
  if (size <= 6 || size > MAX_BACKUP + 6) { prefs.end(); return -1; }
  std::vector<uint8_t> data(size + 1, 0);
  bool ok = prefs.getBytes(key, data.data(), size) == size &&
            !memcmp(data.data(), "JSN4", 4) &&
            RevenueJournal::get(data.data() + 4, 2) == Money::crc16(data.data() + 6, size - 6);
  prefs.end();
  if (!ok) return -1;
  json = reinterpret_cast<const char*>(data.data() + 6);
  return json.length() == size - 6 ? 1 : -1;
}

bool readSnapshot(const char* key, const char* magic, uint8_t* data, size_t size) {
  return prefs.getBytesLength(key) == size && prefs.getBytes(key, data, size) == size &&
         !memcmp(data, magic, 4) && RevenueJournal::get(data + size - 2, 2) == Money::crc16(data, size - 2);
}

bool validConfig(const AppState& state) {
  return state.coinDurationSeconds > 0 && state.availablePieces >= 0 &&
    Money::validValue(state.coinCurrency, state.pulseValueMinor) &&
    state.dzdToTndRateMicros <= Money::MAX_RATE &&
    (state.coinCurrency != Money::DZD || Money::validRate(state.dzdToTndRateMicros));
}

void buildPostsArray(JsonArray arr, const AppState& state) {
  for (const auto& post : state.posts) {
    JsonObject obj = arr.createNestedObject();
    obj["chipId"] = post.chipId; obj["id"] = post.id;
    obj["name"] = post.name; obj["ip"] = post.ip;
    obj["piecesRequired"] = post.piecesRequired;
  }
}

void buildUsersArray(JsonArray arr, const AppState& state) {
  for (const auto& user : state.users) {
    JsonObject obj = arr.createNestedObject();
    obj["username"] = user.username; obj["firstName"] = user.firstName;
    obj["lastName"] = user.lastName; obj["role"] = user.role;
    obj["passwordSalt"] = user.passwordSalt; obj["passwordHash"] = user.passwordHash;
  }
}

bool validUsername(const String& name) {
  if (name.length() < 3 || name.length() > 32) return false;
  for (size_t i = 0; i < name.length(); ++i) {
    char c = name.charAt(i);
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) return false;
  }
  return true;
}

bool loadPostsFromArray(AppState& state, JsonArray arr, int legacyPrice = 0) {
  state.posts.clear();
  for (JsonObject obj : arr) {
    if (!obj["id"].is<const char*>() || !obj["name"].is<const char*>() ||
        !obj["ip"].is<const char*>() ||
        (obj.containsKey("chipId") && !obj["chipId"].is<const char*>())) return false;
    Post p;
    p.chipId = obj["chipId"] | ""; p.id = obj["id"].as<String>();
    p.name = obj["name"].as<String>(); p.ip = obj["ip"].as<String>();
    if (legacyPrice > 0) p.piecesRequired = legacyPrice;
    else {
      if (!obj["piecesRequired"].is<int>()) return false;
      p.piecesRequired = obj["piecesRequired"].as<int>();
    }
    if (p.id.isEmpty() || p.name.isEmpty() || p.ip.isEmpty() || p.piecesRequired <= 0) return false;
    for (const auto& existing : state.posts) {
      if (existing.id == p.id || existing.ip == p.ip ||
          (!p.chipId.isEmpty() && existing.chipId == p.chipId)) return false;
    }
    state.posts.push_back(p);
  }
  return true;
}

bool loadUsersFromArray(AppState& state, JsonArray arr) {
  state.users.clear();
  if (!arr.size() || arr.size() > 10) return false;
  bool hasAdmin = false;
  for (JsonObject obj : arr) {
    const char* fields[] = {"username", "firstName", "lastName", "role", "passwordSalt", "passwordHash"};
    for (const char* key : fields) if (!obj[key].is<const char*>()) return false;
    UserAccount user;
    user.username = obj["username"].as<String>(); user.username.trim(); user.username.toLowerCase();
    user.firstName = obj["firstName"].as<String>(); user.firstName.trim();
    user.lastName = obj["lastName"].as<String>(); user.lastName.trim();
    user.role = obj["role"].as<String>(); user.passwordSalt = obj["passwordSalt"].as<String>();
    user.passwordHash = obj["passwordHash"].as<String>();
    if (!validUsername(user.username) || user.firstName.isEmpty() || user.firstName.length() > 64 ||
        user.lastName.isEmpty() || user.lastName.length() > 64 ||
        (user.role != "admin" && user.role != "user") || user.passwordSalt.isEmpty() ||
        user.passwordHash.length() != 64) return false;
    for (const auto& existing : state.users) if (existing.username == user.username) return false;
    state.users.push_back(user); hasAdmin |= user.role == "admin";
  }
  return hasAdmin;
}

bool validIp(const String& value) {
  unsigned a, b, c, d; char extra;
  return sscanf(value.c_str(), "%u.%u.%u.%u%c", &a, &b, &c, &d, &extra) == 4 &&
         a <= 255 && b <= 255 && c <= 255 && d <= 255;
}

bool validWifi(JsonObject wifi) {
  if (!wifi["ssid"].is<const char*>() || !wifi["password"].is<const char*>() ||
      !wifi["staticIp"].is<bool>() || wifi["ssid"].as<String>().length() > 32 ||
      wifi["password"].as<String>().length() > 64) return false;
  const char* keys[] = {"localIp", "gateway", "subnet", "dns1", "dns2"};
  for (const char* key : keys) {
    if (!wifi[key].is<const char*>()) return false;
    String value = wifi[key].as<String>();
    if ((!value.isEmpty() && !validIp(value)) ||
        (wifi["staticIp"].as<bool>() && value.isEmpty() && strcmp(key, "dns1") && strcmp(key, "dns2"))) return false;
  }
  return true;
}

bool writeWifi(JsonObject wifi) {
  if (!prefs.begin("wifi", false)) return false;
  bool ok = prefs.putBool("staticIp", wifi["staticIp"].as<bool>()) == 1 &&
            prefs.getBool("staticIp", !wifi["staticIp"].as<bool>()) == wifi["staticIp"].as<bool>();
  const char* keys[] = {"ssid", "password", "localIp", "gateway", "subnet", "dns1", "dns2"};
  for (const char* key : keys) {
    String value = wifi[key].as<String>();
    if (prefs.putString(key, value) != value.length() || prefs.getString(key, "\x01") != value) ok = false;
  }
  prefs.end();
  return ok;
}

void buildBackup(const AppState& state, JsonDocument& doc) {
  doc["format"] = "pstm-backup"; doc["version"] = 4;
  doc["creditUnit"] = "pulse";
  doc["coinDurationSeconds"] = state.coinDurationSeconds;
  doc["coinCurrency"] = Money::code(state.coinCurrency);
  doc["pulseValueMinor"] = state.pulseValueMinor;
  doc["dzdToTndRateMicros"] = state.dzdToTndRateMicros;
  doc["availablePieces"] = state.availablePieces; doc["apiToken"] = state.apiToken;
  buildUsersArray(doc.createNestedArray("users"), state);
  buildPostsArray(doc.createNestedArray("posts"), state);
  prefs.begin("wifi", true);
  JsonObject wifi = doc.createNestedObject("wifi");
  wifi["ssid"] = prefs.getString("ssid", ""); wifi["password"] = prefs.getString("password", "");
  wifi["staticIp"] = prefs.getBool("staticIp", false);
  const char* keys[] = {"localIp", "gateway", "subnet", "dns1", "dns2"};
  for (const char* key : keys) wifi[key] = prefs.getString(key, "");
  prefs.end();
}

bool parseBackup(JsonDocument& doc, AppState& restored, String& error) {
  if (!doc.is<JsonObject>() || !doc["version"].is<int>() ||
      String(doc["format"] | "") != "pstm-backup" ||
      !doc["users"].is<JsonArray>() || !doc["posts"].is<JsonArray>()) {
    error = "Format de sauvegarde invalide."; return false;
  }
  int version = doc["version"].as<int>();
  if (version < 1 || version > 4 || !doc["coinDurationSeconds"].is<int>() ||
      !doc["apiToken"].is<const char*>()) { error = "Version ou paramètres invalides."; return false; }
  restored.coinDurationSeconds = doc["coinDurationSeconds"].as<int>();
  restored.apiToken = doc["apiToken"].as<String>();
  if (version >= 3) {
    if (!doc["coinCurrency"].is<const char*>() || !doc["pulseValueMinor"].is<uint32_t>() ||
        !doc["dzdToTndRateMicros"].is<uint32_t>()) { error = "Paramètres monétaires invalides."; return false; }
    String currency = doc["coinCurrency"].as<String>();
    restored.coinCurrency = currency == "TND" ? Money::TND : currency == "DZD" ? Money::DZD : 255;
    restored.pulseValueMinor = doc["pulseValueMinor"].as<uint32_t>();
    restored.dzdToTndRateMicros = doc["dzdToTndRateMicros"].as<uint32_t>();
  } else if (version == 2) {
    if (!doc["pulseValueMillimes"].is<uint32_t>()) { error = "Valeur d'impulsion invalide."; return false; }
    restored.pulseValueMinor = doc["pulseValueMillimes"].as<uint32_t>();
  }
  int legacyPrice = 0;
  if (version == 4) {
    if (String(doc["creditUnit"] | "") != "pulse" || !doc["availablePieces"].is<int>() ||
        doc.containsKey("pulsesPerCoin") || doc.containsKey("availableCoins") || doc.containsKey("pendingPulses")) {
      error = "Le solde V4 doit être exprimé en impulsions."; return false;
    }
    restored.availablePieces = doc["availablePieces"].as<int>();
  } else {
    if (!doc["pulsesPerCoin"].is<int>() || !doc["availableCoins"].is<int>() ||
        (version >= 2 && !doc["pendingPulses"].is<int>()) ||
        (version == 3 && !doc["pendingPulsesPerCoin"].is<int>())) {
      error = "Ancien solde invalide."; return false;
    }
    legacyPrice = doc["pulsesPerCoin"].as<int>();
    int pending = version >= 2 ? doc["pendingPulses"].as<int>() : 0;
    int pendingDivisor = version == 3 ? doc["pendingPulsesPerCoin"].as<int>() : legacyPrice;
    if (!CreditMath::migrate(doc["availableCoins"].as<int>(), legacyPrice, pending, pendingDivisor, restored.availablePieces)) {
      error = "Migration du solde impossible : valeurs invalides ou dépassement."; return false;
    }
  }
  if (!validConfig(restored) || restored.apiToken.length() < 8 || restored.apiToken.length() > 256) {
    error = "Paramètres de sauvegarde invalides."; return false;
  }
  if (!loadUsersFromArray(restored, doc["users"].as<JsonArray>()) ||
      !loadPostsFromArray(restored, doc["posts"].as<JsonArray>(), legacyPrice)) {
    error = "Comptes ou postes invalides."; return false;
  }
  if (doc.containsKey("wifi") && (!doc["wifi"].is<JsonObject>() || !validWifi(doc["wifi"].as<JsonObject>()))) {
    error = "Configuration Wi-Fi invalide."; return false;
  }
  // Les chaînes auth restent compatibles avec les clés historiques NVS.
  JsonDocument users, posts;
  buildUsersArray(users["users"].to<JsonArray>(), restored);
  buildPostsArray(posts["posts"].to<JsonArray>(), restored);
  if (measureJson(users) > 3900 || measureJson(posts) > MAX_POSTS_JSON) {
    error = "Sauvegarde trop volumineuse pour la NVS."; return false;
  }
  return true;
}

// Journal d'import : toutes les écritures sont rejouables. Il est supprimé
// uniquement après vérification de TOUS les domaines, y compris auth et Wi-Fi.
bool applyRestore(JsonDocument& doc, AppState& restored) {
  return saveConfig(restored) && savePosts(restored) && saveCredits(restored) &&
         saveAuth(restored) && saveUsers(restored) && writeWifi(doc["wifi"].as<JsonObject>()) &&
         removeKey("appcfg", "restoreV4");
}

bool resumeRestore(String& error) {
  String json;
  int status = readJsonBlob("restoreV4", json);
  if (!status) return true;
  if (status < 0) { error = "Journal de restauration NVS illisible ; données conservées."; return false; }
  JsonDocument doc;
  AppState restored;
  if (deserializeJson(doc, json) || !parseBackup(doc, restored, error) ||
      doc["version"].as<int>() != 4 || !doc["_localRevenueSequence"].is<uint64_t>() ||
      !doc["wifi"].is<JsonObject>()) {
    error = "Journal de restauration invalide ; reprise suspendue."; return false;
  }
  restored.lastRevenueSequence = doc["_localRevenueSequence"].as<uint64_t>();
  if (!applyRestore(doc, restored)) {
    error = "Restauration NVS en attente ; libérer/rétablir le stockage puis redémarrer."; return false;
  }
  return true;
}
} // namespace

void load(AppState& state) {
  String restoreError;
  bool restoreReady = resumeRestore(restoreError);
  prefs.begin("appcfg", true);
  bool v4 = prefs.isKey("creditsV4");
  int legacyPrice = 1, pending = 0, pendingDivisor = 1;
  bool valid = true;
  if (v4) {
    uint8_t config[32] = {}, credits[40] = {};
    valid = readSnapshot("configV4", "CFG4", config, sizeof(config)) &&
            readSnapshot("creditsV4", "CRD4", credits, sizeof(credits));
    if (valid) {
      state.coinDurationSeconds = RevenueJournal::get(config + 4, 4);
      state.coinCurrency = config[12]; state.pulseValueMinor = RevenueJournal::get(config + 16, 4);
      state.dzdToTndRateMicros = RevenueJournal::get(config + 20, 4);
      state.availablePieces = RevenueJournal::get(credits + 4, 4);
      state.lastRevenueSequence = RevenueJournal::get(credits + 16, 8);
      state.legacyRevenueSequence = RevenueJournal::get(credits + 24, 8);
      valid = !state.legacyRevenueSequence || (state.lastRevenueSequence != UINT64_MAX &&
              state.legacyRevenueSequence == state.lastRevenueSequence + 1);
    }
  } else {
    state.coinDurationSeconds = prefs.getInt("coinDur", AppConfig::DEFAULT_COIN_DURATION_SECONDS);
    legacyPrice = prefs.getInt("pulseCoin", 1);
    state.pulseValueMinor = prefs.getInt("pulseVal", AppConfig::DEFAULT_PULSE_VALUE_MILLIMES);
    int oldCoins = prefs.getInt("availCoins", 0);
    pending = prefs.getInt("pendingPulse", 0); pendingDivisor = legacyPrice;
    if (prefs.isKey("configV3")) {
      uint8_t data[32] = {};
      valid = readSnapshot("configV3", "CFG3", data, sizeof(data));
      if (valid) {
        state.coinDurationSeconds = RevenueJournal::get(data + 4, 4);
        legacyPrice = RevenueJournal::get(data + 8, 4); state.coinCurrency = data[12];
        state.pulseValueMinor = RevenueJournal::get(data + 16, 4);
        state.dzdToTndRateMicros = RevenueJournal::get(data + 20, 4);
        pendingDivisor = legacyPrice;
      }
    }
    if (prefs.isKey("creditsV3")) {
      uint8_t data[40] = {};
      bool ok = readSnapshot("creditsV3", "CRD3", data, sizeof(data));
      valid = valid && ok;
      if (ok) {
        oldCoins = RevenueJournal::get(data + 4, 4); pending = RevenueJournal::get(data + 8, 4);
        pendingDivisor = RevenueJournal::get(data + 12, 4);
        state.lastRevenueSequence = RevenueJournal::get(data + 16, 8);
      }
    }
    valid = valid && CreditMath::migrate(oldCoins, legacyPrice, pending, pendingDivisor, state.availablePieces);
  }
  prefs.end();

  prefs.begin("auth", true);
  state.legacyAdminPassword = prefs.getString("adminPwd", "");
  state.apiToken = prefs.getString("apiToken", "");
  String usersJson = prefs.getString("users", "");
  prefs.end();
  if (!usersJson.isEmpty()) {
    JsonDocument users;
    if (deserializeJson(users, usersJson) || !users["users"].is<JsonArray>() ||
        !loadUsersFromArray(state, users["users"].as<JsonArray>())) valid = false;
  }
  if (state.apiToken.isEmpty()) {
    char token[24]; snprintf(token, sizeof(token), "%08lx%08lx", (unsigned long)esp_random(), (unsigned long)esp_random());
    state.apiToken = token;
    if (restoreReady && !saveAuth(state)) valid = false;
  }

  String postsJson;
  bool postsOk;
  if (v4) postsOk = readJsonBlob("postsV4", postsJson) == 1;
  else {
    prefs.begin("posts", true); postsJson = prefs.getString("items", "[]"); prefs.end();
    postsOk = true;
  }
  JsonDocument posts;
  if (postsOk && !deserializeJson(posts, postsJson)) {
    JsonArray arr = posts.is<JsonArray>() ? posts.as<JsonArray>() : posts["posts"].as<JsonArray>();
    postsOk = !arr.isNull() && loadPostsFromArray(state, arr, v4 ? 0 : legacyPrice);
  } else postsOk = false;
  valid = valid && postsOk && validConfig(state);

  if (!restoreReady) storageFail(state, restoreError);
  else if (!valid) storageFail(state, "Configuration, postes ou solde NVS invalides ; données conservées, affectations suspendues.");

  if (!v4 && state.creditStorageReady) {
    RevenueJournal::Entry transaction;
    int status = loadRevenueTransaction(transaction);
    if (status < 0 || (status == 1 && transaction.sequence > state.lastRevenueSequence &&
        (state.lastRevenueSequence == UINT64_MAX || transaction.sequence != state.lastRevenueSequence + 1))) {
      storageFail(state, "Transaction historique invalide ; migration suspendue.");
    } else if (status == 1 && transaction.sequence > state.lastRevenueSequence) {
      if ((uint64_t)state.availablePieces + transaction.pulses > INT32_MAX)
        storageFail(state, "Transaction historique hors limite ; migration suspendue.");
      else {
        state.legacyRevenueSequence = transaction.sequence;
      }
    }
    // creditsV4 est le marqueur d'engagement atomique. Avant sa présence,
    // toute migration interrompue repart des seules clés historiques intactes.
    if (state.creditStorageReady && (!saveConfig(state) || !savePosts(state) || !saveCredits(state)))
      storageFail(state, "Échec de migration NVS ; anciennes données conservées, redémarrer.");
  }

  String assignmentJson;
  int assignmentStatus = readJsonBlob("assignmentV4", assignmentJson);
  if (assignmentStatus) {
    JsonDocument assignment;
    bool validAssignment = state.creditStorageReady && assignmentStatus == 1 && !deserializeJson(assignment, assignmentJson) &&
      assignment["postId"].is<const char*>() && assignment["before"].is<int>() &&
      assignment["after"].is<int>() && assignment["sequence"].is<uint64_t>() &&
      assignment["before"].as<int>() > assignment["after"].as<int>() && assignment["after"].as<int>() >= 0 &&
      assignment["sequence"].as<uint64_t>() == state.lastRevenueSequence;
    bool committed = validAssignment && assignment["after"].as<int>() == state.availablePieces;
    if (!committed || !clearAssignment()) {
      state.assignmentRecoveryPending = validAssignment &&
        (committed || assignment["before"].as<int>() == state.availablePieces);
      storageFail(state, "Affectation interrompue : vérifier le poste et le débit avant de rétablir les crédits.");
    }
  }
}

bool saveConfig(const AppState& state) {
  if (!validConfig(state)) return false;
  uint8_t data[32] = {'C','F','G','4'};
  RevenueJournal::put(data + 4, state.coinDurationSeconds, 4);
  data[12] = state.coinCurrency;
  RevenueJournal::put(data + 16, state.pulseValueMinor, 4);
  RevenueJournal::put(data + 20, state.dzdToTndRateMicros, 4);
  RevenueJournal::put(data + 30, Money::crc16(data, 30), 2);
  return writeVerified("appcfg", "configV4", data, sizeof(data));
}

bool saveCredits(const AppState& state) {
  if (state.availablePieces < 0) return false;
  uint8_t data[40] = {'C','R','D','4'};
  RevenueJournal::put(data + 4, state.availablePieces, 4);
  RevenueJournal::put(data + 16, state.lastRevenueSequence, 8);
  RevenueJournal::put(data + 24, state.legacyRevenueSequence, 8);
  RevenueJournal::put(data + 38, Money::crc16(data, 38), 2);
  return writeVerified("appcfg", "creditsV4", data, sizeof(data));
}

bool savePosts(const AppState& state) {
  JsonDocument doc;
  for (const auto& post : state.posts) if (post.piecesRequired <= 0) return false;
  buildPostsArray(doc["posts"].to<JsonArray>(), state);
  String json;
  if (doc.overflowed() || measureJson(doc) > MAX_POSTS_JSON || !serializeJson(doc, json)) return false;
  return writeJsonBlob("postsV4", json);
}

bool saveAuth(const AppState& state) { return writeString("auth", "apiToken", state.apiToken); }

bool saveUsers(const AppState& state) {
  JsonDocument doc;
  buildUsersArray(doc["users"].to<JsonArray>(), state);
  String json;
  if (doc.overflowed() || measureJson(doc) > 3900 || !serializeJson(doc, json) || !writeString("auth", "users", json)) return false;
  return removeKey("auth", "adminPwd");
}

bool stageAssignment(const String& postId, int before, int after, uint64_t sequence) {
  JsonDocument doc;
  doc["postId"] = postId; doc["before"] = before; doc["after"] = after; doc["sequence"] = sequence;
  String json; serializeJson(doc, json);
  return writeJsonBlob("assignmentV4", json);
}

bool clearAssignment() { return removeKey("appcfg", "assignmentV4"); }

bool resolveAssignment(AppState& state, bool debit, String& error) {
  String json;
  JsonDocument doc;
  if (!state.assignmentRecoveryPending || state.revenueTransactionPending ||
      readJsonBlob("assignmentV4", json) != 1 || deserializeJson(doc, json) ||
      !doc["before"].is<int>() || !doc["after"].is<int>() || !doc["sequence"].is<uint64_t>() ||
      doc["before"].as<int>() <= doc["after"].as<int>() || doc["after"].as<int>() < 0 ||
      doc["sequence"].as<uint64_t>() != state.lastRevenueSequence) {
    error = "Aucune affectation récupérable ou transaction incompatible."; return false;
  }
  AppState next = state;
  next.availablePieces = doc[debit ? "after" : "before"].as<int>();
  if (!saveCredits(next)) { error = "Échec NVS ; résolution conservée en attente."; return false; }
  state.availablePieces = next.availablePieces;
  if (!clearAssignment()) { error = "Solde enregistré, finalisation NVS en attente."; return false; }
  state.assignmentRecoveryPending = false;
  state.creditStorageReady = true; state.creditStorageError = "";
  return true;
}

int loadRevenueTransaction(RevenueJournal::Entry& entry) {
  if (!prefs.begin("revenue", true)) return 0;
  if (!prefs.isKey("pending")) { prefs.end(); return 0; }
  uint8_t data[RevenueJournal::RECORD_SIZE];
  bool ok = prefs.getBytesLength("pending") == sizeof(data) && prefs.getBytes("pending", data, sizeof(data)) == sizeof(data);
  prefs.end();
  return ok && RevenueJournal::decode(data, entry) && entry.sequence > 0 ? 1 : -1;
}

bool stageRevenueTransaction(const RevenueJournal::Entry& entry) {
  uint8_t data[RevenueJournal::RECORD_SIZE]; RevenueJournal::encode(entry, data);
  return writeVerified("revenue", "pending", data, sizeof(data));
}

bool clearRevenueTransaction() { return removeKey("revenue", "pending"); }

bool exportConfigJson(const AppState& state, String& outputJson) {
  JsonDocument doc;
  buildBackup(state, doc);
  outputJson = "";
  return !doc.overflowed() && measureJson(doc) <= MAX_BACKUP && serializeJson(doc, outputJson) > 0;
}

bool importConfigJson(AppState& state, const String& inputJson, String& error) {
  JsonDocument input;
  AppState restored;
  if (inputJson.length() > MAX_BACKUP || deserializeJson(input, inputJson)) {
    error = "JSON invalide ou sauvegarde trop volumineuse."; return false;
  }
  if (!parseBackup(input, restored, error)) return false;
  if (!state.creditStorageReady || state.revenueTransactionPending) {
    error = "Stockage ou transaction en attente ; restauration refusée."; return false;
  }
  restored.lastRevenueSequence = state.lastRevenueSequence;
  JsonDocument canonical;
  buildBackup(restored, canonical);
  if (input.containsKey("wifi")) canonical["wifi"] = input["wifi"];
  canonical["_localRevenueSequence"] = restored.lastRevenueSequence;
  String json;
  if (canonical.overflowed() || measureJson(canonical) > MAX_BACKUP || !serializeJson(canonical, json)) {
    error = "Sauvegarde trop volumineuse."; return false;
  }
  if (!writeJsonBlob("restoreV4", json)) {
    error = "Préparation NVS impossible ; aucune restauration appliquée. Redémarrer avant de réessayer.";
    return storageFail(state, error);
  }
  if (!applyRestore(canonical, restored)) {
    error = "Restauration préparée en NVS mais incomplète ; redémarrer pour la terminer.";
    return storageFail(state, error);
  }
  state.coinDurationSeconds = restored.coinDurationSeconds; state.coinCurrency = restored.coinCurrency;
  state.pulseValueMinor = restored.pulseValueMinor; state.dzdToTndRateMicros = restored.dzdToTndRateMicros;
  state.availablePieces = restored.availablePieces; state.apiToken = restored.apiToken;
  state.legacyRevenueSequence = 0;
  state.users.swap(restored.users); state.posts.swap(restored.posts);
  state.pendingPosts.clear(); state.sessions.clear();
  return true;
}
}
