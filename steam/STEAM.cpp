// steam_clone.cpp — a tiny Steam-like launcher in pure Win32 C++
// Build (MinGW):  g++ steam_clone.cpp -o steam.exe -mwindows -O2 -std=c++17 -lgdi32 -luser32
// Build (MSVC):   cl /EHsc /std:c++17 /DUNICODE /D_UNICODE steam_clone.cpp user32.lib gdi32.lib

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <vector>
#include <string>
#include <cstdio>

// ------------------------------------------------------------------ palette
static const COLORREF C_BG     = RGB( 27,  40,  56);
static const COLORREF C_NAV    = RGB( 23,  33,  46);
static const COLORREF C_CARD   = RGB( 36,  52,  71);
static const COLORREF C_CARDH  = RGB( 47,  67,  90);
static const COLORREF C_SEL    = RGB( 45,  66,  90);
static const COLORREF C_TEXT   = RGB(199, 213, 224);
static const COLORREF C_DIM    = RGB(143, 160, 175);
static const COLORREF C_BLUE   = RGB(102, 192, 244);
static const COLORREF C_GREEN  = RGB(117, 176,  34);

// ------------------------------------------------------------------ model
struct Game {
    std::wstring title, genre, desc;
    int      price;      // cents
    double   sizeGB;
    COLORREF accent;
    bool     installed  = false;
    bool     installing = false;
    float    progress   = 0.f;
};

static std::vector<Game> g_games = {
 { L"Neon Drift",        L"Racing",     L"Tear through a rain-slicked megacity at 300 km/h. Build your ride, outrun the law, and chase the perfect line across 40 neon circuits.", 1999,  8.4, RGB(255,  82, 120) },
 { L"Hollow Depths",     L"Roguelike",  L"Descend into a procedurally generated abyss. Every run reshapes the dungeon, every death teaches you something new.",                    2499,  3.2, RGB( 90, 200, 180) },
 { L"Starforge Tactics", L"Strategy",   L"Command a fleet across a living galaxy. Diplomacy, logistics and orbital bombardment — all in real time.",                              3999, 22.1, RGB(120, 140, 255) },
 { L"Pixel Harvest",     L"Simulation", L"A cozy farming sim with a difference: your crops keep growing in real time, even while you are away.",                                   1499,  1.1, RGB(240, 180,  70) },
 { L"Void Runner",       L"Action",     L"Free-to-play parkour shooter. Chain wall-runs, grapple swings and headshots in zero-gravity arenas.",                                    0, 14.7, RGB(180, 110, 240) },
 { L"Chrono Divide",     L"RPG",        L"A 60-hour open-world RPG where every choice forks the timeline. Six endings. One you.",                                                  5999, 48.9, RGB( 80, 200, 120) },
};

// ------------------------------------------------------------------ ui state
enum { H_NONE = 0, H_TAB_STORE = 1, H_TAB_LIB = 2, H_TAB_COMM = 3,
       H_CARD = 100, H_CARD_BTN = 200, H_LIB = 300, H_DETAIL_BTN = 400 };

struct Hit { RECT r; int id; };

static HFONT g_fLogo, g_fHuge, g_fH1, g_fH2, g_fBody, g_fSmall, g_fBtn;
static int   g_cw = 1100, g_ch = 740;
static int   g_view = 0;        // 0 store, 1 library, 2 community
static int   g_sel  = 0;
static int   g_hover = H_NONE;
static bool  g_tracking = false;

static std::vector<Hit>  g_hits;
static std::vector<RECT> g_cards, g_libRows;
static RECT g_tabStore, g_tabLib, g_tabComm, g_detailRect, g_detailBtn;

// ------------------------------------------------------------------ gdi helpers
static COLORREF Blend(COLORREF a, COLORREF b, float t) {
    return RGB((int)(GetRValue(a) * (1 - t) + GetRValue(b) * t),
               (int)(GetGValue(a) * (1 - t) + GetGValue(b) * t),
               (int)(GetBValue(a) * (1 - t) + GetBValue(b) * t));
}

static void Fill(HDC dc, RECT r, COLORREF c) {
    HBRUSH b = CreateSolidBrush(c);
    ::FillRect(dc, &r, b);
    DeleteObject(b);
}

static void FillRound(HDC dc, RECT r, int rad, COLORREF c) {
    HBRUSH b = CreateSolidBrush(c);
    HPEN   p = CreatePen(PS_SOLID, 1, c);
    HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, p);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, rad, rad);
    SelectObject(dc, ob); SelectObject(dc, op);
    DeleteObject(b); DeleteObject(p);
}

static void Text(HDC dc, const std::wstring& s, RECT r, COLORREF c, HFONT f, UINT fmt) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, c);
    HGDIOBJ o = SelectObject(dc, f);
    DrawTextW(dc, s.c_str(), (int)s.size(), &r, fmt);
    SelectObject(dc, o);
}

static void DrawButton(HDC dc, RECT r, const std::wstring& label, COLORREF base, bool hot) {
    FillRound(dc, r, 6, hot ? Blend(base, RGB(255, 255, 255), 0.15f) : base);
    Text(dc, label, r, RGB(255, 255, 255), g_fBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ------------------------------------------------------------------ state helpers
static bool PtIn(const RECT& r, int x, int y) {
    return x >= r.left && x < r.right && y >= r.top && y < r.bottom;
}

static int HitTest(int x, int y) {
    for (int i = (int)g_hits.size() - 1; i >= 0; --i)
        if (PtIn(g_hits[i].r, x, y)) return g_hits[i].id;
    return H_NONE;
}

static std::wstring BtnLabel(const Game& g) {
    if (g.installed)  return L"PLAY";
    if (g.installing) return L"CANCEL";
    if (g.price > 0) {
        wchar_t buf[64];
        swprintf(buf, 64, L"BUY  $%d.%02d", g.price / 100, g.price % 100);
        return buf;
    }
    return L"INSTALL";
}

static COLORREF BtnColor(const Game& g) {
    if (g.installing) return RGB(70, 92, 112);
    if (g.installed || g.price == 0) return C_GREEN;
    return RGB(60, 130, 200);
}

static void PrimaryAction(HWND hwnd, int i) {
    Game& g = g_games[i];
    if (g.installed) {
        MessageBoxW(hwnd, (L"Launching " + g.title + L"...").c_str(), L"Steam", MB_OK | MB_ICONINFORMATION);
    } else if (g.installing) {
        g.installing = false;
        g.progress = 0.f;
    } else {
        g.installing = true;
        g.progress = 0.f;
    }
}

// ------------------------------------------------------------------ layout
static void ComputeLayout() {
    g_hits.clear();
    g_cards.clear();
    g_libRows.clear();

    g_tabStore = { 170, 10, 280, 46 };
    g_tabLib   = { 280, 10, 400, 46 };
    g_tabComm  = { 400, 10, 530, 46 };
    g_hits.push_back({ g_tabStore, H_TAB_STORE });
    g_hits.push_back({ g_tabLib,   H_TAB_LIB   });
    g_hits.push_back({ g_tabComm,  H_TAB_COMM  });

    if (g_view == 0) {
        const int pad = 24, gap = 18, top = 120, cardH = 260;
        int cardW = (g_cw - pad * 2 - gap * 2) / 3;
        for (int i = 0; i < (int)g_games.size(); ++i) {
            int col = i % 3, row = i / 3;
            RECT r = { pad + col * (cardW + gap), top + row * (cardH + gap), 0, 0 };
            r.right  = r.left + cardW;
            r.bottom = r.top + cardH;
            g_cards.push_back(r);
            g_hits.push_back({ r, H_CARD + i });
            RECT b = { r.right - 126, r.bottom - 52, r.right - 16, r.bottom - 16 };
            g_hits.push_back({ b, H_CARD_BTN + i });
        }
    } else if (g_view == 1) {
        int y = 88;
        for (int i = 0; i < (int)g_games.size(); ++i) {
            RECT r = { 32, y, 316, y + 64 };
            g_libRows.push_back(r);
            g_hits.push_back({ r, H_LIB + i });
            y += 70;
        }
        g_detailRect = { 348, 70, g_cw - 24, g_ch - 24 };
        g_detailBtn  = { g_detailRect.left + 24, g_detailRect.bottom - 76,
                         g_detailRect.left + 24 + 210, g_detailRect.bottom - 24 };
        g_hits.push_back({ g_detailBtn, H_DETAIL_BTN });
    }
}

// ------------------------------------------------------------------ drawing
static void DrawNav(HDC dc) {
    Fill(dc, { 0, 0, g_cw, 56 }, C_NAV);
    Text(dc, L"STEAM", { 24, 0, 160, 56 }, C_TEXT, g_fLogo, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    struct Tab { RECT r; const wchar_t* label; int id; int view; };
    Tab tabs[] = {
        { g_tabStore, L"STORE",     H_TAB_STORE, 0 },
        { g_tabLib,   L"LIBRARY",   H_TAB_LIB,   1 },
        { g_tabComm,  L"COMMUNITY", H_TAB_COMM,  2 },
    };
    for (auto& t : tabs) {
        bool active = (g_view == t.view);
        if (active)      FillRound(dc, t.r, 6, C_SEL);
        else if (g_hover == t.id) FillRound(dc, t.r, 6, RGB(30, 44, 60));
        Text(dc, t.label, t.r, active ? C_TEXT : C_DIM, g_fBody,
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

static void DrawCard(HDC dc, int i) {
    const Game& g = g_games[i];
    RECT r = g_cards[i];
    bool hot = (g_hover == H_CARD + i) || (g_hover == H_CARD_BTN + i);

    FillRound(dc, r, 10, hot ? C_CARDH : C_CARD);

    // banner with the accent colour, clipped to the card's rounded corners
    RECT ban = { r.left, r.top, r.right, r.top + 130 };
    HRGN rgn = CreateRoundRectRgn(r.left, r.top, r.right + 1, r.bottom + 1, 20, 20);
    SelectClipRgn(dc, rgn);
    Fill(dc, ban, g.accent);
    Text(dc, std::wstring(1, g.title[0]), ban, Blend(g.accent, RGB(0, 0, 0), 0.30f),
         g_fHuge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectClipRgn(dc, NULL);
    DeleteObject(rgn);

    Text(dc, g.title, { r.left + 16, r.top + 142, r.right - 16, r.top + 170 },
         C_TEXT, g_fH2, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    wchar_t sub[96];
    swprintf(sub, 96, L"%s  ·  %.1f GB", g.genre.c_str(), g.sizeGB);
    Text(dc, sub, { r.left + 16, r.top + 168, r.right - 16, r.top + 190 },
         C_BLUE, g_fSmall, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT btn = { r.right - 126, r.bottom - 52, r.right - 16, r.bottom - 16 };

    if (g.installing) {
        FillRound(dc, btn, 6, RGB(20, 30, 42));
        RECT f = btn;
        f.right = btn.left + (int)((btn.right - btn.left) * g.progress);
        if (f.right > f.left + 4) FillRound(dc, f, 6, C_BLUE);
        wchar_t pct[16];
        swprintf(pct, 16, L"%d%%", (int)(g.progress * 100));
        Text(dc, pct, btn, RGB(255, 255, 255), g_fBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    } else {
        DrawButton(dc, btn, BtnLabel(g), BtnColor(g), g_hover == H_CARD_BTN + i);
    }
}

static void DrawStore(HDC dc) {
    Text(dc, L"FEATURED & RECOMMENDED", { 24, 70, g_cw - 24, 104 },
         C_TEXT, g_fH2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    for (int i = 0; i < (int)g_games.size(); ++i) DrawCard(dc, i);
}

static void DrawLibrary(HDC dc) {
    FillRound(dc, { 24, 70, 324, g_ch - 24 }, 10, C_NAV);

    for (int i = 0; i < (int)g_games.size(); ++i) {
        RECT r = g_libRows[i];
        const Game& g = g_games[i];
        bool sel = (i == g_sel);
        if (sel)              FillRound(dc, r, 8, C_SEL);
        else if (g_hover == H_LIB + i) FillRound(dc, r, 8, RGB(30, 44, 60));

        FillRound(dc, { r.left + 12, r.top + 16, r.left + 44, r.top + 48 }, 6, g.accent);
        Text(dc, g.title, { r.left + 56, r.top + 6, r.right - 12, r.top + 36 },
             C_TEXT, g_fBody, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        std::wstring st = g.installed ? L"Installed"
                        : g.installing ? L"Downloading…" : L"Not installed";
        Text(dc, st, { r.left + 56, r.top + 34, r.right - 12, r.top + 58 },
             C_DIM, g_fSmall, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    const Game& g = g_games[g_sel];
    RECT d = g_detailRect;
    FillRound(dc, d, 10, C_NAV);

    RECT ban = { d.left + 16, d.top + 16, d.right - 16, d.top + 236 };
    HRGN rgn = CreateRoundRectRgn(ban.left, ban.top, ban.right + 1, ban.bottom + 1, 20, 20);
    SelectClipRgn(dc, rgn);
    Fill(dc, ban, g.accent);
    Text(dc, std::wstring(1, g.title[0]), ban, Blend(g.accent, RGB(0, 0, 0), 0.30f),
         g_fHuge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectClipRgn(dc, NULL);
    DeleteObject(rgn);

    int y = ban.bottom + 16;
    Text(dc, g.title, { d.left + 24, y, d.right - 24, y + 40 },
         C_TEXT, g_fH1, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    y += 40;

    wchar_t meta[128];
    swprintf(meta, 128, L"%s   ·   %.1f GB", g.genre.c_str(), g.sizeGB);
    Text(dc, meta, { d.left + 24, y, d.right - 24, y + 24 },
         C_BLUE, g_fSmall, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    y += 32;

    Text(dc, g.desc, { d.left + 24, y, d.right - 24, g_detailBtn.top - 16 },
         C_DIM, g_fBody, DT_LEFT | DT_TOP | DT_WORDBREAK);

    if (g.installing) {
        RECT btn = g_detailBtn;
        FillRound(dc, btn, 6, RGB(20, 30, 42));
        RECT f = btn;
        f.right = btn.left + (int)((btn.right - btn.left) * g.progress);
        if (f.right > f.left + 4) FillRound(dc, f, 6, C_BLUE);
        wchar_t pct[48];
        swprintf(pct, 48, L"DOWNLOADING  %d%%", (int)(g.progress * 100));
        Text(dc, pct, btn, RGB(255, 255, 255), g_fBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    } else {
        DrawButton(dc, g_detailBtn, BtnLabel(g), BtnColor(g), g_hover == H_DETAIL_BTN);
    }
}

static void DrawCommunity(HDC dc) {
    Text(dc, L"COMMUNITY", { 24, 70, g_cw - 24, 104 },
         C_TEXT, g_fH2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    Text(dc, L"Nothing here yet. Go install something.", { 24, 120, g_cw - 24, 160 },
         C_DIM, g_fBody, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

static void Paint(HDC dc) {
    ComputeLayout();
    Fill(dc, { 0, 0, g_cw, g_ch }, C_BG);
    DrawNav(dc);

    if      (g_view == 0) DrawStore(dc);
    else if (g_view == 1) DrawLibrary(dc);
    else                  DrawCommunity(dc);
}

// ------------------------------------------------------------------ window proc
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        auto mk = [](int h, int w) {
            return CreateFontW(h, 0, 0, 0, w, 0, 0, 0, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        };
        g_fLogo  = mk(-20, FW_BOLD);
        g_fHuge  = mk(-96, FW_BOLD);
        g_fH1    = mk(-26, FW_BOLD);
        g_fH2    = mk(-17, FW_SEMIBOLD);
        g_fBody  = mk(-15, FW_NORMAL);
        g_fSmall = mk(-13, FW_NORMAL);
        g_fBtn   = mk(-15, FW_SEMIBOLD);
        SetTimer(hwnd, 1, 50, nullptr);
        return 0;
    }

    case WM_SIZE:
        g_cw = LOWORD(lp);
        g_ch = HIWORD(lp);
        return 0;

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mm = (MINMAXINFO*)lp;
        mm->ptMinTrackSize.x = 940;
        mm->ptMinTrackSize.y = 780;
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        HDC mem = CreateCompatibleDC(dc);
        HBITMAP bmp = CreateCompatibleBitmap(dc, g_cw, g_ch);
        HGDIOBJ old = SelectObject(mem, bmp);

        Paint(mem);

        BitBlt(dc, 0, 0, g_cw, g_ch, mem, 0, 0, SRCCOPY);
        SelectObject(mem, old);
        DeleteObject(bmp);
        DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (!g_tracking) {
            TRACKMOUSEEVENT tme = { sizeof(tme) };
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            g_tracking = true;
        }
        ComputeLayout();
        int id = HitTest((short)LOWORD(lp), (short)HIWORD(lp));
        if (id != g_hover) {
            g_hover = id;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        g_tracking = false;
        if (g_hover != H_NONE) {
            g_hover = H_NONE;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT) {
            POINT p; GetCursorPos(&p); ScreenToClient(hwnd, &p);
            ComputeLayout();
            SetCursor(LoadCursor(nullptr, HitTest(p.x, p.y) ? IDC_HAND : IDC_ARROW));
            return TRUE;
        }
        break;

    case WM_LBUTTONDOWN: {
        ComputeLayout();
        int id = HitTest((short)LOWORD(lp), (short)HIWORD(lp));
        if (id == H_TAB_STORE)      { g_view = 0; }
        else if (id == H_TAB_LIB)   { g_view = 1; }
        else if (id == H_TAB_COMM)  { g_view = 2; }
        else if (id >= H_CARD && id < H_CARD + 100) {
            g_sel = id - H_CARD;
            g_view = 1;
        }
        else if (id >= H_CARD_BTN && id < H_CARD_BTN + 100) {
            PrimaryAction(hwnd, id - H_CARD_BTN);
        }
        else if (id >= H_LIB && id < H_LIB + 100) {
            g_sel = id - H_LIB;
        }
        else if (id == H_DETAIL_BTN) {
            PrimaryAction(hwnd, g_sel);
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_TIMER: {
        bool dirty = false;
        for (auto& g : g_games) {
            if (!g.installing) continue;
            g.progress += 0.008f;
            if (g.progress >= 1.f) {
                g.progress = 1.f;
                g.installing = false;
                g.installed = true;
            }
            dirty = true;
        }
        if (dirty) InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"SteamCloneWnd";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        0, L"SteamCloneWnd", L"Steam",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 820,
        nullptr, nullptr, hInst, nullptr);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}