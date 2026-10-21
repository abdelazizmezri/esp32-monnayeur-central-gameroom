#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <LittleFS.h>
#include "StorageService.h"
#include "PostService.h"
#include "PosteClient.h"
#include "RevenueService.h"
#include "CreditMath.h"
#include <iostream>
#include <stdexcept>

#define CHECK(condition) do { if (!(condition)) throw std::runtime_error(std::string(__func__) + ":" + std::to_string(__LINE__) + " " #condition); } while (0)
namespace FakePost {
inline bool reachable = true;
inline int commands = 0;
inline long duration = 0;
inline String action;
inline bool failStorageAfterCommand = false;
}
namespace PosteClient {
bool sendCommand(const String&, const String& action, long duration) {
  ++FakePost::commands; FakePost::action = action; FakePost::duration = duration;
  if (FakePost::failStorageAfterCommand) FakeNvs::writesLeft = 0;
  return FakePost::reachable;
}
bool configurePost(const String&, const String&, const String&) { return FakePost::reachable; }
bool clearIdentity(const String&) { return FakePost::reachable; }
bool fetchStatus(Post&, uint16_t) { return FakePost::reachable; }
}
namespace LogService {
void info(AppState&, const String&) {}
void warn(AppState&, const String&) {}
void error(AppState&, const String&) {}
}

void reset() {
  FakeNvs::reset(); LittleFS.files.clear(); FakePost::reachable = true;
  FakePost::commands = 0; FakePost::failStorageAfterCommand = false;
}
Post post(const char* id, int price) {
  Post p; p.id = id; p.chipId = id; p.name = id; p.ip = String("192.168.1.") + (id[2] - '0' + 10);
  p.piecesRequired = price; return p;
}
AppState initial() {
  AppState s; StorageService::load(s); CHECK(s.creditStorageReady);
  UserAccount admin; admin.username = "admin"; admin.firstName = "Admin"; admin.lastName = "Test";
  admin.passwordSalt = "salt"; admin.passwordHash = std::string(64, 'a'); admin.role = "admin";
  s.users.push_back(admin);
  CHECK(StorageService::saveUsers(s));
  return s;
}
String encode(JsonDocument& doc) { String json; serializeJson(doc, json); return json; }
JsonDocument backup(AppState& state) {
  String json; CHECK(StorageService::exportConfigJson(state, json));
  JsonDocument doc; CHECK(!deserializeJson(doc, json)); return doc;
}
void seedLegacy(int price, int coins, int pending, int pendingDivisor) {
  Preferences nvs;
  uint8_t cfg[32] = {'C','F','G','3'}, crd[40] = {'C','R','D','3'};
  RevenueJournal::put(cfg+4, 1800, 4); RevenueJournal::put(cfg+8, price, 4);
  RevenueJournal::put(cfg+16, 1000, 4); RevenueJournal::put(cfg+30, Money::crc16(cfg, 30), 2);
  RevenueJournal::put(crd+4, coins, 4); RevenueJournal::put(crd+8, pending, 4);
  RevenueJournal::put(crd+12, pendingDivisor, 4); RevenueJournal::put(crd+38, Money::crc16(crd, 38), 2);
  nvs.begin("appcfg"); nvs.putBytes("configV3",cfg,sizeof(cfg)); nvs.putBytes("creditsV3",crd,sizeof(crd)); nvs.end();
  nvs.begin("posts"); nvs.putString("items", "{\"posts\":[{\"id\":\"ps1\",\"name\":\"PS1\",\"ip\":\"192.168.1.11\",\"chipId\":\"ps1\"}]}"); nvs.end();
}

void assignments() {
  reset(); auto s = initial(); s.availablePieces = 6; s.posts = {post("ps1",1),post("ps2",4),post("ps3",1)};
  CHECK(StorageService::savePosts(s)); CHECK(StorageService::saveCredits(s));
  String error;
  CHECK(PostService::assignCoins(s,"ps2",1,error)); CHECK(s.availablePieces == 2);
  CHECK(FakePost::duration == 1800); CHECK(FakePost::action == "START_SESSION");
  CHECK(s.posts[1].remaining == 1800);
  CHECK(PostService::assignCoins(s,"ps1",1,error)); CHECK(s.availablePieces == 1);
  int sent = FakePost::commands;
  CHECK(!PostService::assignCoins(s,"ps2",1,error)); CHECK(s.availablePieces == 1); CHECK(sent == FakePost::commands);
  CHECK(PostService::assignCoins(s,"ps1",1,error)); CHECK(FakePost::action == "ADD_TIME");
  CHECK(s.posts[0].remaining == 3600); CHECK(s.availablePieces == 0);
  s.availablePieces = 10;
  FakePost::reachable = false;
  CHECK(!PostService::assignCoins(s,"ps3",1,error)); CHECK(s.availablePieces == 10);
  CHECK(!FakeNvs::data["appcfg"].count("assignmentV4"));
  FakePost::reachable = true;
  s.posts[2].remaining = INT32_MAX; s.posts[2].status = "active";
  CHECK(!PostService::assignCoins(s,"ps3",1,error));
  CHECK(!PostService::assignCoins(s,"ps1",INT32_MAX,error));
  CHECK(!PostService::assignCoins(s,"ps1",0,error));
  AppState reboot; StorageService::load(reboot);
  CHECK(reboot.creditStorageReady && reboot.availablePieces == 0 && reboot.posts[1].piecesRequired == 4);
}
void configuration() {
  reset(); auto s = initial(); s.availablePieces = 10; String error;
  PendingPost pending; pending.chipId = "ps2"; pending.ip = "192.168.1.12"; s.pendingPosts.push_back(pending);
  CHECK(PostService::configurePendingPost(s,"ps2","PS2",4,error));
  CHECK(s.posts[0].piecesRequired == 4);
  CHECK(!PostService::updatePost(s,"ps2","PS2",0,error));
  s.posts[0].status = "active"; s.posts[0].remaining = 900;
  FakePost::reachable = false; // Tarif local, aucun accès au poste nécessaire.
  CHECK(PostService::updatePost(s,"ps2","PS2",5,error));
  CHECK(s.posts[0].piecesRequired == 5 && s.posts[0].remaining == 900 && s.availablePieces == 10);
  CHECK(!PostService::updatePost(s,"ps2","Autre nom",5,error));
  Post discovery = s.posts[0]; discovery.ip = "192.168.1.90"; discovery.piecesRequired = 1;
  bool added;
  CHECK(PostService::upsertDiscoveredPost(s,discovery,added,error));
  CHECK(!added && s.posts[0].piecesRequired == 5 && s.posts[0].ip == "192.168.1.90");
  discovery.id = "changed-id";
  CHECK(PostService::upsertDiscoveredPost(s,discovery,added,error)); CHECK(!added && s.posts.size() == 1);
  AppState reboot; StorageService::load(reboot); CHECK(reboot.creditStorageReady && reboot.posts[0].piecesRequired == 5);
}
void migration() {
  reset(); seedLegacy(4,3,2,7);
  AppState s; StorageService::load(s);
  CHECK(s.creditStorageReady && s.availablePieces == 14 && s.posts[0].piecesRequired == 4);
  CHECK(FakeNvs::data["appcfg"].count("configV3"));
  AppState reboot; StorageService::load(reboot); CHECK(reboot.creditStorageReady && reboot.availablePieces == 14);
  for (int writes = 0; writes < 4; ++writes) {
    reset(); seedLegacy(4,3,2,7); FakeNvs::writesLeft = writes;
    AppState interrupted; StorageService::load(interrupted); CHECK(!interrupted.creditStorageReady);
    FakeNvs::writesLeft = -1;
    AppState recovered; StorageService::load(recovered); CHECK(recovered.creditStorageReady && recovered.availablePieces == 14);
  }
  reset(); seedLegacy(4,INT32_MAX,0,4); auto before = FakeNvs::data;
  AppState overflow; StorageService::load(overflow); CHECK(!overflow.creditStorageReady);
  CHECK(!FakeNvs::data["appcfg"].count("creditsV4"));
  reset(); seedLegacy(4,3,4,4);
  AppState bad; StorageService::load(bad); CHECK(!bad.creditStorageReady);
}
void importExport() {
  reset(); auto s = initial(); s.availablePieces = 6; s.posts = {post("ps1",1),post("ps2",4),post("ps3",1)};
  s.lastRevenueSequence = 77;
  String error;
  auto doc = backup(s); CHECK(doc["version"].as<int>() == 4 && !doc.containsKey("pulsesPerCoin"));
  CHECK(StorageService::importConfigJson(s,encode(doc),error));
  AppState reboot; StorageService::load(reboot);
  CHECK(reboot.creditStorageReady && reboot.availablePieces == 6 && reboot.posts[1].piecesRequired == 4 && reboot.lastRevenueSequence == 77);
  auto exported = backup(reboot); CHECK(encode(exported) == encode(doc));
  for (int version = 1; version <= 3; ++version) {
    auto old = backup(s); old["version"] = version; old.remove("creditUnit"); old.remove("availablePieces");
    old["pulsesPerCoin"] = 4; old["availableCoins"] = 3;
    if (version >= 2) { old["pendingPulses"] = 2; old["pulseValueMillimes"] = 1000; }
    if (version == 3) old["pendingPulsesPerCoin"] = 7;
    for (JsonObject post : old["posts"].as<JsonArray>()) post.remove("piecesRequired");
    CHECK(StorageService::importConfigJson(s,encode(old),error));
    CHECK(s.availablePieces == (version == 1 ? 12 : 14));
    for (auto& post : s.posts) CHECK(post.piecesRequired == 4);
  }
  for (const char* invalid : {"0","-1","1.5","2147483648","\"4\"","true","null"}) {
    auto bad = backup(s); JsonDocument value; deserializeJson(value,invalid); bad["posts"][0]["piecesRequired"] = value.as<JsonVariant>();
    auto before = FakeNvs::data; int balance = s.availablePieces;
    CHECK(!StorageService::importConfigJson(s,encode(bad),error)); CHECK(FakeNvs::data == before && s.availablePieces == balance);
  }
  auto bad = backup(s); bad["wifi"]["staticIp"] = "false"; auto before = FakeNvs::data;
  CHECK(!StorageService::importConfigJson(s,encode(bad),error)); CHECK(before == FakeNvs::data);
}
void interruptedImports() {
  reset(); auto s = initial(); s.availablePieces = 2; s.posts = {post("ps1",1)};
  CHECK(StorageService::savePosts(s)); CHECK(StorageService::saveCredits(s));
  auto original = FakeNvs::data;
  auto restore = backup(s); restore["availablePieces"] = 19; restore["posts"][0]["piecesRequired"] = 4;
  restore["coinCurrency"] = "DZD"; restore["pulseValueMinor"] = 10000; restore["dzdToTndRateMicros"] = 20000;
  restore["users"][0]["firstName"] = "Restauré"; restore["apiToken"] = "restored-api-token";
  restore["wifi"]["ssid"] = "Restored wifi";
  String json = encode(restore), error;
  // Chaque frontière NVS : avant préparation, config, postes, solde, auth,
  // utilisateurs, Wi-Fi, suppression du journal.
  for (int writes = 0; writes <= 15; ++writes) {
    FakeNvs::data = original; FakeNvs::writesLeft = writes;
    AppState working = s;
    bool ok = StorageService::importConfigJson(working,json,error);
    bool staged = FakeNvs::data["appcfg"].count("restoreV4");
    if (!ok) CHECK(!working.creditStorageReady && working.availablePieces == 2);
    FakeNvs::writesLeft = -1;
    AppState reboot; StorageService::load(reboot); CHECK(reboot.creditStorageReady);
    CHECK(reboot.availablePieces == (writes ? 19 : 2));
    if (writes) {
      CHECK(reboot.posts[0].piecesRequired == 4 && reboot.apiToken == "restored-api-token");
      CHECK(reboot.users[0].firstName == "Restauré" && reboot.coinCurrency == Money::DZD);
      CHECK(!FakeNvs::data["appcfg"].count("restoreV4"));
      auto roundtrip = backup(reboot); CHECK(roundtrip["wifi"]["ssid"].as<String>() == "Restored wifi");
    } else CHECK(!staged);
  }
}
void interruptedAssignment() {
  reset(); auto s = initial(); s.availablePieces = 6; s.posts = {post("ps2",4)};
  CHECK(StorageService::savePosts(s)); CHECK(StorageService::saveCredits(s));
  FakePost::failStorageAfterCommand = true;
  String error; CHECK(!PostService::assignCoins(s,"ps2",1,error));
  CHECK(s.availablePieces == 2 && !s.creditStorageReady);
  CHECK(!PostService::assignCoins(s,"ps2",1,error));
  FakeNvs::writesLeft = -1;
  AppState reboot; StorageService::load(reboot); CHECK(!reboot.creditStorageReady);
  CHECK(reboot.availablePieces == 6 && reboot.assignmentRecoveryPending);
  CHECK(StorageService::resolveAssignment(reboot,true,error));
  CHECK(reboot.availablePieces == 2 && reboot.creditStorageReady && !reboot.assignmentRecoveryPending);
  AppState resolved; StorageService::load(resolved); CHECK(resolved.availablePieces == 2 && resolved.creditStorageReady);
  reset(); s = initial(); s.availablePieces = 6; s.posts = {post("ps2",4)};
  CHECK(StorageService::savePosts(s)); CHECK(StorageService::saveCredits(s));
  CHECK(StorageService::stageAssignment("ps2",6,2,0)); s.availablePieces = 2;
  CHECK(StorageService::saveCredits(s)); // Coupure avant suppression du marqueur.
  AppState committed; StorageService::load(committed); CHECK(committed.creditStorageReady && committed.availablePieces == 2);
}
void revenue() {
  reset(); seedLegacy(4,3,2,7);
  RevenueJournal::Entry pending;
  pending.sequence = 1; pending.pulses = 1; pending.credits = 0;
  pending.balanceAfter = 3; pending.pendingAfter = 1; pending.pulsesPerCoin = 4;
  pending.nativeAmount = pending.convertedMillimes = 1000; pending.pulseValue = 1000;
  CHECK(StorageService::stageRevenueTransaction(pending));
  AppState s; StorageService::load(s); CHECK(s.availablePieces == 14 && s.legacyRevenueSequence == 1);
  RevenueService::begin(s); CHECK(s.revenueStorageReady && !s.revenueTransactionPending);
  CHECK(s.availablePieces == 15 && s.lastRevenueSequence == 1 && !s.legacyRevenueSequence);
  size_t journalSize = LittleFS.usedBytes();
  CHECK(RevenueService::recoverPending(s)); CHECK(s.availablePieces == 15 && LittleFS.usedBytes() == journalSize);
  CHECK(RevenueService::recordPulses(s,4,Money::DZD,10000,20000,0));
  CHECK(s.availablePieces == 19 && s.lastRevenueSequence == 2);
  auto bytes = *LittleFS.files.at(AppConfig::REVENUE_FILE);
  CHECK(bytes.size() == RevenueJournal::HEADER_SIZE + 2 * RevenueJournal::RECORD_SIZE);
  RevenueJournal::Entry first, second;
  CHECK(RevenueJournal::decode(bytes.data()+RevenueJournal::HEADER_SIZE, first));
  CHECK(first.credits == 0 && first.pulsesPerCoin == 4); // Historique intact.
  CHECK(RevenueJournal::decode(bytes.data()+RevenueJournal::HEADER_SIZE+RevenueJournal::RECORD_SIZE, second));
  CHECK(second.pulses == 4 && second.credits == 4 && second.nativeAmount == 40000 && second.convertedMillimes == 8000);
  AppState reboot; StorageService::load(reboot); RevenueService::begin(reboot);
  CHECK(reboot.creditStorageReady && reboot.availablePieces == 19 && LittleFS.usedBytes() == journalSize + RevenueJournal::RECORD_SIZE);
}
int main() {
  try {
    assignments(); configuration(); migration(); importExport(); interruptedImports(); interruptedAssignment(); revenue();
    std::cout << "PASS: per-post pricing, assignments, restart, migrations V1–V3, V4 round trip, invalid imports, power-loss recovery and revenue\n";
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
