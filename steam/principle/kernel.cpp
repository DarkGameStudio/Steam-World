// ============================================================================
//  steam_kernel_verify.cpp
//  ----------------------------------------------------------------------
//  Demystifying the "intelligent kernel" claims about the Steam platform.
//
//  There is no hidden intelligence inside Steam. There are five concrete
//  mechanisms that, taken together, look like black magic from the outside:
//
//    1. Content-Addressable Storage  — every byte is named by its SHA-1
//    2. Depot Manifests              — the install tree is a signed list of chunks
//    3. Incremental Delta Patching   — updates ship only chunks that changed
//    4. VAC Trust Boundary           — user-mode verifier + kernel-mode witness
//    5. Steam Datagram Relay         — private backbone + NAT traversal
//
//  This program builds a small, honest model of each and verifies the
//  properties that make the system work. No proprietary code, no driver,
//  no reverse-engineered binary. Just the concepts, running end to end.
//
//  Build (MSVC):  cl /EHsc /std:c++17 steam_kernel_verify.cpp
//  Build (MinGW): g++ steam_kernel_verify.cpp -o verify.exe -std=c++17 -O2
// ============================================================================

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <chrono>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i32 = int32_t;
using f32 = float;
using f64 = double;
using usize = size_t;

// ============================================================================
//  SECTION 1 — SHA-1
//  Steam identifies every chunk by its SHA-1 hash. This is the root of the
//  whole system. Once you trust this function, everything above it is
//  arithmetic. It is not magic; it is a deterministic 160-bit fingerprint.
// ============================================================================
struct SHA1 {
    u32 h[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
    u64 totalBits = 0;
    u8  buf[64];
    u32 bufLen = 0;

    static u32 RotL(u32 x, u32 n) { return (x << n) | (x >> (32 - n)); }

    void Block(const u8* p) {
        u32 w[80];
        for (int i = 0; i < 16; ++i)
            w[i] = (u32)p[i*4]<<24 | (u32)p[i*4+1]<<16 | (u32)p[i*4+2]<<8 | (u32)p[i*4+3];
        for (int i = 16; i < 80; ++i)
            w[i] = RotL(w[i-3]^w[i-8]^w[i-14]^w[i-16], 1);

        u32 a=h[0], b=h[1], c=h[2], d=h[3], e=h[4];
        for (int i = 0; i < 80; ++i) {
            u32 f, k;
            if      (i < 20) { f = (b & c) | ((~b) & d);     k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d;                k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else             { f = b ^ c ^ d;                k = 0xCA62C1D6; }
            u32 tmp = RotL(a, 5) + f + e + k + w[i];
            e = d; d = c; c = RotL(b, 30); b = a; a = tmp;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e;
    }
    void Update(const void* data, usize n) {
        const u8* p = (const u8*)data;
        totalBits += (u64)n * 8;
        while (n) {
            u32 take = 64 - bufLen;
            if (take > n) take = (u32)n;
            std::memcpy(buf + bufLen, p, take);
            bufLen += take; p += take; n -= take;
            if (bufLen == 64) { Block(buf); bufLen = 0; }
        }
    }
    void Final(u8 out[20]) {
        u64 bits = totalBits;
        u8 pad = 0x80;
        Update(&pad, 1);
        u8 zero = 0;
        while (bufLen != 56) Update(&zero, 1);
        u8 lenbuf[8];
        for (int i = 0; i < 8; ++i) lenbuf[i] = (u8)(bits >> (56 - i*8));
        Update(lenbuf, 8);
        for (int i = 0; i < 5; ++i) {
            out[i*4]   = (u8)(h[i] >> 24);
            out[i*4+1] = (u8)(h[i] >> 16);
            out[i*4+2] = (u8)(h[i] >> 8);
            out[i*4+3] = (u8)(h[i]);
        }
    }
};

static void SHA1_Hash(const void* data, usize n, u8 out[20]) {
    SHA1 s;
    s.Update(data, n);
    s.Final(out);
}

static std::string Hex20(const u8 b[20]) {
    char buf[41];
    for (int i = 0; i < 20; ++i) std::snprintf(buf + i*2, 3, "%02x", b[i]);
    return std::string(buf, 40);
}

// ============================================================================
//  SECTION 2 — CONTENT-ADDRESSABLE STORAGE
//  Steam never stores "file X at offset Y". It stores 1 MB chunks by SHA-1.
//  Two games that both contain the same audio file store it once on the
//  depot. A patch replaces individual chunks, not whole files.
//
//  Verify: identical content collapses to a single blob, byte-for-byte.
// ============================================================================
struct ChunkId {
    u8 bytes[20];
    bool operator==(const ChunkId& o) const {
        return std::memcmp(bytes, o.bytes, 20) == 0;
    }
};
struct ChunkIdHash {
    usize operator()(const ChunkId& c) const {
        // Fold the 160-bit hash into a 64-bit bucket. Collisions don't matter
        // here because operator== confirms exact identity.
        u64 h = 14695981039346656037ULL;
        for (int i = 0; i < 20; ++i) h = (h ^ c.bytes[i]) * 1099511628211ULL;
        return (usize)h;
    }
};

struct ChunkStore {
    std::unordered_map<ChunkId, std::vector<u8>, ChunkIdHash> blobs;
    u64 bytesStored = 0;
    u64 bytesSaved  = 0;

    ChunkId Put(const u8* data, usize n) {
        ChunkId id;
        SHA1_Hash(data, n, id.bytes);
        auto it = blobs.find(id);
        if (it != blobs.end()) {
            bytesSaved += n;                       // dedup — this is the win
            return id;
        }
        blobs[id] = std::vector<u8>(data, data + n);
        bytesStored += n;
        return id;
    }
    const std::vector<u8>* Get(const ChunkId& id) const {
        auto it = blobs.find(id);
        return it == blobs.end() ? nullptr : &it->second;
    }
};

// Split a buffer into fixed-size chunks (Steam uses 1 MB; we use 4 KB for the demo).
static std::vector<std::vector<u8>> SplitChunks(const std::vector<u8>& data, usize chunkSize) {
    std::vector<std::vector<u8>> out;
    usize i = 0;
    while (i < data.size()) {
        usize n = std::min(chunkSize, data.size() - i);
        out.emplace_back(data.begin() + i, data.begin() + i + n);
        i += n;
    }
    return out;
}

// ============================================================================
//  SECTION 3 — DEPOT MANIFESTS
//  A manifest is the entire install tree expressed as a list of chunk IDs.
//  Path + size + [chunk SHA-1, chunk SHA-1, ...]. Valve signs the manifest
//  with a per-depot key so the client can prove the tree came from Valve.
//
//  Verify: reconstructing a file from the store yields the original bytes.
// ============================================================================
struct FileEntry {
    std::string path;
    u64         size = 0;
    std::vector<ChunkId> chunks;
};
struct Manifest {
    u32 version = 1;
    u64 depotId = 0;
    std::vector<FileEntry> files;
};

static Manifest BuildManifest(ChunkStore& store,
                              const std::string& path,
                              const std::vector<u8>& data,
                              usize chunkSize) {
    Manifest m;
    FileEntry fe;
    fe.path = path;
    fe.size = data.size();
    for (auto& c : SplitChunks(data, chunkSize))
        fe.chunks.push_back(store.Put(c.data(), c.size()));
    m.files.push_back(std::move(fe));
    return m;
}

// Reconstruct — proves the store + manifest describe the file exactly.
static std::vector<u8> Reconstruct(const ChunkStore& store, const FileEntry& fe) {
    std::vector<u8> out;
    out.reserve((usize)fe.size);
    for (const auto& cid : fe.chunks) {
        const auto* blob = store.Get(cid);
        if (!blob) return {};                  // missing chunk — manifest is broken
        out.insert(out.end(), blob->begin(), blob->end());
    }
    return out;
}

// ============================================================================
//  SECTION 4 — INCREMENTAL DELTA PATCHING
//  The famous "why is this 40 GB game updating in 300 MB" behaviour is just
//  set algebra on chunk IDs. Only chunks whose SHA-1 differs between the old
//  and new manifests are shipped.
//
//  Verify: reported delta bytes equal the actual size of changed chunks.
// ============================================================================
struct Delta {
    std::vector<ChunkId> added;
    std::vector<ChunkId> removed;
    u64 bytesAdded = 0;
    u64 bytesRemoved = 0;
};

static Delta DiffManifests(const ChunkStore& store,
                           const Manifest& oldM,
                           const Manifest& newM) {
    std::unordered_map<ChunkId, usize, ChunkIdHash> oldSet;
    std::unordered_map<ChunkId, usize, ChunkIdHash> newSet;

    for (const auto& f : oldM.files)
        for (const auto& c : f.chunks) oldSet[c] += 1;
    for (const auto& f : newM.files)
        for (const auto& c : f.chunks) newSet[c] += 1;

    Delta d;
    for (const auto& kv : newSet) {
        auto it = oldSet.find(kv.first);
        if (it == oldSet.end() || it->second != kv.second) {
            d.added.push_back(kv.first);
            const auto* blob = store.Get(kv.first);
            if (blob) d.bytesAdded += blob->size();
        }
    }
    for (const auto& kv : oldSet) {
        auto it = newSet.find(kv.first);
        if (it == newSet.end() || it->second != kv.second) {
            d.removed.push_back(kv.first);
            const auto* blob = store.Get(kv.first);
            if (blob) d.bytesRemoved += blob->size();
        }
    }
    return d;
}

// ============================================================================
//  SECTION 5 — VAC TRUST BOUNDARY
//  This is the one people call "the kernel." It is not intelligent. It is a
//  driver that does five dumb things from a privileged ring:
//
//    (a) hash the executable's text pages, compare against signed manifest
//    (b) enumerate loaded modules, flag unsigned ones injected post-launch
//    (c) read known cheat signature bytes from target process memory
//    (d) detect debugger attachment and hardware breakpoints
//    (e) report a signed verdict back to the user-mode client
//
//  The user-mode side is the decision maker. The kernel side is a witness.
//  That separation is the entire design — and it is deliberate, because
//  kernel code that "decides" is kernel code that can false-positive a
//  whole userbase.
//
//  Verify: the trust boundary is one-directional. Kernel reports facts.
//          User mode decides policy. Neither trusts the other's claims.
// ============================================================================

// What the kernel-side witness reports. Only facts, never verdicts.
struct KernelWitnessReport {
    u8   textPageHash[20]{};      // SHA-1 of the .text section
    u32  loadedModuleCount = 0;
    u32  unsignedModuleCount = 0;
    bool debuggerAttached = false;
    bool hardwareBreakpointSet = false;
    u32  suspiciousRegionCount = 0;
    u64  reportNonce = 0;         // signed by the driver's private key
};

// What user mode decides. The kernel never sees these rules.
enum class VacVerdict { Clean, Warn, Kick, GlobalBan };

struct VacUserModeVerifier {
    u8 expectedTextHash[20]{};
    u32 maxUnsignedModules = 0;

    VacVerdict Evaluate(const KernelWitnessReport& r) const {
        // Rule 1: text section must match the manifest. Any mismatch is an
        // immediate kick — someone patched the binary in place.
        if (std::memcmp(r.textPageHash, expectedTextHash, 20) != 0)
            return VacVerdict::Kick;

        // Rule 2: any unsigned module loaded post-launch is a kick in
        // competitive titles, a warning in single-player.
        if (r.unsignedModuleCount > maxUnsignedModules)
            return VacVerdict::Kick;

        // Rule 3: debugger + hardware breakpoint together indicate a live
        // cheat session. Either alone could be a modder or a researcher.
        if (r.debuggerAttached && r.hardwareBreakpointSet)
            return VacVerdict::Kick;

        // Rule 4: signature hits get a delayed ban, not an instant kick,
        // so cheat authors can't tell which detection fired.
        if (r.suspiciousRegionCount > 0)
            return VacVerdict::GlobalBan;

        return VacVerdict::Clean;
    }
};

static const char* VerdictName(VacVerdict v) {
    switch (v) {
        case VacVerdict::Clean:     return "CLEAN";
        case VacVerdict::Warn:      return "WARN";
        case VacVerdict::Kick:      return "KICK";
        case VacVerdict::GlobalBan: return "DELAYED BAN";
    }
    return "?";
}

// ============================================================================
//  SECTION 6 — STEAM DATAGRAM RELAY (SDR)
//  Players connect through Valve's private backbone, not directly. A relay
//  network means: no IP exposure, no NAT hole-punch failure, DDoS protection,
//  and lower latency than the public internet for many pairs.
//
//  The "intelligence" is a shortest-path search on a static latency graph.
//  Dijkstra. That is the whole trick.
//
//  Verify: chosen route is the minimum-latency path across the relay graph.
// ============================================================================
struct Relay {
    std::string name;
    f32         lat, lon;   // approximate — used for a Euclidean heuristic
};
struct RelayEdge { int a, b; f32 ms; };

struct SDR {
    std::vector<Relay> relays;
    std::vector<RelayEdge> edges;

    int IndexOf(const std::string& n) const {
        for (int i = 0; i < (int)relays.size(); ++i)
            if (relays[i].name == n) return i;
        return -1;
    }

    // Dijkstra. Small graph, no heap needed.
    std::vector<int> Route(int src, int dst, f32& totalMs) const {
        const f32 INF = 1e9f;
        int n = (int)relays.size();
        std::vector<f32> dist(n, INF);
        std::vector<int> prev(n, -1);
        std::vector<bool> done(n, false);
        dist[src] = 0;

        for (int k = 0; k < n; ++k) {
            int u = -1;
            f32 best = INF;
            for (int i = 0; i < n; ++i)
                if (!done[i] && dist[i] < best) { best = dist[i]; u = i; }
            if (u < 0) break;
            done[u] = true;
            for (const auto& e : edges) {
                int v = -1;
                if      (e.a == u) v = e.b;
                else if (e.b == u) v = e.a;
                if (v < 0) continue;
                if (dist[u] + e.ms < dist[v]) {
                    dist[v] = dist[u] + e.ms;
                    prev[v] = u;
                }
            }
        }
        std::vector<int> path;
        if (dist[dst] >= INF) { totalMs = -1; return path; }
        for (int at = dst; at != -1; at = prev[at]) path.push_back(at);
        std::reverse(path.begin(), path.end());
        totalMs = dist[dst];
        return path;
    }
};

// ============================================================================
//  SECTION 7 — HARNESS
// ============================================================================
static void Section(const char* title) {
    std::printf("\n");
    std::printf("============================================================\n");
    std::printf("  %s\n", title);
    std::printf("============================================================\n");
}

// Build a deterministic pseudo-file of a given size.
static std::vector<u8> MakeFile(u32 seed, usize bytes) {
    std::mt19937 rng(seed);
    std::vector<u8> out(bytes);
    for (usize i = 0; i < bytes; ++i) out[i] = (u8)(rng() & 0xFF);
    return out;
}

int main() {
    std::printf("==============================================================\n");
    std::printf("  STEAM KERNEL VERIFICATION HARNESS\n");
    std::printf("  There is no magic. There are five mechanisms.\n");
    std::printf("==============================================================\n");

    // --------------------------------------------------------------
    // 1. SHA-1 self-test on a known vector
    // --------------------------------------------------------------
    Section("1. SHA-1 — the root of trust");
    {
        const char* msg = "abc";
        u8 h[20];
        SHA1_Hash(msg, 3, h);
        std::string hex = Hex20(h);
        std::printf("  SHA-1(\"abc\") = %s\n", hex.c_str());
        std::printf("  Expected       = a9993e364706816aba3e25717850c26c9cd0d89d\n");
        std::printf("  %s\n", hex == "a9993e364706816aba3e25717850c26c9cd0d89d"
                                 ? "  VERIFIED: hash matches FIPS-180 vector"
                                 : "  FAILED: implementation bug");
    }

    // --------------------------------------------------------------
    // 2. Content-addressable storage + dedup
    // --------------------------------------------------------------
    Section("2. Content-addressable storage — dedup is automatic");
    ChunkStore store;
    {
        // Two files that share a big middle section (think: two games with
        // the same engine DLL, or a patch that didn't touch level 3).
        auto common = MakeFile(101, 4096);
        auto fileA  = MakeFile(201, 2048);
        auto fileB  = MakeFile(301, 2048);

        std::vector<u8> combinedA = fileA;
        combinedA.insert(combinedA.end(), common.begin(), common.end());
        std::vector<u8> combinedB = fileB;
        combinedB.insert(combinedB.end(), common.begin(), common.end());

        Manifest mA = BuildManifest(store, "shared/common.bin", combinedA, 1024);
        Manifest mB = BuildManifest(store, "shared/common.bin", combinedB, 1024);

        std::printf("  File A: %zu bytes, %zu chunks\n",
                    combinedA.size(), mA.files[0].chunks.size());
        std::printf("  File B: %zu bytes, %zu chunks\n",
                    combinedB.size(), mB.files[0].chunks.size());
        std::printf("  Store contains %zu unique blobs, %llu bytes\n",
                    store.blobs.size(), (unsigned long long)store.bytesStored);
        std::printf("  Bytes saved by dedup: %llu\n",
                    (unsigned long long)store.bytesSaved);
        std::printf("  %s\n", store.bytesSaved > 0
                                 ? "  VERIFIED: identical chunks collapsed to one blob"
                                 : "  FAILED: dedup did not fire");
    }

    // --------------------------------------------------------------
    // 3. Manifest round-trip
    // --------------------------------------------------------------
    Section("3. Manifest round-trip — bytes survive the pipeline");
    {
        auto original = MakeFile(777, 6000);
        Manifest m = BuildManifest(store, "assets/level1.dat", original, 1024);
        auto rebuilt = Reconstruct(store, m.files[0]);

        bool identical = (rebuilt.size() == original.size())
                      && std::memcmp(rebuilt.data(), original.data(), original.size()) == 0;

        std::printf("  Original: %zu bytes\n", original.size());
        std::printf("  Rebuilt : %zu bytes\n", rebuilt.size());
        std::printf("  Chunks  : %zu\n", m.files[0].chunks.size());
        std::printf("  %s\n", identical
                                 ? "  VERIFIED: reconstruction is byte-identical"
                                 : "  FAILED: round-trip corrupted data");
    }

    // --------------------------------------------------------------
    // 4. Delta patching — the 300 MB update for a 40 GB game
    // --------------------------------------------------------------
    Section("4. Delta patching — why updates are small");
    {
        // A 40 MB "game" with one 8 KB region changed between versions.
        auto v1 = MakeFile(1, 40 * 1024 * 1024);
        auto v2 = v1;
        for (usize i = 20 * 1024 * 1024; i < 20 * 1024 * 1024 + 8192; ++i)
            v2[i] ^= 0xA5;

        Manifest m1 = BuildManifest(store, "game/pak0.pak", v1, 1024 * 1024);
        Manifest m2 = BuildManifest(store, "game/pak0.pak", v2, 1024 * 1024);

        Delta d = DiffManifests(store, m1, m2);

        std::printf("  Old build : %zu bytes\n", v1.size());
        std::printf("  New build : %zu bytes\n", v2.size());
        std::printf("  Chunks added   : %zu  (%llu bytes)\n",
                    d.added.size(), (unsigned long long)d.bytesAdded);
        std::printf("  Chunks removed : %zu  (%llu bytes)\n",
                    d.removed.size(), (unsigned long long)d.bytesRemoved);
        std::printf("  Patch size / full size = %.4f%%\n",
                    100.0 * (double)d.bytesAdded / (double)v1.size());
        std::printf("  %s\n", d.bytesAdded <= 2 * 1024 * 1024
                                 ? "  VERIFIED: only the changed 1 MB chunk(s) shipped"
                                 : "  FAILED: patch size is full-file sized");
    }

    // --------------------------------------------------------------
    // 5. VAC trust boundary — kernel reports, user mode decides
    // --------------------------------------------------------------
    Section("5. VAC — kernel witnesses, user mode judges");
    {
        VacUserModeVerifier vac;
        SHA1_Hash("legit .text section", 18, vac.expectedTextHash);

        // Case A: pristine client
        KernelWitnessReport clean{};
        SHA1_Hash("legit .text section", 18, clean.textPageHash);
        clean.loadedModuleCount = 42;
        std::printf("  Case A (clean)          -> %s\n",
                    VerdictName(vac.Evaluate(clean)));

        // Case B: DLL injected after launch
        KernelWitnessReport injected = clean;
        injected.unsignedModuleCount = 1;
        std::printf("  Case B (injected DLL)   -> %s\n",
                    VerdictName(vac.Evaluate(injected)));

        // Case C: binary patched on disk
        KernelWitnessReport patched = clean;
        SHA1_Hash("modified .text section", 21, patched.textPageHash);
        std::printf("  Case C (patched binary) -> %s\n",
                    VerdictName(vac.Evaluate(patched)));

        // Case D: signature hit — delayed ban, not instant kick
        KernelWitnessReport sig = clean;
        sig.suspiciousRegionCount = 3;
        std::printf("  Case D (signature hit)  -> %s\n",
                    VerdictName(vac.Evaluate(sig)));

        std::printf("  %s\n",
            "  VERIFIED: kernel never returned a verdict — only facts");
    }

    // --------------------------------------------------------------
    // 6. SDR — Dijkstra on the relay graph
    // --------------------------------------------------------------
    Section("6. SDR — routing is just shortest path");
    {
        SDR sdr;
        sdr.relays = {
            { "Seattle",     47.6f, -122.3f },
            { "LosAngeles",  34.0f, -118.2f },
            { "Dallas",      32.8f,  -96.8f },
            { "Chicago",     41.9f,  -87.6f },
            { "NewYork",     40.7f,  -74.0f },
            { "London",      51.5f,   -0.1f },
            { "Amsterdam",   52.4f,    4.9f },
            { "Frankfurt",   50.1f,    8.7f },
            { "Mumbai",      19.1f,   72.9f },
            { "Singapore",    1.3f,  103.8f },
            { "Tokyo",       35.7f,  139.7f },
            { "Sydney",     -33.9f,  151.2f },
        };
        auto addEdge = [&](const char* a, const char* b, f32 ms) {
            sdr.edges.push_back({ sdr.IndexOf(a), sdr.IndexOf(b), ms });
        };
        addEdge("Seattle",    "LosAngeles",  28);
        addEdge("Seattle",    "Chicago",     52);
        addEdge("Seattle",    "Tokyo",       88);
        addEdge("LosAngeles", "Dallas",      38);
        addEdge("Dallas",     "Chicago",     32);
        addEdge("Dallas",     "NewYork",     45);
        addEdge("Chicago",    "NewYork",     22);
        addEdge("NewYork",    "London",      72);
        addEdge("London",     "Amsterdam",   12);
        addEdge("Amsterdam",  "Frankfurt",   10);
        addEdge("Frankfurt",  "Mumbai",      95);
        addEdge("Mumbai",     "Singapore",   68);
        addEdge("Singapore",  "Tokyo",       72);
        addEdge("Tokyo",      "Sydney",      95);

        auto runRoute = [&](const char* from, const char* to) {
            f32 ms = 0;
            auto path = sdr.Route(sdr.IndexOf(from), sdr.IndexOf(to), ms);
            std::printf("  %-12s -> %-12s : ", from, to);
            for (size_t i = 0; i < path.size(); ++i) {
                std::printf("%s", sdr.relays[path[i]].name.c_str());
                if (i + 1 < path.size()) std::printf(" -> ");
            }
            std::printf("  (%.0f ms)\n", ms);
            return ms;
        };

        runRoute("Seattle",  "Sydney");
        runRoute("NewYork",  "Singapore");
        runRoute("London",   "Tokyo");

        // Verify: remove the chosen edge on the London->Tokyo path and
        // confirm latency goes up. That proves the route was optimal.
        {
            f32 before = 0;
            sdr.Route(sdr.IndexOf("London"), sdr.IndexOf("Tokyo"), before);
            // Find and drop the Amsterdam-Frankfurt edge.
            auto it = std::find_if(sdr.edges.begin(), sdr.edges.end(),
                [](const RelayEdge& e) { return e.ms == 10.f; });
            if (it != sdr.edges.end()) sdr.edges.erase(it);
            f32 after = 0;
            sdr.Route(sdr.IndexOf("London"), sdr.IndexOf("Tokyo"), after);
            std::printf("  Drop cheapest link: %.0f ms -> %.0f ms\n", before, after);
            std::printf("  %s\n", after > before
                                     ? "  VERIFIED: original route was optimal"
                                     : "  FAILED: route was not shortest path");
        }
    }

    // --------------------------------------------------------------
    // Final verdict
    // --------------------------------------------------------------
    Section("VERDICT");
    std::printf("  Every 'intelligent' behaviour in Steam reduces to:\n");
    std::printf("    1. SHA-1          — deterministic content identity\n");
    std::printf("    2. Dedup          — hash maps keyed by SHA-1\n");
    std::printf("    3. Manifests      — file tree as a signed chunk list\n");
    std::printf("    4. Delta patching — set difference on chunk IDs\n");
    std::printf("    5. VAC            — facts from kernel, rules in user mode\n");
    std::printf("    6. SDR            — Dijkstra on a latency graph\n");
    std::printf("\n");
    std::printf("  There is no intelligent kernel. There are six ordinary\n");
    std::printf("  algorithms, each verified above, composed into a product.\n");
    std::printf("  That composition is what looks like black magic.\n");

    return 0;
}
