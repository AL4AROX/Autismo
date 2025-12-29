// Prefetcher_ULTRA_FIXED.c - Versión Corregida con Animaciones
#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <psapi.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")

/* ==================== CONSTANTES ==================== */
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define ANIMATION_FRAMES 60
#define GLITCH_CHANCE 20

/* ==================== ESTRUCTURAS ==================== */
typedef struct {
    char filename[256];
    char executable[256];
    char signature[32];
    long fileSize;
    int runCount;
    char lastRun[32];
    char path[512];
    char loadedModules[5][256];
    int moduleCount;
    BOOL isRunning;
    DWORD processId;
} PrefetchFile;

typedef struct {
    char name[256];
    DWORD pid;
    char exePath[MAX_PATH];
    SIZE_T memory;
    DWORD threads;
    DWORD handles;
    char commandLine[512];
    char loadedDLLs[10][256];
    int dllCount;
    BOOL isTarget;
    char type[32];
} ProcessInfo;

typedef struct {
    float x, y;
    float dx, dy;
    char text[32];
    COLORREF color;
    int lifetime;
} Particle;

/* ==================== VARIABLES GLOBALES ==================== */
HWND hMainWnd;
PrefetchFile* prefetchFiles = NULL;
ProcessInfo* processes = NULL;
Particle* particles = NULL;

int prefetchCount = 0;
int processCount = 0;
int particleCount = 0;
int currentTab = 0;
int currentView = 0;
BOOL showStartup = TRUE;
float animationProgress = 0.0f;

HFONT hFontConsolas;
HFONT hFontHack;
HFONT hFontSegoe;
HBRUSH hBrushDark;
HBRUSH hBrushBlack;
HPEN hPenGreen;
HPEN hPenCyan;

/* ==================== FUNCIONES DE ANIMACIÓN ==================== */
void CreateParticle(float x, float y, COLORREF color) {
    if (particleCount < 1000) {
        if (!particles) {
            particles = malloc(1000 * sizeof(Particle));
        }
        
        particles[particleCount].x = x;
        particles[particleCount].y = y;
        particles[particleCount].dx = (rand() % 100 - 50) / 50.0f;
        particles[particleCount].dy = (rand() % 100 - 50) / 50.0f;
        particles[particleCount].color = color;
        particles[particleCount].lifetime = 30 + rand() % 60;
        
        const char* texts[] = {"0x", "1x", "FF", "NULL", "VOID", "MEM", "SYS", "RUN", "JMP", "CALL"};
        strcpy_s(particles[particleCount].text, 32, texts[rand() % 10]);
        
        particleCount++;
    }
}

void UpdateParticles() {
    for (int i = 0; i < particleCount; i++) {
        particles[i].x += particles[i].dx;
        particles[i].y += particles[i].dy;
        particles[i].lifetime--;
        
        if (particles[i].lifetime <= 0) {
            particles[i] = particles[--particleCount];
            i--;
        }
    }
}

void DrawParticles(HDC hdc) {
    for (int i = 0; i < particleCount; i++) {
        SetTextColor(hdc, particles[i].color);
        SetBkColor(hdc, RGB(10, 10, 20));
        TextOutA(hdc, (int)particles[i].x, (int)particles[i].y, 
                particles[i].text, (int)strlen(particles[i].text));
    }
}

/* ==================== PANTALLA DE INICIO ==================== */
void DrawStartupScreen(HDC hdc) {
    RECT rc;
    GetClientRect(hMainWnd, &rc);
    
    FillRect(hdc, &rc, hBrushBlack);
    
    for (int y = 0; y < rc.bottom; y += 4) {
        SetPixel(hdc, 0, y, RGB(0, 20, 0));
        SetPixel(hdc, rc.right - 1, y, RGB(0, 20, 0));
    }
    
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFontHack);
    SetTextColor(hdc, RGB(0, 255, 0));
    SetBkColor(hdc, RGB(0, 0, 0));
    
    const char* logoLines[] = {
        "==================================================================================",
        "                                                                                  ",
        "   ██████╗ ██████╗ ███████╗ ██████╗ ████████╗ ██████╗██╗  ██╗███████╗            ",
        "  ██╔═══██╗██╔══██╗██╔════╝██╔═══██╗╚══██╔══╝██╔════╝██║  ██║██╔════╝            ",
        "  ██║   ██║██████╔╝█████╗  ██║   ██║   ██║   ██║     ███████║█████╗              ",
        "  ██║   ██║██╔══██╗██╔══╝  ██║   ██║   ██║   ██║     ██╔══██║██╔══╝              ",
        "  ╚██████╔╝██║  ██║███████╗╚██████╔╝   ██║   ╚██████╗██║  ██║███████╗            ",
        "   ╚═════╝ ╚═╝  ╚═╝╚══════╝ ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚══════╝            ",
        "                                                                                  ",
        "                    P R E F E T C H E R   U L T R A   v 3 . 0                     ",
        "                                                                                  ",
        "=================================================================================="
    };
    
    int startY = rc.bottom / 2 - 150;
    for (int i = 0; i < 12; i++) {
        TextOutA(hdc, rc.right / 2 - 300, startY + i * 20, logoLines[i], (int)strlen(logoLines[i]));
    }
    
    const char* statusMessages[] = {
        "[1/5] Initializing systems...",
        "[2/5] Parsing Prefetch database...",
        "[3/5] Analyzing signatures...",
        "[4/5] Loading process data...",
        "[5/5] Building interface...",
        "READY"
    };
    
    int statusIndex = (int)(animationProgress * 5);
    if (statusIndex > 5) statusIndex = 5;
    
    SetTextColor(hdc, RGB(0, 220, 220));
    TextOutA(hdc, rc.right / 2 - 150, rc.bottom / 2 + 80, statusMessages[statusIndex], 
            (int)strlen(statusMessages[statusIndex]));
    
    int barWidth = 400;
    int barHeight = 20;
    int barX = rc.right / 2 - barWidth / 2;
    int barY = rc.bottom / 2 + 110;
    
    SelectObject(hdc, hPenGreen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, barX, barY, barX + barWidth, barY + barHeight);
    
    int progressWidth = (int)(barWidth * animationProgress);
    RECT progressRect = {barX + 2, barY + 2, barX + 2 + progressWidth, barY + barHeight - 2};
    HBRUSH hProgressBrush = CreateSolidBrush(RGB(0, 180, 0));
    FillRect(hdc, &progressRect, hProgressBrush);
    DeleteObject(hProgressBrush);
    
    char percent[32];
    sprintf_s(percent, 32, "%.0f%%", animationProgress * 100);
    TextOutA(hdc, barX + barWidth + 20, barY, percent, (int)strlen(percent));
    
    if (rand() % GLITCH_CHANCE == 0) {
        int glitchX = rand() % rc.right;
        int glitchY = rand() % rc.bottom;
        int glitchWidth = 50 + rand() % 100;
        int glitchHeight = 2;
        BitBlt(hdc, glitchX, glitchY, glitchWidth, glitchHeight, 
               hdc, glitchX + (rand() % 20 - 10), glitchY, SRCCOPY);
    }
    
    SelectObject(hdc, hOldFont);
}

/* ==================== INTERFAZ PRINCIPAL ==================== */
void DrawMainInterface(HDC hdc) {
    RECT rc;
    GetClientRect(hMainWnd, &rc);
    
    HBRUSH hGradientBrush = CreateSolidBrush(RGB(10, 15, 25));
    FillRect(hdc, &rc, hGradientBrush);
    DeleteObject(hGradientBrush);
    
    for (int y = 0; y < rc.bottom; y += 2) {
        HPEN hLinePen = CreatePen(PS_SOLID, 1, RGB(0, 15 + y % 20, 25));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hLinePen);
        MoveToEx(hdc, 0, y, NULL);
        LineTo(hdc, rc.right, y);
        SelectObject(hdc, hOldPen);
        DeleteObject(hLinePen);
    }
    
    RECT topBar = {0, 0, rc.right, 40};
    HBRUSH hTopBrush = CreateSolidBrush(RGB(20, 25, 35));
    FillRect(hdc, &topBar, hTopBrush);
    DeleteObject(hTopBrush);
    
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFontSegoe);
    SetTextColor(hdc, RGB(0, 255, 200));
    SetBkColor(hdc, RGB(30, 30, 40));
    
    const char* title = "PREFETCHER ULTRA - ADVANCED FORENSIC ANALYSIS";
    TextOutA(hdc, 20, 10, title, (int)strlen(title));
    
    char sysInfo[256];
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);
    
    sprintf_s(sysInfo, 256, "Memory: %.1f%% | Files: %d | Processes: %d | View: %d", 
            (float)memStatus.dwMemoryLoad, prefetchCount, processCount, currentView);
    TextOutA(hdc, rc.right - 450, 10, sysInfo, (int)strlen(sysInfo));
    
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPenCyan);
    MoveToEx(hdc, 0, 42, NULL);
    LineTo(hdc, rc.right, 42);
    SelectObject(hdc, hOldPen);
    
    RECT filterPanel = {10, 50, 250, rc.bottom - 10};
    HBRUSH hFilterBrush = CreateSolidBrush(RGB(20, 25, 35));
    FillRect(hdc, &filterPanel, hFilterBrush);
    DeleteObject(hFilterBrush);
    
    SelectObject(hdc, hPenGreen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, filterPanel.left, filterPanel.top, filterPanel.right, filterPanel.bottom);
    
    SetTextColor(hdc, RGB(0, 255, 150));
    TextOutA(hdc, filterPanel.left + 10, filterPanel.top + 10, "FILTERS & ANALYSIS", 18);
    
    const char* filters[] = {
        "All Prefetch Files",
        "UNSIGNED Only",
        "CHEAT Signatures",
        "RUNDLL32 Instances",
        "CONHOST Processes",
        "JAVAW Executables",
        "Running Processes",
        "Suspicious Files"
    };
    
    int filterY = filterPanel.top + 40;
    for (int i = 0; i < 8; i++) {
        if (i == currentView) {
            RECT filterRect = {filterPanel.left + 5, filterY - 2, 
                              filterPanel.right - 5, filterY + 18};
            HBRUSH hActiveBrush = CreateSolidBrush(RGB(0, 50, 30));
            FillRect(hdc, &filterRect, hActiveBrush);
            DeleteObject(hActiveBrush);
            SetTextColor(hdc, RGB(0, 255, 100));
        } else {
            SetTextColor(hdc, RGB(150, 200, 150));
        }
        
        TextOutA(hdc, filterPanel.left + 15, filterY, filters[i], (int)strlen(filters[i]));
        filterY += 25;
    }
    
    RECT contentPanel = {260, 50, rc.right - 10, rc.bottom - 10};
    HBRUSH hContentBrush = CreateSolidBrush(RGB(15, 20, 25));
    FillRect(hdc, &contentPanel, hContentBrush);
    DeleteObject(hContentBrush);
    
    SelectObject(hdc, hPenGreen);
    Rectangle(hdc, contentPanel.left, contentPanel.top, contentPanel.right, contentPanel.bottom);
    
    SetTextColor(hdc, RGB(0, 200, 255));
    const char* headers[] = {"Filename", "Signature", "Executable", "Size", "Runs", "Status"};
    int headerX = contentPanel.left + 10;
    
    for (int i = 0; i < 6; i++) {
        TextOutA(hdc, headerX, contentPanel.top + 10, headers[i], (int)strlen(headers[i]));
        headerX += (i == 0) ? 250 : 120;
    }
    
    HPEN hHeaderPen = CreatePen(PS_SOLID, 1, RGB(0, 150, 200));
    SelectObject(hdc, hHeaderPen);
    MoveToEx(hdc, contentPanel.left + 5, contentPanel.top + 30, NULL);
    LineTo(hdc, contentPanel.right - 5, contentPanel.top + 30);
    SelectObject(hdc, hOldPen);
    DeleteObject(hHeaderPen);
    
    int dataY = contentPanel.top + 40;
    int itemsToShow = min(15, prefetchCount);
    int itemsShown = 0;
    
    for (int i = 0; i < prefetchCount && itemsShown < itemsToShow; i++) {
        BOOL showItem = TRUE;
        switch (currentView) {
            case 1: showItem = (strcmp(prefetchFiles[i].signature, "UNSIGNED") == 0); break;
            case 2: showItem = (strcmp(prefetchFiles[i].signature, "CHEAT") == 0); break;
            case 3: showItem = (strstr(prefetchFiles[i].executable, "RUNDLL32") != NULL); break;
            case 4: showItem = (strstr(prefetchFiles[i].executable, "CONHOST") != NULL); break;
            case 5: showItem = (strstr(prefetchFiles[i].executable, "JAVAW") != NULL); break;
            case 6: showItem = prefetchFiles[i].isRunning; break;
            case 7: showItem = (strcmp(prefetchFiles[i].signature, "CHEAT") == 0) || 
                               (strcmp(prefetchFiles[i].signature, "UNSIGNED") == 0); break;
        }
        
        if (!showItem) continue;
        
        if (itemsShown % 2 == 0) {
            SetTextColor(hdc, RGB(200, 200, 200));
        } else {
            SetTextColor(hdc, RGB(150, 180, 200));
        }
        
        if (strcmp(prefetchFiles[i].signature, "CHEAT") == 0) {
            SetTextColor(hdc, RGB(255, 50, 50));
        } else if (strcmp(prefetchFiles[i].signature, "UNSIGNED") == 0) {
            SetTextColor(hdc, RGB(255, 150, 50));
        }
        
        int colX = contentPanel.left + 10;
        TextOutA(hdc, colX, dataY, prefetchFiles[i].filename, (int)strlen(prefetchFiles[i].filename));
        colX += 250;
        
        TextOutA(hdc, colX, dataY, prefetchFiles[i].signature, (int)strlen(prefetchFiles[i].signature));
        colX += 120;
        
        TextOutA(hdc, colX, dataY, prefetchFiles[i].executable, (int)strlen(prefetchFiles[i].executable));
        colX += 120;
        
        char sizeStr[32];
        sprintf_s(sizeStr, 32, "%.1f KB", prefetchFiles[i].fileSize / 1024.0f);
        TextOutA(hdc, colX, dataY, sizeStr, (int)strlen(sizeStr));
        colX += 120;
        
        char runsStr[32];
        sprintf_s(runsStr, 32, "%d", prefetchFiles[i].runCount);
        TextOutA(hdc, colX, dataY, runsStr, (int)strlen(runsStr));
        colX += 120;
        
        const char* status = prefetchFiles[i].isRunning ? "RUNNING" : "INACTIVE";
        SetTextColor(hdc, prefetchFiles[i].isRunning ? RGB(0, 255, 0) : RGB(150, 150, 150));
        TextOutA(hdc, colX, dataY, status, (int)strlen(status));
        
        dataY += 25;
        itemsShown++;
        
        if (itemsShown < itemsToShow) {
            HPEN hGrayPen = CreatePen(PS_SOLID, 1, RGB(50, 60, 70));
            SelectObject(hdc, hGrayPen);
            MoveToEx(hdc, contentPanel.left + 5, dataY - 2, NULL);
            LineTo(hdc, contentPanel.right - 5, dataY - 2);
            SelectObject(hdc, hOldPen);
            DeleteObject(hGrayPen);
        }
    }
    
    RECT detailPanel = {contentPanel.left, contentPanel.bottom - 120, 
                       contentPanel.right, contentPanel.bottom};
    HBRUSH hDetailBrush = CreateSolidBrush(RGB(25, 30, 40));
    FillRect(hdc, &detailPanel, hDetailBrush);
    DeleteObject(hDetailBrush);
    
    SelectObject(hdc, hPenGreen);
    Rectangle(hdc, detailPanel.left, detailPanel.top, detailPanel.right, detailPanel.bottom);
    
    SetTextColor(hdc, RGB(0, 220, 220));
    TextOutA(hdc, detailPanel.left + 10, detailPanel.top + 5, "DETAILED PROCESS ANALYSIS", 25);
    
    if (prefetchCount > 0) {
        SetTextColor(hdc, RGB(200, 220, 255));
        
        char details[512];
        sprintf_s(details, 512, "File: %s | Executable: %s", 
                prefetchFiles[0].filename, 
                prefetchFiles[0].executable);
        TextOutA(hdc, detailPanel.left + 10, detailPanel.top + 25, details, (int)strlen(details));
        
        sprintf_s(details, 512, "Path: %s", prefetchFiles[0].path);
        TextOutA(hdc, detailPanel.left + 10, detailPanel.top + 45, details, (int)strlen(details));
        
        sprintf_s(details, 512, "Loaded Modules: %d | Last Run: %s | PID: %lu",
                prefetchFiles[0].moduleCount,
                prefetchFiles[0].lastRun,
                prefetchFiles[0].processId);
        TextOutA(hdc, detailPanel.left + 10, detailPanel.top + 65, details, (int)strlen(details));
    }
    
    UpdateParticles();
    DrawParticles(hdc);
    
    SelectObject(hdc, hOldFont);
}

/* ==================== CARGA DE DATOS ==================== */
void LoadSampleData() {
    if (prefetchFiles) {
        free(prefetchFiles);
        prefetchFiles = NULL;
    }
    
    if (processes) {
        free(processes);
        processes = NULL;
    }
    
    prefetchCount = 15;
    prefetchFiles = malloc(prefetchCount * sizeof(PrefetchFile));
    
    struct {
        const char* filename;
        const char* executable;
        const char* signature;
        const char* path;
        BOOL isRunning;
        DWORD pid;
    } sampleData[] = {
        {"RUNDLL32.EXE-426DCC9E.pf", "RUNDLL32.EXE", "CHEAT", "C:\\Windows\\System32\\rundll32.exe", TRUE, 4567},
        {"CONHOST.EXE-94136077.pf", "CONHOST.EXE", "SIGNED", "C:\\Windows\\System32\\conhost.exe", TRUE, 7890},
        {"JAVAW.EXE-49E8B208.pf", "JAVAW.EXE", "SIGNED", "C:\\Program Files\\Java\\bin\\javaw.exe", FALSE, 0},
        {"PREFETCHER.EXE-F8EC3F27.pf", "PREFETCHER.EXE", "UNSIGNED", "C:\\Tools\\Prefetcher.exe", TRUE, GetCurrentProcessId()},
        {"SMARTSCREEN.EXE-EACC1250.pf", "SMARTSCREEN.EXE", "UNSIGNED", "C:\\Windows\\System32\\smartscreen.exe", FALSE, 0},
        {"G++.EXE-79CA7C18.pf", "G++.EXE", "UNSIGNED", "C:\\MinGW\\bin\\g++.exe", FALSE, 0},
        {"CHROME.EXE-1853BBCF.pf", "CHROME.EXE", "SIGNED", "C:\\Program Files\\Google\\Chrome\\chrome.exe", TRUE, 12345},
        {"EXPLORER.EXE-CBAB91C0.pf", "EXPLORER.EXE", "SIGNED", "C:\\Windows\\explorer.exe", TRUE, 2345},
        {"NOTEPAD.EXE-87382445.df", "NOTEPAD.EXE", "SIGNED", "C:\\Windows\\System32\\notepad.exe", FALSE, 0},
        {"NODE.EXE-2D8A4E0.df", "NODE.EXE", "SIGNED", "C:\\Program Files\\Node.js\\node.exe", FALSE, 0},
        {"PYTHON.EXE-3A8B1C2D.pf", "PYTHON.EXE", "SIGNED", "C:\\Python\\python.exe", FALSE, 0},
        {"WINWORD.EXE-4B9C3D2E.pf", "WINWORD.EXE", "SIGNED", "C:\\Program Files\\Microsoft Office\\WINWORD.EXE", FALSE, 0},
        {"TELEGRAM.EXE-5C0D4E3F.pf", "TELEGRAM.EXE", "SIGNED", "C:\\Users\\User\\AppData\\Telegram\\telegram.exe", TRUE, 5678},
        {"STEAM.EXE-6D1E5F4A.pf", "STEAM.EXE", "SIGNED", "C:\\Program Files\\Steam\\steam.exe", FALSE, 0},
        {"VBOX.EXE-7E2F6A5B.pf", "VIRTUALBOX.EXE", "SIGNED", "C:\\Program Files\\Oracle\\VirtualBox\\virtualbox.exe", FALSE, 0}
    };
    
    const char* sampleModules[] = {
        "kernel32.dll", "user32.dll", "ntdll.dll", "advapi32.dll", "gdi32.dll",
        "shell32.dll", "msvcrt.dll", "ole32.dll", "wininet.dll", "ws2_32.dll"
    };
    
    for (int i = 0; i < prefetchCount; i++) {
        strcpy_s(prefetchFiles[i].filename, 256, sampleData[i].filename);
        strcpy_s(prefetchFiles[i].executable, 256, sampleData[i].executable);
        strcpy_s(prefetchFiles[i].signature, 32, sampleData[i].signature);
        strcpy_s(prefetchFiles[i].path, 512, sampleData[i].path);
        prefetchFiles[i].isRunning = sampleData[i].isRunning;
        prefetchFiles[i].processId = sampleData[i].pid;
        
        prefetchFiles[i].fileSize = 10240 + rand() % 204800;
        prefetchFiles[i].runCount = 1 + rand() % 200;
        
        time_t t = time(NULL) - (rand() % 2592000);
        struct tm tm_info;
        localtime_s(&tm_info, &t);
        strftime(prefetchFiles[i].lastRun, 32, "%Y-%m-%d %H:%M:%S", &tm_info);
        
        prefetchFiles[i].moduleCount = 3 + rand() % 3;
        for (int j = 0; j < prefetchFiles[i].moduleCount; j++) {
            strcpy_s(prefetchFiles[i].loadedModules[j], 256, sampleModules[rand() % 10]);
        }
    }
    
    processCount = 8;
    processes = malloc(processCount * sizeof(ProcessInfo));
    
    struct {
        const char* name;
        const char* type;
        BOOL isTarget;
    } procData[] = {
        {"RUNDLL32.EXE", "RUNDLL32", TRUE},
        {"CONHOST.EXE", "CONHOST", TRUE},
        {"JAVAW.EXE", "JAVAW", TRUE},
        {"EXPLORER.EXE", "EXPLORER", FALSE},
        {"CHROME.EXE", "CHROME", FALSE},
        {"PREFETCHER.EXE", "PREFETCHER", FALSE},
        {"TELEGRAM.EXE", "TELEGRAM", FALSE},
        {"SYSTEM", "SYSTEM", FALSE}
    };
    
    for (int i = 0; i < processCount; i++) {
        strcpy_s(processes[i].name, 256, procData[i].name);
        strcpy_s(processes[i].type, 32, procData[i].type);
        processes[i].isTarget = procData[i].isTarget;
        processes[i].pid = 1000 + i * 100;
        processes[i].memory = 51200 + rand() % 512000;
        processes[i].threads = 1 + rand() % 8;
        processes[i].handles = 20 + rand() % 100;
        
        sprintf_s(processes[i].exePath, MAX_PATH, "C:\\Windows\\System32\\%s", procData[i].name);
        sprintf_s(processes[i].commandLine, 512, "\"%s\" --type=renderer", processes[i].exePath);
        
        processes[i].dllCount = 5;
        for (int j = 0; j < 5; j++) {
            strcpy_s(processes[i].loadedDLLs[j], 256, sampleModules[rand() % 10]);
        }
    }
}

/* ==================== WINDOW PROCEDURE ==================== */
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            hFontConsolas = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                      FIXED_PITCH | FF_DONTCARE, "Consolas");
            
            hFontHack = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                  CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                  FIXED_PITCH | FF_DONTCARE, "Courier New");
            
            hFontSegoe = CreateFont(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                   CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                   VARIABLE_PITCH, "Segoe UI");
            
            hBrushDark = CreateSolidBrush(RGB(20, 25, 30));
            hBrushBlack = CreateSolidBrush(RGB(0, 0, 0));
            hPenGreen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
            hPenCyan = CreatePen(PS_SOLID, 1, RGB(0, 200, 255));
            
            LoadSampleData();
            
            SetTimer(hWnd, 1, 16, NULL);
            SetTimer(hWnd, 2, 100, NULL);
            SetTimer(hWnd, 3, 50, NULL);
            break;
        }
        
        case WM_TIMER:
            if (wParam == 1) {
                InvalidateRect(hWnd, NULL, FALSE);
            } else if (wParam == 2) {
                if (!showStartup) {
                    for (int i = 0; i < 3; i++) {
                        CreateParticle(rand() % WINDOW_WIDTH, rand() % WINDOW_HEIGHT, 
                                      RGB(0, 150 + rand() % 100, 0));
                    }
                }
            } else if (wParam == 3 && showStartup) {
                animationProgress += 0.02f;
                if (animationProgress >= 1.0f) {
                    animationProgress = 1.0f;
                    showStartup = FALSE;
                    KillTimer(hWnd, 3);
                    
                    for (int i = 0; i < 50; i++) {
                        CreateParticle(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 
                                      RGB(0, 200 + rand() % 55, 0));
                    }
                }
            }
            break;
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
            SelectObject(hdcMem, hBitmap);
            
            if (showStartup) {
                DrawStartupScreen(hdcMem);
            } else {
                DrawMainInterface(hdcMem);
            }
            
            BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hdcMem, 0, 0, SRCCOPY);
            
            DeleteObject(hBitmap);
            DeleteDC(hdcMem);
            EndPaint(hWnd, &ps);
            break;
        }
        
        case WM_KEYDOWN:
            if (!showStartup) {
                switch (wParam) {
                    case VK_F1: currentView = 0; break;
                    case VK_F2: currentView = 1; break;
                    case VK_F3: currentView = 2; break;
                    case VK_F4: currentView = 3; break;
                    case VK_F5: currentView = 4; break;
                    case VK_F6: currentView = 5; break;
                    case VK_F7: currentView = 6; break;
                    case VK_F8: currentView = 7; break;
                    case 'R': LoadSampleData(); break;
                    case VK_ESCAPE: PostQuitMessage(0); break;
                }
                InvalidateRect(hWnd, NULL, FALSE);
            }
            break;
        
        case WM_LBUTTONDOWN:
            if (!showStartup) {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                
                if (x >= 10 && x <= 250 && y >= 90 && y <= 290) {
                    int clickedFilter = (y - 90) / 25;
                    if (clickedFilter >= 0 && clickedFilter < 8) {
                        currentView = clickedFilter;
                        InvalidateRect(hWnd, NULL, FALSE);
                    }
                }
            }
            break;
        
        case WM_DESTROY:
            if (prefetchFiles) free(prefetchFiles);
            if (processes) free(processes);
            if (particles) free(particles);
            
            DeleteObject(hFontConsolas);
            DeleteObject(hFontHack);
            DeleteObject(hFontSegoe);
            DeleteObject(hBrushDark);
            DeleteObject(hBrushBlack);
            DeleteObject(hPenGreen);
            DeleteObject(hPenCyan);
            
            KillTimer(hWnd, 1);
            KillTimer(hWnd, 2);
            KillTimer(hWnd, 3);
            
            PostQuitMessage(0);
            break;
        
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

/* ==================== MAIN ENTRY ==================== */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);
    
    srand((unsigned int)time(NULL));
    
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "PrefetcherUltraClass";
    
    if (!RegisterClassEx(&wc)) {
        MessageBoxA(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowX = (screenWidth - WINDOW_WIDTH) / 2;
    int windowY = (screenHeight - WINDOW_HEIGHT) / 2;
    
    hMainWnd = CreateWindowEx(0, "PrefetcherUltraClass", "Prefetcher Ultra v3.0",
                              WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
                              windowX, windowY, WINDOW_WIDTH, WINDOW_HEIGHT,
                              NULL, NULL, hInstance, NULL);
    
    if (!hMainWnd) {
        MessageBoxA(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
