// ============================================================
// OSM Map Viewer — GLFW + OpenGL3 + ImGui + OpenStreetMap tiles
// macOS (OpenGL 3.2 core). Uses libcurl + stb_image.
// ============================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <cmath>
#include <map>
#include <vector>
#include <memory>
#include <sstream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <algorithm>
#include <chrono>

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

// OpenGL loader (glad)
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

// GLFW
#include <GLFW/glfw3.h>

// ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// stb_image (PNG decode)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// libcurl
#include <curl/curl.h>

// ─────────────────────────────────────────────
//  Geo math helpers
// ─────────────────────────────────────────────
constexpr double PI = 3.14159265358979323846;
static inline double deg2rad(double d) { return d * PI / 180.0; }
static inline double rad2deg(double r) { return r * 180.0 / PI; }

// lat/lon → tile x,y at given zoom (WebMercator)
static inline int lon2tile(double lon, int z) {
    return (int)std::floor((lon + 180.0) / 360.0 * (1 << z));
}
static inline int lat2tile(double lat, int z) {
    double lr = deg2rad(lat);
    return (int)std::floor((1.0 - std::log(std::tan(lr) + 1.0 / std::cos(lr)) / PI) / 2.0 * (1 << z));
}

// ─────────────────────────────────────────────
//  Tile identifier
// ─────────────────────────────────────────────
struct TileKey {
    int x=0, y=0, z=0;
    bool operator<(const TileKey& o) const {
        if (z != o.z) return z < o.z;
        if (x != o.x) return x < o.x;
        return y < o.y;
    }
    bool operator==(const TileKey& o) const { return x==o.x && y==o.y && z==o.z; }
    std::string url(const std::string& server) const {
        return server + std::to_string(z) + "/" + std::to_string(x) + "/" + std::to_string(y) + ".png";
    }
};

// ─────────────────────────────────────────────
//  OpenGL texture wrapper
// ─────────────────────────────────────────────
struct TileTexture {
    GLuint id = 0;
    bool   ready = false;
    bool   failed = false;
    int    w = 0, h = 0;

    // raw RGBA pixels waiting for upload (GL thread)
    std::vector<unsigned char> pixels;
};

// ─────────────────────────────────────────────
//  Curl write callback
// ─────────────────────────────────────────────
struct CurlBuffer { std::vector<unsigned char> data; };
static size_t curlWrite(void* ptr, size_t sz, size_t nmemb, void* ud) {
    auto* buf = (CurlBuffer*)ud;
    size_t total = sz * nmemb;
    const unsigned char* p = (const unsigned char*)ptr;
    buf->data.insert(buf->data.end(), p, p + total);
    return total;
}

// ─────────────────────────────────────────────
//  Tile cache & download manager
// ─────────────────────────────────────────────
class TileManager {
public:
    static constexpr const char* TILE_SERVERS[] = {
        "https://tile.openstreetmap.org/",
        "https://a.tile.openstreetmap.org/",
        "https://b.tile.openstreetmap.org/",
    };

    TileManager() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~TileManager() {
        running = false;
        curl_global_cleanup();
        // Note: worker threads are detached; they will exit quickly after running=false.
    }

    // Called from GL thread
    std::shared_ptr<TileTexture> get(const TileKey& k) {
        {
            std::lock_guard<std::mutex> lk(cacheMutex);
            auto it = cache.find(k);
            if (it != cache.end()) return it->second;

            // create placeholder + mark in-flight
            auto t = std::make_shared<TileTexture>();
            cache[k] = t;
        }

        // Enqueue for download
        {
            std::lock_guard<std::mutex> qlk(queueMutex);
            downloadQueue.push(k);
        }
        spawnWorkerIfNeeded();
        return cacheGetNoCreate(k);
    }

    // Upload any decoded tiles to GL (call from GL thread each frame)
    void processUploads() {
        std::vector<std::pair<TileKey, std::shared_ptr<TileTexture>>> work;
        {
            std::lock_guard<std::mutex> lk(uploadMutex);
            work.swap(pendingUpload);
        }
        for (auto& [key, tex] : work) {
            if (!tex || tex->pixels.empty()) continue;

            glGenTextures(1, &tex->id);
            glBindTexture(GL_TEXTURE_2D, tex->id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->w, tex->h, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, tex->pixels.data());

            tex->pixels.clear();
            tex->ready = true;
        }
    }

    // Evict tiles far from current view (must be called on GL thread)
    void evict(int cx, int cy, int zoom, int radius = 10) {
        std::lock_guard<std::mutex> lk(cacheMutex);
        for (auto it = cache.begin(); it != cache.end();) {
            const auto& k = it->first;
            if (k.z != zoom || std::abs(k.x - cx) > radius || std::abs(k.y - cy) > radius) {
                if (it->second && it->second->id) glDeleteTextures(1, &it->second->id);
                it = cache.erase(it);
            } else {
                ++it;
            }
        }
    }

    int queueSize() const {
        std::lock_guard<std::mutex> lk(queueMutex);
        return (int)downloadQueue.size();
    }

    int activeWorkersCount() const { return activeWorkers.load(); }

private:
    mutable std::mutex cacheMutex;
    std::map<TileKey, std::shared_ptr<TileTexture>> cache;

    mutable std::mutex queueMutex;
    std::queue<TileKey> downloadQueue;

    std::mutex uploadMutex;
    std::vector<std::pair<TileKey, std::shared_ptr<TileTexture>>> pendingUpload;

    std::atomic<bool> running{true};
    std::atomic<int>  activeWorkers{0};
    int serverIdx = 0;

    static constexpr int MAX_WORKERS = 6;

    std::shared_ptr<TileTexture> cacheGetNoCreate(const TileKey& k) {
        std::lock_guard<std::mutex> lk(cacheMutex);
        auto it = cache.find(k);
        if (it != cache.end()) return it->second;
        return nullptr;
    }

    void spawnWorkerIfNeeded() {
        int cur = activeWorkers.load();
        if (cur >= MAX_WORKERS) return;

        // bump worker count and spawn
        activeWorkers++;
        std::thread([this]{ workerLoop(); }).detach();
    }

    bool popJob(TileKey& outKey) {
        std::lock_guard<std::mutex> lk(queueMutex);
        if (downloadQueue.empty()) return false;
        outKey = downloadQueue.front();
        downloadQueue.pop();
        return true;
    }

    void workerLoop() {
        while (running.load()) {
            TileKey key{};
            if (!popJob(key)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }

            // Check still needed + not already done
            auto tex = cacheGetNoCreate(key);
            if (!tex || tex->ready || tex->failed) continue;

            // Pick server
            int si = (serverIdx++) % 3;
            std::string url = key.url(TILE_SERVERS[si]);

            // Download
            CurlBuffer buf;
            CURL* curl = curl_easy_init();
            if (!curl) continue;

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "OSM-ImGui-Map/1.0");
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK || buf.data.empty()) {
                if (tex) tex->failed = true;
                continue;
            }

            // Decode PNG OUTSIDE locks
            int w=0, h=0, ch=0;
            unsigned char* pix = stbi_load_from_memory(
                buf.data.data(), (int)buf.data.size(), &w, &h, &ch, 4);
            if (!pix || w <= 0 || h <= 0) {
                if (pix) stbi_image_free(pix);
                if (tex) tex->failed = true;
                continue;
            }

            // Store decoded pixels for GL upload
            tex->w = w; tex->h = h;
            tex->pixels.assign(pix, pix + (size_t)w * (size_t)h * 4);
            stbi_image_free(pix);

            {
                std::lock_guard<std::mutex> ulk(uploadMutex);
                pendingUpload.push_back({key, tex});
            }
        }
        activeWorkers--;
    }
};

constexpr const char* TileManager::TILE_SERVERS[];

// ─────────────────────────────────────────────
//  Marker structure
// ─────────────────────────────────────────────
struct Marker {
    double lat=0, lon=0;
    std::string label;
    ImVec4 color = ImVec4(1, 0.3f, 0.3f, 1);
};

// ─────────────────────────────────────────────
//  Map State
// ─────────────────────────────────────────────
struct MapState {
    double centerLat = 40.7128;   // NYC default
    double centerLon = -74.0060;
    int    zoom      = 13;

    bool   dragging     = false;
    ImVec2 dragStart    = {0,0};
    double dragStartLat = 0, dragStartLon = 0;

    std::vector<Marker> markers;

    // Search
    char searchBuf[256] = "New York, NY";
    std::atomic<bool> geocoding{false};
    std::atomic<bool> geocodeDone{false};
    bool   geocodeOK = false;
    double geocodeLat = 0, geocodeLon = 0;

    std::string toastMsg;
    float toastT = 0;

    // UI toggles
    bool showCoords     = true;
    bool showTileInfo   = false;
    bool showMarkerList = false;
    bool showHelp       = true;
    bool darkMode       = true;

    // Marker placement
    bool placingMarker  = false;
    char markerLabel[64] = "Waypoint";

    // Stats
    int tilesLoaded = 0, tilesLoading = 0;
};

// ─────────────────────────────────────────────
//  Geocode via Nominatim (blocking; call in thread)
// ─────────────────────────────────────────────
static bool geocodeNominatim(const std::string& query, double& outLat, double& outLon) {
    // URL encode minimally (spaces + a few unsafe chars)
    auto urlEncode = [](const std::string& s) {
        std::ostringstream oss;
        for (unsigned char c : s) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c=='-' || c=='_' || c=='.' || c=='~') {
                oss << c;
            } else if (c == ' ') {
                oss << '+';
            } else {
                oss << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)c
                    << std::nouppercase << std::dec;
            }
        }
        return oss.str();
    };

    std::string url = "https://nominatim.openstreetmap.org/search?q=" + urlEncode(query) + "&format=json&limit=1";

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    CurlBuffer buf;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "OSM-ImGui-Map/1.0 (contact: you@example.com)");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 8L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) return false;

    std::string json(buf.data.begin(), buf.data.end());

    auto extract = [&](const std::string& key) -> std::string {
        std::string token = "\"" + key + "\":\"";
        size_t p = json.find(token);
        if (p == std::string::npos) return "";
        p += token.size();
        size_t e = json.find('"', p);
        if (e == std::string::npos) return "";
        return json.substr(p, e - p);
    };

    std::string slat = extract("lat");
    std::string slon = extract("lon");
    if (slat.empty() || slon.empty()) return false;

    outLat = std::stod(slat);
    outLon = std::stod(slon);
    return true;
}

// ─────────────────────────────────────────────
//  UI helpers
// ─────────────────────────────────────────────
static void pushToast(MapState& ms, const std::string& msg, float seconds = 3.0f) {
    ms.toastMsg = msg;
    ms.toastT = seconds;
}

static void drawToast(MapState& ms) {
    if (ms.toastT <= 0.0f || ms.toastMsg.empty()) return;

    ImGuiIO& io = ImGui::GetIO();
    ms.toastT -= io.DeltaTime;

    float alpha = std::min(1.0f, ms.toastT / 0.4f);
    ImGui::SetNextWindowBgAlpha(0.85f * alpha);

    ImVec2 pad(12, 10);
    ImVec2 pos(io.DisplaySize.x - 18, 18);
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(1,0));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoMove;

    ImGui::Begin("##toast", nullptr, flags);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, pad);
    ImGui::TextUnformatted(ms.toastMsg.c_str());
    ImGui::PopStyleVar();
    ImGui::End();
}

// ─────────────────────────────────────────────
//  Render the tile map into current window region
// ─────────────────────────────────────────────
static void renderMap(MapState& ms, TileManager& tm, ImVec2 mapPos, ImVec2 mapSize) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io    = ImGui::GetIO();

    const int tileZ   = ms.zoom;
    const int TILE_PX = 256;
    const int maxTile = (1 << tileZ);

    // Center tile in "tile space" (float)
    double cx = (ms.centerLon + 180.0) / 360.0 * (1 << tileZ);
    double lr = deg2rad(ms.centerLat);
    double cy = (1.0 - std::log(std::tan(lr) + 1.0/std::cos(lr)) / PI) / 2.0 * (1 << tileZ);

    double tileFracX = cx - std::floor(cx);
    double tileFracY = cy - std::floor(cy);
    int tileOriginX  = (int)std::floor(cx);
    int tileOriginY  = (int)std::floor(cy);

    // pixel center of view
    float pcx = mapPos.x + mapSize.x * 0.5f;
    float pcy = mapPos.y + mapSize.y * 0.5f;

    int tilesW = (int)std::ceil(mapSize.x / TILE_PX) + 2;
    int tilesH = (int)std::ceil(mapSize.y / TILE_PX) + 2;

    // tileOriginX+tileFracX should align to pcx
    float offsetX = (float)(pcx - tileFracX * TILE_PX);
    float offsetY = (float)(pcy - tileFracY * TILE_PX);

    dl->PushClipRect(mapPos, ImVec2(mapPos.x + mapSize.x, mapPos.y + mapSize.y), true);

    // background
    dl->AddRectFilled(mapPos, ImVec2(mapPos.x + mapSize.x, mapPos.y + mapSize.y),
                      ms.darkMode ? IM_COL32(22,26,34,255) : IM_COL32(210,220,230,255));

    int loadedCount = 0, loadingCount = 0;

    // draw tiles
    for (int dy = -tilesH/2; dy <= tilesH/2 + 1; dy++) {
        for (int dx = -tilesW/2; dx <= tilesW/2 + 1; dx++) {
            int tx = ((tileOriginX + dx) % maxTile + maxTile) % maxTile;
            int ty = tileOriginY + dy;
            if (ty < 0 || ty >= maxTile) continue;

            TileKey key{tx, ty, tileZ};
            auto tex = tm.get(key);

            float px = offsetX + dx * TILE_PX;
            float py = offsetY + dy * TILE_PX;

            if (tex && tex->ready && tex->id) {
                loadedCount++;
                dl->AddImage((ImTextureID)(uintptr_t)tex->id, ImVec2(px, py), ImVec2(px + TILE_PX, py + TILE_PX));
                if (ms.darkMode) dl->AddRectFilled(ImVec2(px, py), ImVec2(px+TILE_PX, py+TILE_PX), IM_COL32(0,10,30,70));
            } else if (tex && tex->failed) {
                dl->AddRectFilled(ImVec2(px,py), ImVec2(px+TILE_PX,py+TILE_PX), IM_COL32(70,30,30,180));
                dl->AddText(ImVec2(px+8, py+8), IM_COL32(255,140,140,255), "tile error");
            } else {
                loadingCount++;
                float t = (float)ImGui::GetTime() * 0.8f;
                unsigned char pulse = (unsigned char)(120 + 60 * std::sin(t + dx + dy));
                dl->AddRectFilled(ImVec2(px,py), ImVec2(px+TILE_PX,py+TILE_PX), IM_COL32(30,35,45,200));
                float shimmerY = py + std::fmod(t * 80.0f, (float)TILE_PX);
                dl->AddLine(ImVec2(px, shimmerY), ImVec2(px+TILE_PX, shimmerY), IM_COL32(80,120,180,pulse), 1.5f);
            }

            if (ms.showTileInfo) {
                dl->AddRect(ImVec2(px,py), ImVec2(px+TILE_PX,py+TILE_PX), IM_COL32(255,255,0,50));
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%d/%d/%d", tileZ, tx, ty);
                dl->AddText(ImVec2(px+4, py+4), IM_COL32(255,255,0,180), buf);
            }
        }
    }

    ms.tilesLoaded  = loadedCount;
    ms.tilesLoading = loadingCount;

    // markers
    for (auto& mk : ms.markers) {
        double mfx = (mk.lon + 180.0) / 360.0 * (1 << tileZ);
        double mlr = deg2rad(mk.lat);
        double mfy = (1.0 - std::log(std::tan(mlr) + 1.0/std::cos(mlr)) / PI) / 2.0 * (1 << tileZ);

        float mpx = offsetX + (float)((mfx - tileOriginX) * TILE_PX);
        float mpy = offsetY + (float)((mfy - tileOriginY) * TILE_PX);

        if (mpx < mapPos.x - 30 || mpx > mapPos.x + mapSize.x + 30) continue;
        if (mpy < mapPos.y - 30 || mpy > mapPos.y + mapSize.y + 30) continue;

        ImU32 col = ImGui::ColorConvertFloat4ToU32(mk.color);
        dl->AddCircleFilled(ImVec2(mpx+2, mpy+2), 9, IM_COL32(0,0,0,80));
        dl->AddCircleFilled(ImVec2(mpx, mpy), 9, col);
        dl->AddCircle(ImVec2(mpx, mpy), 9, IM_COL32(255,255,255,200), 0, 1.5f);
        dl->AddCircleFilled(ImVec2(mpx, mpy), 3, IM_COL32(255,255,255,255));
        dl->AddLine(ImVec2(mpx, mpy+9), ImVec2(mpx, mpy+18), col, 2.0f);

        if (!mk.label.empty()) {
            ImVec2 ts = ImGui::CalcTextSize(mk.label.c_str());
            float lx = mpx + 12, ly = mpy - 8;
            dl->AddRectFilled(ImVec2(lx-4, ly-3), ImVec2(lx+ts.x+4, ly+ts.y+3), IM_COL32(0,0,0,160), 5);
            dl->AddText(ImVec2(lx, ly), IM_COL32(255,255,255,255), mk.label.c_str());
        }
    }

    // crosshair
    float crossSize = 14;
    ImU32 crossCol  = ms.darkMode ? IM_COL32(120,220,255,220) : IM_COL32(0,80,200,220);
    dl->AddLine(ImVec2(pcx - crossSize, pcy), ImVec2(pcx + crossSize, pcy), crossCol, 1.5f);
    dl->AddLine(ImVec2(pcx, pcy - crossSize), ImVec2(pcx, pcy + crossSize), crossCol, 1.5f);
    dl->AddCircle(ImVec2(pcx, pcy), 4, crossCol, 0, 1.5f);

    dl->PopClipRect();

    // input handling in map window
    bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    if (hovered) {
        // Right-click context menu
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ImGui::OpenPopup("MapCtxMenu");

        // Scroll zoom
        if (io.MouseWheel != 0.0f && !io.WantCaptureMouse) {
            int newZ = ms.zoom + (io.MouseWheel > 0 ? 1 : -1);
            ms.zoom = std::max(2, std::min(19, newZ));
        }

        // Left click: place marker if in placement mode; else start pan drag
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.WantCaptureMouse) {
            if (ms.placingMarker) {
                ImVec2 mpos = io.MousePos;

                // screen -> tile float
                double fx = (mpos.x - offsetX) / TILE_PX;
                double fy = (mpos.y - offsetY) / TILE_PX;

                // tile float -> lon/lat
                double mlon = fx / (1 << tileZ) * 360.0 - 180.0;
                double n = PI - 2.0 * PI * fy / (1 << tileZ);
                double mlat = rad2deg(std::atan(0.5 * (std::exp(n) - std::exp(-n))));

                Marker mk;
                mk.lat = mlat;
                mk.lon = mlon;
                mk.label = ms.markerLabel;

                static ImVec4 colors[] = {
                    {1,0.3f,0.3f,1},{0.3f,0.85f,0.3f,1},{0.3f,0.6f,1,1},
                    {1,0.8f,0,1},{0.8f,0.4f,1,1},{1,0.6f,0,1}
                };
                mk.color = colors[ms.markers.size() % 6];

                ms.markers.push_back(mk);
                ms.placingMarker = false;
                pushToast(ms, "Marker placed: " + std::string(ms.markerLabel));
            } else {
                ms.dragging = true;
                ms.dragStart = io.MousePos;
                ms.dragStartLat = ms.centerLat;
                ms.dragStartLon = ms.centerLon;
            }
        }

        // Drag pan (WebMercator inverse for latitude)
        if (ms.dragging && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !io.WantCaptureMouse) {
            float dx = io.MousePos.x - ms.dragStart.x;
            float dy = io.MousePos.y - ms.dragStart.y;

            double lonPerPx = 360.0 / (TILE_PX * (1 << tileZ));
            ms.centerLon = ms.dragStartLon - dx * lonPerPx;

            // latitude via mercator y offset
            double latR0 = deg2rad(ms.dragStartLat);
            double fy0 = (1.0 - std::log(std::tan(latR0) + 1.0/std::cos(latR0)) / PI) / 2.0 * (1 << tileZ);
            double fy1 = fy0 - (double)dy / TILE_PX;
            double n = PI - 2.0 * PI * fy1 / (1 << tileZ);
            ms.centerLat = rad2deg(std::atan(0.5 * (std::exp(n) - std::exp(-n))));

            ms.centerLat = std::max(-85.0, std::min(85.0, ms.centerLat));
            ms.centerLon = std::fmod(ms.centerLon + 540.0, 360.0) - 180.0;
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) ms.dragging = false;
    }

    // Context menu
    if (ImGui::BeginPopup("MapCtxMenu")) {
        if (ImGui::MenuItem("Place Marker Here")) {
            ms.placingMarker = true;
            pushToast(ms, "Click map to place marker");
        }
        if (ImGui::MenuItem("Copy Center Coords")) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.6f, %.6f", ms.centerLat, ms.centerLon);
            ImGui::SetClipboardText(buf);
            pushToast(ms, "Copied: " + std::string(buf));
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Zoom In"))  ms.zoom = std::min(19, ms.zoom+1);
        if (ImGui::MenuItem("Zoom Out")) ms.zoom = std::max(2,  ms.zoom-1);
        ImGui::EndPopup();
    }
}

// ─────────────────────────────────────────────
//  GLFW error callback
// ─────────────────────────────────────────────
static void glfwErrCB(int e, const char* d) {
    std::cerr << "GLFW error " << e << ": " << d << "\n";
}

// ─────────────────────────────────────────────
//  Main
// ─────────────────────────────────────────────
int main() {
    glfwSetErrorCallback(glfwErrCB);
    if (!glfwInit()) return 1;

    // macOS: OpenGL 3.2 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(1400, 900, "OSM Map Viewer", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Load OpenGL function pointers// ImGui init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 10.0f;
    style.FrameRounding     = 7.0f;
    style.PopupRounding     = 9.0f;
    style.ScrollbarRounding = 7.0f;
    style.GrabRounding      = 7.0f;
    style.WindowBorderSize  = 0.0f;
    style.FrameBorderSize   = 0.0f;
    style.ItemSpacing       = ImVec2(8, 6);
    style.FramePadding      = ImVec2(10, 6);
    style.WindowPadding     = ImVec2(14, 12);

    // Tactical-ish theme
    auto& c = style.Colors;
    c[ImGuiCol_WindowBg]     = ImVec4(0.07f, 0.09f, 0.14f, 0.97f);
    c[ImGuiCol_ChildBg]      = ImVec4(0.05f, 0.07f, 0.12f, 0.96f);
    c[ImGuiCol_PopupBg]      = ImVec4(0.08f, 0.10f, 0.16f, 0.98f);
    c[ImGuiCol_FrameBg]      = ImVec4(0.12f, 0.15f, 0.24f, 1.00f);
    c[ImGuiCol_FrameBgHovered]=ImVec4(0.18f, 0.22f, 0.36f, 1.00f);
    c[ImGuiCol_FrameBgActive]= ImVec4(0.22f, 0.28f, 0.44f, 1.00f);
    c[ImGuiCol_Button]       = ImVec4(0.18f, 0.36f, 0.78f, 1.00f);
    c[ImGuiCol_ButtonHovered]= ImVec4(0.24f, 0.48f, 1.00f, 1.00f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.14f, 0.28f, 0.62f, 1.00f);
    c[ImGuiCol_Header]       = ImVec4(0.18f, 0.36f, 0.78f, 0.60f);
    c[ImGuiCol_HeaderHovered]= ImVec4(0.24f, 0.48f, 1.00f, 0.80f);
    c[ImGuiCol_Text]         = ImVec4(0.92f, 0.95f, 1.00f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.52f, 0.68f, 1.00f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    // Optional font (safe fallback if it fails)
    io.Fonts->AddFontDefault();

    TileManager tm;
    MapState ms;

    // sample marker
    ms.markers.push_back({40.7128, -74.0060, "New York City", {0.3f,0.7f,1.0f,1.0f}});
    pushToast(ms, "Right-click map for actions. Press H for help.");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Upload decoded tiles
        tm.processUploads();

        // Handle geocode completion
        if (ms.geocodeDone.load()) {
            ms.geocodeDone = false;
            ms.geocoding = false;
            if (ms.geocodeOK) {
                ms.centerLat = ms.geocodeLat;
                ms.centerLon = ms.geocodeLon;
                ms.zoom = std::max(ms.zoom, 13);
                pushToast(ms, "Navigated to: " + std::string(ms.searchBuf));
            } else {
                pushToast(ms, "Location not found.");
            }
        }

        // Evict distant tiles (GL thread)
        int ctX = lon2tile(ms.centerLon, ms.zoom);
        int ctY = lat2tile(ms.centerLat, ms.zoom);
        tm.evict(ctX, ctY, ms.zoom, 10);

        // Keyboard shortcuts
        if (!io.WantCaptureKeyboard) {
            if (ImGui::IsKeyPressed(ImGuiKey_H)) ms.showHelp = !ms.showHelp;
            if (ImGui::IsKeyPressed(ImGuiKey_M)) ms.showMarkerList = !ms.showMarkerList;
            if (ImGui::IsKeyPressed(ImGuiKey_T)) ms.showTileInfo = !ms.showTileInfo;
            if (ImGui::IsKeyPressed(ImGuiKey_C)) ms.showCoords = !ms.showCoords;
            if (ImGui::IsKeyPressed(ImGuiKey_N)) { ms.darkMode = !ms.darkMode; pushToast(ms, ms.darkMode ? "Night overlay on" : "Night overlay off"); }

            float panAmt = 0.0004f * (1 << (16 - ms.zoom));
            if (ImGui::IsKeyDown(ImGuiKey_LeftArrow)  || ImGui::IsKeyDown(ImGuiKey_A)) ms.centerLon -= panAmt;
            if (ImGui::IsKeyDown(ImGuiKey_RightArrow) || ImGui::IsKeyDown(ImGuiKey_D)) ms.centerLon += panAmt;
            if (ImGui::IsKeyDown(ImGuiKey_UpArrow)    || ImGui::IsKeyDown(ImGuiKey_W)) ms.centerLat = std::min(85.0, ms.centerLat + panAmt * 0.7);
            if (ImGui::IsKeyDown(ImGuiKey_DownArrow)  || ImGui::IsKeyDown(ImGuiKey_S)) ms.centerLat = std::max(-85.0, ms.centerLat - panAmt * 0.7);

            if (ImGui::IsKeyPressed(ImGuiKey_Equal) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd)) ms.zoom = std::min(19, ms.zoom+1);
            if (ImGui::IsKeyPressed(ImGuiKey_Minus) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract)) ms.zoom = std::max(2,  ms.zoom-1);
        }

        // GL frame
        int fw, fh;
        glfwGetFramebufferSize(window, &fw, &fh);
        glViewport(0, 0, fw, fh);
        glClearColor(0.04f, 0.06f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Full-screen host window (no decoration)
        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
        ImGui::Begin("##MapFull", nullptr,
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar();

        // Top control bar
        ImGui::SetCursorPos(ImVec2(14, 12));
        ImGui::BeginGroup();
        ImGui::PushItemWidth(420);

        ImGui::InputTextWithHint("##search", "Search (Nominatim): 'Tempe AZ', 'Eiffel Tower', 'Tokyo Station'...", ms.searchBuf, sizeof(ms.searchBuf));
        ImGui::SameLine();

        bool canSearch = !ms.geocoding.load();
        if (!canSearch) ImGui::BeginDisabled();
        if (ImGui::Button("Go") || (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused() == false)) {
            std::string q = ms.searchBuf;
            if (!q.empty()) {
                ms.geocoding = true;
                pushToast(ms, "Searching: " + q);

                std::thread([q, &ms]{
                    double lat=0, lon=0;
                    bool ok = geocodeNominatim(q, lat, lon);
                    ms.geocodeOK = ok;
                    ms.geocodeLat = lat;
                    ms.geocodeLon = lon;
                    ms.geocodeDone = true;
                }).detach();
            }
        }
        if (!canSearch) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Add Marker")) {
            ms.placingMarker = true;
            pushToast(ms, "Click map to place marker");
        }

        ImGui::SameLine();
        ImGui::Checkbox("Night", &ms.darkMode);
        ImGui::SameLine();
        ImGui::Checkbox("Coords", &ms.showCoords);
        ImGui::SameLine();
        ImGui::Checkbox("Tiles", &ms.showTileInfo);

        ImGui::PopItemWidth();
        ImGui::EndGroup();

        // Map region (leave a top padding for bar)
        ImVec2 mapPos = ImGui::GetWindowPos();
        ImVec2 mapSize = ImGui::GetWindowSize();
        float topBarH = 56.0f;
        mapPos.y += topBarH;
        mapSize.y -= topBarH;

        // Create an invisible child for consistent clipping/hover
        ImGui::SetCursorPos(ImVec2(0, topBarH));
        ImGui::BeginChild("##mapchild", mapSize, false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImVec2 childPos = ImGui::GetWindowPos();
        ImVec2 childSize = ImGui::GetWindowSize();

        renderMap(ms, tm, childPos, childSize);

        ImGui::EndChild();

        // Bottom-left HUD
        ImGui::SetNextWindowBgAlpha(0.55f);
        ImGui::SetNextWindowPos(ImVec2(14, io.DisplaySize.y - 14), ImGuiCond_Always, ImVec2(0,1));
        ImGuiWindowFlags hudFlags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoMove;
        ImGui::Begin("##hud", nullptr, hudFlags);

        if (ms.showCoords) {
            ImGui::Text("Center: %.6f, %.6f  |  Zoom: %d", ms.centerLat, ms.centerLon, ms.zoom);
        }
        ImGui::Text("Tiles: %d ready, %d loading | queue: %d | workers: %d",
                    ms.tilesLoaded, ms.tilesLoading, tm.queueSize(), tm.activeWorkersCount());
        if (ms.placingMarker) {
            ImGui::Separator();
            ImGui::InputText("Label", ms.markerLabel, sizeof(ms.markerLabel));
            ImGui::TextDisabled("Click map to place marker (Esc cancels).");
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) ms.placingMarker = false;
        }
        ImGui::End();

        // Marker list window
        if (ms.showMarkerList) {
            ImGui::SetNextWindowSize(ImVec2(360, 420), ImGuiCond_FirstUseEver);
            ImGui::Begin("Markers", &ms.showMarkerList);
            if (ImGui::Button("Clear All")) {
                ms.markers.clear();
                pushToast(ms, "Cleared markers");
            }
            ImGui::SameLine();
            if (ImGui::Button("Add at Center")) {
                Marker mk;
                mk.lat = ms.centerLat;
                mk.lon = ms.centerLon;
                mk.label = "Center";
                mk.color = ImVec4(0.3f,0.7f,1.0f,1.0f);
                ms.markers.push_back(mk);
                pushToast(ms, "Added marker at center");
            }

            ImGui::Separator();
            for (int i = 0; i < (int)ms.markers.size(); i++) {
                auto& mk = ms.markers[i];
                ImGui::PushID(i);
                ImGui::ColorButton("##c", mk.color, ImGuiColorEditFlags_NoTooltip, ImVec2(12,12));
                ImGui::SameLine();
                ImGui::Text("%s", mk.label.empty() ? "(unnamed)" : mk.label.c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("Go")) {
                    ms.centerLat = mk.lat;
                    ms.centerLon = mk.lon;
                    pushToast(ms, "Jumped to marker");
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("X")) {
                    ms.markers.erase(ms.markers.begin() + i);
                    ImGui::PopID();
                    break;
                }
                ImGui::TextDisabled("  %.6f, %.6f", mk.lat, mk.lon);
                ImGui::PopID();
            }
            ImGui::End();
        }

        // Help overlay
        if (ms.showHelp) {
            ImGui::SetNextWindowBgAlpha(0.70f);
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 14, io.DisplaySize.y - 14), ImGuiCond_Always, ImVec2(1,1));
            ImGui::Begin("##help", nullptr,
                         ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoMove);
            ImGui::Text("Controls");
            ImGui::Separator();
            ImGui::Text("Pan: drag left mouse (or WASD / arrows)");
            ImGui::Text("Zoom: mouse wheel (+ / -)");
            ImGui::Text("Context menu: right click");
            ImGui::Text("Place marker: Add Marker, then click map");
            ImGui::Text("Shortcuts: H help, M markers, T tile grid, C coords, N night");
            ImGui::End();
        }

        drawToast(ms);

        ImGui::End(); // ##MapFull

        // Render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Shutdown
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


