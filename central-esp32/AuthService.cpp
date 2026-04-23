#include "AuthService.h"

namespace AuthService {

  static String generateToken() {
    uint32_t a = (uint32_t)esp_random();
    uint32_t b = (uint32_t)esp_random();
    uint32_t c = (uint32_t)esp_random();
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%08lx%08lx%08lx",
             (unsigned long)a, (unsigned long)b, (unsigned long)c);
    return String(buffer);
  }

  void begin(AppState&) {
  }

  bool login(AppState& state, const String& password) {
    if (password != state.adminPassword) {
      return false;
    }

    state.sessionToken = generateToken();
    return true;
  }

  void logout(AppState& state) {
    state.sessionToken = "";
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

  bool isAuthenticated(WebServer& server, AppState& state) {
    if (state.sessionToken.isEmpty()) return false;
    if (!server.hasHeader("Cookie")) return false;

    String cookie = server.header("Cookie");
    String session = extractCookieValue(cookie, "ESPSESSION");
    return session == state.sessionToken;
  }

  bool hasValidApiAccess(WebServer& server, AppState& state) {
    if (isAuthenticated(server, state)) {
      return true;
    }

    if (!server.hasHeader("Authorization")) {
      return false;
    }

    String auth = server.header("Authorization");
    String prefix = "Bearer ";
    if (!auth.startsWith(prefix)) {
      return false;
    }

    String token = auth.substring(prefix.length());
    return token == state.apiToken;
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

  String getMaskedToken(const AppState& state) {
    if (state.apiToken.length() <= 8) return state.apiToken;
    return state.apiToken.substring(0, 4) + "..." + state.apiToken.substring(state.apiToken.length() - 4);
  }

  String regenerateApiToken(AppState& state) {
    state.apiToken = generateToken();
    return state.apiToken;
  }
}