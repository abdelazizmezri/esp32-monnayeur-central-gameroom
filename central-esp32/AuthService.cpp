#include "AuthService.h"
#include "AppConfig.h"
#include "StorageService.h"

#include <mbedtls/md.h>

namespace AuthService {

  // La valeur JSON reste sous la limite d'une entrée NVS même avec des noms longs.
  static const size_t MAX_USERS = 10;
  static const size_t MAX_SESSIONS = 12;

  static String generateRandomHex(size_t wordCount) {
    String token;
    token.reserve(wordCount * 8);
    for (size_t i = 0; i < wordCount; i++) {
      char part[9];
      snprintf(part, sizeof(part), "%08lx", (unsigned long)esp_random());
      token += part;
    }
    return token;
  }

  static String generateToken() {
    return generateRandomHex(3);
  }

  static String normalizeUsername(const String& value) {
    String result = value;
    result.trim();
    result.toLowerCase();
    return result;
  }

  static String trimmedCopy(const String& value) {
    String result = value;
    result.trim();
    return result;
  }

  static bool validUsername(const String& username) {
    if (username.length() < 3 || username.length() > 32) {
      return false;
    }

    for (size_t i = 0; i < username.length(); i++) {
      char c = username.charAt(i);
      bool allowed = (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') ||
                     c == '.' || c == '_' || c == '-';
      if (!allowed) {
        return false;
      }
    }
    return true;
  }

  static bool validRole(const String& role) {
    return role == "admin" || role == "user";
  }

  static String hashPassword(const String& salt, const String& password) {
    String input = salt + ":" + password;
    unsigned char digest[32];
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!info || mbedtls_md(info,
                            reinterpret_cast<const unsigned char*>(input.c_str()),
                            input.length(), digest) != 0) {
      return "";
    }

    char hex[65];
    for (size_t i = 0; i < sizeof(digest); i++) {
      snprintf(hex + (i * 2), 3, "%02x", digest[i]);
    }
    hex[64] = '\0';
    return String(hex);
  }

  static bool secureEquals(const String& left, const String& right) {
    if (left.length() != right.length()) {
      return false;
    }

    uint8_t difference = 0;
    for (size_t i = 0; i < left.length(); i++) {
      difference |= static_cast<uint8_t>(left.charAt(i) ^ right.charAt(i));
    }
    return difference == 0;
  }

  static void invalidateSessions(AppState& state, const String& username) {
    for (size_t i = 0; i < state.sessions.size();) {
      if (state.sessions[i].username == username) {
        state.sessions.erase(state.sessions.begin() + i);
      } else {
        i++;
      }
    }
  }

  static size_t adminCount(const AppState& state) {
    size_t count = 0;
    for (const auto& user : state.users) {
      if (user.role == "admin") {
        count++;
      }
    }
    return count;
  }

  UserAccount* findUser(AppState& state, const String& username) {
    String normalized = normalizeUsername(username);
    for (auto& user : state.users) {
      if (user.username == normalized) {
        return &user;
      }
    }
    return nullptr;
  }

  bool createUser(AppState& state, const String& username, const String& firstName,
                  const String& lastName, const String& password, const String& role,
                  String& error) {
    String cleanUsername = normalizeUsername(username);
    String cleanFirstName = trimmedCopy(firstName);
    String cleanLastName = trimmedCopy(lastName);
    String cleanRole = trimmedCopy(role);
    cleanRole.toLowerCase();

    if (!validUsername(cleanUsername)) {
      error = "invalid username";
      return false;
    }
    if (cleanFirstName.isEmpty() || cleanLastName.isEmpty() ||
        cleanFirstName.length() > 64 || cleanLastName.length() > 64) {
      error = "invalid name";
      return false;
    }
    bool legacyAdminMigration = state.users.empty() &&
                                cleanUsername == AppConfig::DEFAULT_ADMIN_USERNAME &&
                                cleanRole == "admin" &&
                                !state.legacyAdminPassword.isEmpty() &&
                                password == state.legacyAdminPassword;
    if (password.length() < (legacyAdminMigration ? 4 : 6)) {
      error = "password too short";
      return false;
    }
    if (!validRole(cleanRole)) {
      error = "invalid role";
      return false;
    }
    if (findUser(state, cleanUsername)) {
      error = "username already exists";
      return false;
    }
    if (state.users.size() >= MAX_USERS) {
      error = "user limit reached";
      return false;
    }

    UserAccount user;
    user.username = cleanUsername;
    user.firstName = cleanFirstName;
    user.lastName = cleanLastName;
    user.role = cleanRole;
    user.passwordSalt = generateRandomHex(2);
    user.passwordHash = hashPassword(user.passwordSalt, password);
    if (user.passwordHash.isEmpty()) {
      error = "password hash failed";
      return false;
    }

    state.users.push_back(user);
    StorageService::saveUsers(state);
    return true;
  }

  void begin(AppState& state) {
    if (adminCount(state) == 0) {
      UserAccount* existingAdmin = findUser(state, AppConfig::DEFAULT_ADMIN_USERNAME);
      if (existingAdmin) {
        existingAdmin->role = "admin";
        StorageService::saveUsers(state);
      } else {
        String error;
        String initialPassword = state.legacyAdminPassword.length() >= 4
          ? state.legacyAdminPassword
          : String(AppConfig::DEFAULT_ADMIN_PASSWORD);
        createUser(state, AppConfig::DEFAULT_ADMIN_USERNAME,
                   "Administrateur", "Principal",
                   initialPassword, "admin", error);
      }
    } else if (!state.legacyAdminPassword.isEmpty()) {
      // Termine proprement une éventuelle migration interrompue.
      StorageService::saveUsers(state);
    }

    state.legacyAdminPassword = "";
    StorageService::saveAuth(state);
  }

  bool login(AppState& state, const String& username, const String& password) {
    UserAccount* user = findUser(state, username);
    if (!user || user->passwordSalt.isEmpty() || user->passwordHash.isEmpty()) {
      return false;
    }

    String attemptedHash = hashPassword(user->passwordSalt, password);
    if (attemptedHash.isEmpty() || !secureEquals(attemptedHash, user->passwordHash)) {
      return false;
    }

    if (state.sessions.size() >= MAX_SESSIONS) {
      state.sessions.erase(state.sessions.begin());
    }

    UserSession session;
    session.token = generateToken();
    session.username = user->username;
    state.sessions.push_back(session);
    return true;
  }

  static String extractCookieValue(const String& cookieHeader, const String& key) {
    String pattern = key + "=";
    int start = cookieHeader.indexOf(pattern);
    if (start < 0) return "";

    start += pattern.length();
    int end = cookieHeader.indexOf(';', start);
    if (end < 0) end = cookieHeader.length();

    return cookieHeader.substring(start, end);
  }

  static String requestSessionToken(WebServer& server) {
    if (!server.hasHeader("Cookie")) {
      return "";
    }
    return extractCookieValue(server.header("Cookie"), "ESPSESSION");
  }

  UserAccount* currentUser(WebServer& server, AppState& state) {
    String token = requestSessionToken(server);
    if (token.isEmpty()) {
      return nullptr;
    }

    for (auto& session : state.sessions) {
      if (secureEquals(session.token, token)) {
        return findUser(state, session.username);
      }
    }
    return nullptr;
  }

  void logout(WebServer& server, AppState& state) {
    String token = requestSessionToken(server);
    if (token.isEmpty()) {
      return;
    }

    for (size_t i = 0; i < state.sessions.size(); i++) {
      if (secureEquals(state.sessions[i].token, token)) {
        state.sessions.erase(state.sessions.begin() + i);
        return;
      }
    }
  }

  bool isAuthenticated(WebServer& server, AppState& state) {
    return currentUser(server, state) != nullptr;
  }

  bool isAdmin(WebServer& server, AppState& state) {
    UserAccount* user = currentUser(server, state);
    return user && user->role == "admin";
  }

  static bool hasValidApiToken(WebServer& server, AppState& state) {
    if (state.apiToken.isEmpty() || !server.hasHeader("Authorization")) {
      return false;
    }

    String auth = server.header("Authorization");
    String prefix = "Bearer ";
    if (!auth.startsWith(prefix)) {
      return false;
    }

    return secureEquals(auth.substring(prefix.length()), state.apiToken);
  }

  bool hasValidApiAccess(WebServer& server, AppState& state) {
    return isAuthenticated(server, state) || hasValidApiToken(server, state);
  }

  bool hasAdminAccess(WebServer& server, AppState& state) {
    return isAdmin(server, state) || hasValidApiToken(server, state);
  }

  void requirePageAuth(WebServer& server, AppState& state) {
    if (isAuthenticated(server, state)) {
      return;
    }

    server.sendHeader("Location", "/login", true);
    server.send(302, "text/plain", "");
  }

  bool requireApiAuth(WebServer& server, AppState& state) {
    if (hasValidApiAccess(server, state)) {
      return true;
    }

    server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
    return false;
  }

  bool requireAdminAuth(WebServer& server, AppState& state) {
    if (hasAdminAccess(server, state)) {
      return true;
    }

    if (!hasValidApiAccess(server, state)) {
      server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
    } else {
      server.send(403, "application/json", "{\"error\":\"admin required\"}");
    }
    return false;
  }

  bool updateUser(AppState& state, const String& actorUsername, const String& username,
                  const String& firstName, const String& lastName,
                  const String& password, const String& role, String& error) {
    UserAccount* user = findUser(state, username);
    if (!user) {
      error = "user not found";
      return false;
    }

    String cleanFirstName = trimmedCopy(firstName);
    String cleanLastName = trimmedCopy(lastName);
    String cleanRole = trimmedCopy(role);
    cleanRole.toLowerCase();
    String cleanActor = normalizeUsername(actorUsername);

    if (cleanFirstName.isEmpty() || cleanLastName.isEmpty() ||
        cleanFirstName.length() > 64 || cleanLastName.length() > 64) {
      error = "invalid name";
      return false;
    }
    if (!validRole(cleanRole)) {
      error = "invalid role";
      return false;
    }
    if (!password.isEmpty() && password.length() < 6) {
      error = "password too short";
      return false;
    }
    if (!cleanActor.isEmpty() && user->username == cleanActor && user->role != cleanRole) {
      error = "cannot change own role";
      return false;
    }
    if (!cleanActor.isEmpty() && user->username == cleanActor && !password.isEmpty()) {
      error = "use password settings";
      return false;
    }
    if (user->role == "admin" && cleanRole != "admin" && adminCount(state) <= 1) {
      error = "last admin required";
      return false;
    }

    bool credentialsChanged = user->role != cleanRole || !password.isEmpty();
    String newSalt;
    String newHash;
    if (!password.isEmpty()) {
      newSalt = generateRandomHex(2);
      newHash = hashPassword(newSalt, password);
      if (newHash.isEmpty()) {
        error = "password hash failed";
        return false;
      }
    }

    user->firstName = cleanFirstName;
    user->lastName = cleanLastName;
    user->role = cleanRole;

    if (!password.isEmpty()) {
      user->passwordSalt = newSalt;
      user->passwordHash = newHash;
    }

    String targetUsername = user->username;
    StorageService::saveUsers(state);
    if (credentialsChanged && targetUsername != cleanActor) {
      invalidateSessions(state, targetUsername);
    }
    return true;
  }

  bool deleteUser(AppState& state, const String& actorUsername, const String& username,
                  String& error) {
    String cleanUsername = normalizeUsername(username);
    String cleanActor = normalizeUsername(actorUsername);

    if (!cleanActor.isEmpty() && cleanUsername == cleanActor) {
      error = "cannot delete own account";
      return false;
    }

    for (size_t i = 0; i < state.users.size(); i++) {
      if (state.users[i].username != cleanUsername) {
        continue;
      }
      if (state.users[i].role == "admin" && adminCount(state) <= 1) {
        error = "last admin required";
        return false;
      }

      invalidateSessions(state, cleanUsername);
      state.users.erase(state.users.begin() + i);
      StorageService::saveUsers(state);
      return true;
    }

    error = "user not found";
    return false;
  }

  bool changePassword(AppState& state, const String& username, const String& password,
                      String& error) {
    if (password.length() < 6) {
      error = "password too short";
      return false;
    }

    UserAccount* user = findUser(state, username);
    if (!user) {
      error = "user not found";
      return false;
    }

    user->passwordSalt = generateRandomHex(2);
    user->passwordHash = hashPassword(user->passwordSalt, password);
    if (user->passwordHash.isEmpty()) {
      error = "password hash failed";
      return false;
    }

    String targetUsername = user->username;
    StorageService::saveUsers(state);
    invalidateSessions(state, targetUsername);
    return true;
  }

  String getMaskedToken(const AppState& state) {
    if (state.apiToken.length() <= 8) return state.apiToken;
    return state.apiToken.substring(0, 4) + "..." +
           state.apiToken.substring(state.apiToken.length() - 4);
  }

  String regenerateApiToken(AppState& state) {
    state.apiToken = generateToken();
    return state.apiToken;
  }
}
