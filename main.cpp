#include <windows.h>
#include <shellapi.h>
#include <string>
#include <fstream>
#include <iostream>
#include <cstring>
#include <strsafe.h>
#include <filesystem> // Added

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_ICON 1001
#define ID_SHOW_CONSOLE 2001
#define ID_EXIT 2003
#define HOTKEY_ID_COPY_TRANSCRIPT 1 // ID for the hotkey

HWND g_hWnd;
NOTIFYICONDATAW nid; // Changed to NOTIFYICONDATAW
bool consoleVisible = false;
WNDPROC originalConsoleProc = nullptr;

// Python script embedded
// The Python script can continue to work with ANSI characters, so std::string can remain.
// However, temporary file path and command creation operations should be set to use WCHAR.
const std::string PYTHON_SCRIPT = R"(
import sys,webbrowser,pyperclip,pyautogui,threading,re,time,requests
from youtube_transcript_api import YouTubeTranscriptApi

def setup_session():
    session = requests.Session()
    session.headers.update({
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36',
        'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8',
        'Accept-Language': 'en-US,en;q=0.5',
        'Accept-Encoding': 'gzip, deflate',
        'Connection': 'keep-alive',
        'Upgrade-Insecure-Requests': '1',
    })
    return session

def extract_video_id(url):
    if 'youtu.be/' in url: return url.split('youtu.be/')[1].split('?')[0]
    elif 'youtube.com/watch' in url: return url.split('v=')[1].split('&')[0]
    return None

def get_transcript_robust(video_id):
    methods = [
        lambda: get_with_custom_session(video_id),
        lambda: get_with_fallback(video_id),
        lambda: get_with_basic_api(video_id)
    ]
    
    for i, method in enumerate(methods):
        try:
            result = method()
            if result[0] is not None:
                return result
            time.sleep(1)
        except Exception as e:
            time.sleep(2)
    
    return None, None

def get_with_custom_session(video_id):
    session = setup_session()
    try:
        transcript_list = YouTubeTranscriptApi.list_transcripts(video_id, proxies=None, cookies=session.cookies)
        priority_languages = ['tr', 'en']
        
        for lang_code in priority_languages:
            for transcript_entry in transcript_list:
                if transcript_entry.language_code == lang_code:
                    try:
                        data = transcript_entry.fetch()
                        return data, lang_code
                    except:
                        continue

        for transcript_entry in transcript_list:
            try:
                data = transcript_entry.fetch()
                return data, transcript_entry.language_code 
            except:
                continue
    except:
        pass
    return None, None

def get_with_fallback(video_id):
    try:
        priority_languages = ['tr', 'en']
        for lang_code in priority_languages:
            try:
                transcript = YouTubeTranscriptApi.get_transcript(video_id, languages=[lang_code])
                return transcript, lang_code
            except:
                continue
        transcript = YouTubeTranscriptApi.get_transcript(video_id)
        return transcript, 'auto'
    except:
        pass
    return None, None

def get_with_basic_api(video_id):
    try:
        transcript = YouTubeTranscriptApi.get_transcript(video_id)
        return transcript, 'basic'
    except:
        pass
    return None, None

def format_text(text):
    text = re.sub(r'[\n\t]+', ' ', text) 
    text = re.sub(r'\s{2,}', ' ', text).strip()
    text = re.sub(r'\s+([.!?,:;])', r'\1', text)
    text = re.sub(r'([.!?,:;])(?=[a-zA-Z0-9])', r'\1 ', text)
    text = re.sub(r'([.!?]){2,}', r'\1', text)
    text = re.sub(r'([,:;]){2,}', r'\1', text)
    if text and text[0].islower():
        text = text[0].upper() + text[1:]
    def capitalize_sentence(match):
        return match.group(1) + match.group(2).upper()
    text = re.sub(r'([.!?]\s*)([a-z])', capitalize_sentence, text)
    text = re.sub(r'\s{2,}', ' ', text).strip()
    return text

def open_gemini_and_paste():
    webbrowser.open('https://gemini.google.com/')
    time.sleep(3)
    
    attempts = 0
    max_attempts = 10
    
    while attempts < max_attempts:
        try:
            if attempts == 0:
                pyautogui.click(pyautogui.size()[0] // 2, pyautogui.size()[1] // 2)
                time.sleep(0.5)
            
            if attempts < 3:
                for _ in range(attempts + 1):
                    pyautogui.press('tab')
                    time.sleep(0.1)
            elif attempts < 6:
                pyautogui.click(pyautogui.size()[0] // 2, int(pyautogui.size()[1] * 0.8))
                time.sleep(0.5)
            else:
                pyautogui.hotkey('ctrl', 'a')
                time.sleep(0.2)
            
            pyautogui.hotkey('ctrl', 'v')
            time.sleep(0.5)
            return True
            
        except:
            pass
            
        attempts += 1
        time.sleep(1)
    
    return False

if __name__ == '__main__':
    video_id = extract_video_id(sys.argv[1]) if len(sys.argv) > 1 else None
    if not video_id:
        print('Error: Invalid YouTube URL provided.')
        sys.exit(1)
    
    print('Status: Getting transcript with anti-blocking methods...')
    
    transcript, lang = get_transcript_robust(video_id) 
    if transcript: 
        # Yeni API sürümü için güncellenen veri işleme
        if hasattr(transcript[0], 'text'):
            # Yeni format: FetchedTranscriptSnippet objects
            text = ' '.join(item.text for item in transcript)
        else:
            # Eski format: dictionary  
            text = ' '.join(item['text'] for item in transcript if 'text' in item)
            
        if text.strip():
            formatted = format_text(text) + " lütfen bu transkriptin kapsamlı ve ayrıntılı bir özetini Türkçe olarak sağlayın"
            print(f'Status: Transcript ready (language: {lang})')
            pyperclip.copy(formatted)
            
            print('Status: Opening Gemini and pasting...')
            success = open_gemini_and_paste()
            
            if success:
                print('Status: Successfully pasted to Gemini!')
            else:
                print('Status: Copied to clipboard. Paste manually to Gemini.')
        else:
            print('Status: Empty transcript received.')
    else:
        print('Error: No transcript found for the video.')
)";

std::wstring createTempScript() { // Changed to wstring
    WCHAR tempPath[MAX_PATH]; // Changed to WCHAR
    GetTempPathW(MAX_PATH, tempPath); // Changed to GetTempPathW
    std::wstring scriptPath = std::wstring(tempPath) + L"yt_transcript.py"; // Used wstring and L""
    
    std::ofstream file(std::filesystem::path(scriptPath), std::ios::out | std::ios::binary); // Used std::filesystem::path
    if (!file.is_open()) {
        // In case of error, return an empty wstring or throw an exception
        // In this example, show a message box to the user and return an empty string.
        MessageBoxW(nullptr, L"Could not create temporary script file.", L"Error", MB_ICONERROR);
        return L""; 
    }
    file << PYTHON_SCRIPT;
    file.close();
    
    return scriptPath;
}

// Console window procedure to intercept minimize
LRESULT CALLBACK ConsoleWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_SYSCOMMAND && wParam == SC_MINIMIZE) {
        ShowWindow(hwnd, SW_HIDE);
        consoleVisible = false;
        return 0;
    }
    if (uMsg == WM_SHOWWINDOW && !wParam) { // Window is being hidden
        consoleVisible = false;
        // Fall through to original proc
    }
    if (uMsg == WM_WINDOWPOSCHANGING) {
        WINDOWPOS* pos = (WINDOWPOS*)lParam;
        if (pos->flags & SWP_HIDEWINDOW) {
            consoleVisible = false;
        }
    }
    if (originalConsoleProc) {
        return CallWindowProc(originalConsoleProc, hwnd, uMsg, wParam, lParam);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam); // Fallback to default if originalConsoleProc is null
}

void createConsole() {
    if (!AllocConsole()) {
        MessageBoxW(nullptr, L"Could not create console.", L"Error", MB_ICONERROR);
        return;
    }
    
    FILE* pCout;
    FILE* pCerr;
    FILE* pCin;
    
    // Redirect standard I/O to the new console
    freopen_s(&pCout, "CONOUT$", "w", stdout);
    freopen_s(&pCerr, "CONOUT$", "w", stderr);
    freopen_s(&pCin, "CONIN$", "r", stdin);
    
    HWND consoleWnd = GetConsoleWindow();
    if (consoleWnd) { // Check if console window handle is valid
        // Subclass console window to intercept minimize
        originalConsoleProc = (WNDPROC)SetWindowLongPtr(consoleWnd, GWLP_WNDPROC, (LONG_PTR)ConsoleWindowProc);
        
        ShowWindow(consoleWnd, SW_HIDE); // Initially hide the console
        consoleVisible = false;
    }
    
    std::cout << "YouTube Transcript Copier\n";
    std::cout << "Use Ctrl+Y to copy YouTube link transcript.\n";
    std::cout << "Right-click tray icon for options.\n";
}

void showTrayMenu() {
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_SHOW_CONSOLE, L"Show Console"); // Used AppendMenuW and L""
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr); // Used AppendMenuW
    AppendMenuW(hMenu, MF_STRING, ID_EXIT, L"Exit"); // Used AppendMenuW and L""
    
    SetForegroundWindow(g_hWnd); // Necessary for the menu to work correctly
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, cursorPos.x, cursorPos.y, 0, g_hWnd, nullptr);
    DestroyMenu(hMenu);
}

std::string getSelectedText() {
    // Ensure clipboard is opened and emptied before simulating Ctrl+C
    if (!OpenClipboard(nullptr)) {
        // Optionally, log or display an error that clipboard could not be opened
        return "";
    }
    EmptyClipboard(); // Clear previous clipboard content
    CloseClipboard(); // Close clipboard before sending keys

    // Simulate Ctrl+C
    keybd_event(VK_CONTROL, 0, 0, 0); // Press Ctrl
    keybd_event('C', 0, 0, 0);        // Press C
    keybd_event('C', 0, KEYEVENTF_KEYUP, 0); // Release C
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0); // Release Ctrl
    
    Sleep(100); // Increased sleep duration for clipboard operation to complete
    
    std::string result;
    if (OpenClipboard(nullptr)) {
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (hData != nullptr) {
            char* pszText = static_cast<char*>(GlobalLock(hData));
            if (pszText != nullptr) {
                result = pszText;
                GlobalUnlock(hData);
            } else {
                // GlobalLock failed, can log or display an error.
                // Silently continuing in this example, result will be empty.
            }
        }
        CloseClipboard();
    } else {
        // Optionally, log or display an error that clipboard could not be opened again
    }
    return result;
}

inline bool isYouTubeLink(const std::string& text) {
    // More robust check for YouTube links
    return (text.find("youtube.com/watch?v=") != std::string::npos ||
            text.find("youtu.be/") != std::string::npos ||
            text.find("youtube.com/shorts/") != std::string::npos);
}

void AddTrayIcon() {
    memset(&nid, 0, sizeof(NOTIFYICONDATAW)); // Use NOTIFYICONDATAW
    nid.cbSize = sizeof(NOTIFYICONDATAW);    // Use size of NOTIFYICONDATAW
    nid.hWnd = g_hWnd;
    nid.uID = ID_TRAY_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    
    // Attempt to load a standard icon first, then application icon as fallback
    nid.hIcon = LoadIcon(nullptr, IDI_INFORMATION); // A common system icon
    if (nid.hIcon == nullptr) {
        nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION); // Fallback to application icon
    }
    if (nid.hIcon == nullptr) {
        // If icon still couldn't be loaded, display a warning.
        // The program will continue, but the tray icon might be missing or default.
        MessageBoxW(nullptr, L"Could not load tray icon.", L"Warning", MB_ICONWARNING);
    }
    StringCchCopyW(nid.szTip, ARRAYSIZE(nid.szTip), L"YouTube Transcript Copier (Ctrl+Y)"); // Tooltip in English
    
    Shell_NotifyIconW(NIM_ADD, &nid); // Changed to Shell_NotifyIconW
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
             // Optionally, create console here if needed at startup, or defer
            break;
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED && GetConsoleWindow() && IsWindowVisible(GetConsoleWindow())) {
                // If main window is minimized and console is visible, hide console too.
                // This behavior might be desirable, or not, depending on user preference.
                // For now, let's not hide console automatically on main window minimize.
            } else if (wParam == SIZE_MINIMIZED) {
                 ShowWindow(hwnd, SW_HIDE); // Hide main window when minimized (to tray)
                 return 0;
            }
            break;
        case WM_HOTKEY:
            if (wParam == HOTKEY_ID_COPY_TRANSCRIPT) {
                std::string selectedText = getSelectedText(); 
                if (!selectedText.empty() && isYouTubeLink(selectedText)) {
                    std::wstring scriptPath = createTempScript(); 
                    if (scriptPath.empty()) {
                        // MessageBoxW(g_hWnd, L"Could not run Python script (temp file issue).", L"Error", MB_ICONERROR);
                        break; 
                    }
                    
                    std::wstring selectedTextW(selectedText.begin(), selectedText.end());
                    std::wstring command = L"pythonw.exe \"" + scriptPath + L"\" \"" + selectedTextW + L"\""; // Use pythonw.exe to run without console
                    
                    STARTUPINFOW si = {0}; 
                    PROCESS_INFORMATION pi = {0};
                    si.cb = sizeof(si);
                    si.dwFlags = STARTF_USESHOWWINDOW;
                    si.wShowWindow = SW_HIDE; // Hide the Python script's own console if it appears
                    
                    if (CreateProcessW(nullptr, const_cast<wchar_t*>(command.c_str()), nullptr, nullptr, FALSE, 
                                     CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) { // CREATE_NO_WINDOW flag
                        // Python process'ini arka planda çalıştır
                        CloseHandle(pi.hProcess);
                        CloseHandle(pi.hThread);
                        // Geçici dosyayı temizleme işlemini ertele
                        Sleep(1000); // 1 saniye bekle
                        DeleteFileW(scriptPath.c_str());
                    } else {
                        // CreateProcessW failed. Inform the user.
                        // Try with 'python.exe' as a fallback if 'pythonw.exe' fails or is not found
                        command = L"python.exe \"" + scriptPath + L"\" \"" + selectedTextW + L"\"";
                        if (CreateProcessW(nullptr, const_cast<wchar_t*>(command.c_str()), nullptr, nullptr, FALSE,
                                         CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
                            CloseHandle(pi.hProcess);
                            CloseHandle(pi.hThread);
                            Sleep(1000);
                            DeleteFileW(scriptPath.c_str());
                        } else {
                             MessageBoxW(g_hWnd, L"Could not run Python script.\nEnsure Python is in your system PATH.", L"Execution Error", MB_ICONERROR);
                        }
                    }
                } else if (selectedText.empty()) {
                    // MessageBoxW(g_hWnd, L"No text selected or clipboard is empty.", L"Info", MB_ICONINFORMATION);
                } else if (!isYouTubeLink(selectedText)) {
                    // MessageBoxW(g_hWnd, L"The selected text is not a valid YouTube link.", L"Info", MB_ICONINFORMATION);
                }
            }
            break;
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) { // Show menu on left or right click
                showTrayMenu();
            }
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_SHOW_CONSOLE:
                    {
                        HWND consoleWnd = GetConsoleWindow();
                        if (consoleWnd == nullptr) {
                            createConsole(); // Create console if it doesn't exist
                            consoleWnd = GetConsoleWindow(); // Get handle again
                        }
                        if (consoleWnd) {
                             // Toggle visibility
                            if (IsWindowVisible(consoleWnd)) {
                                ShowWindow(consoleWnd, SW_HIDE);
                                consoleVisible = false;
                            } else {
                                ShowWindow(consoleWnd, SW_SHOW);
                                SetForegroundWindow(consoleWnd); // Bring to front
                                consoleVisible = true;
                            }
                        }
                    }
                    break;
                case ID_EXIT:
                    PostQuitMessage(0);
                    break;
            }
            break;
        case WM_DESTROY:
            // Clean up temporary script file if it exists
            {
                WCHAR tempPathChars[MAX_PATH];
                GetTempPathW(MAX_PATH, tempPathChars);
                std::wstring scriptPathOnExit = std::wstring(tempPathChars) + L"yt_transcript.py";
                DeleteFileW(scriptPathOnExit.c_str());
            }
            Shell_NotifyIconW(NIM_DELETE, &nid); // Changed to Shell_NotifyIconW
            if (originalConsoleProc && GetConsoleWindow()) { // Restore original console proc if console exists
                 SetWindowLongPtr(GetConsoleWindow(), GWLP_WNDPROC, (LONG_PTR)originalConsoleProc);
            }
            if (GetConsoleWindow()) { // Free console if it was allocated
                FreeConsole();
            }
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0; // Return 0 for messages we've handled
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Ensure only one instance of the application runs
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"Global\\YTCopierMutex"); // Used CreateMutexW, L"", and Global namespace for system-wide mutex
    if (hMutex == nullptr || GetLastError() == ERROR_ALREADY_EXISTS) {
        if(hMutex) CloseHandle(hMutex);
        // Optionally, bring the existing instance to the foreground instead of just exiting.
        // For simplicity, we just exit here.
        // HWND existingWnd = FindWindowW(L"YTCopier", nullptr);
        // if (existingWnd) {
        //     ShowWindow(existingWnd, SW_RESTORE);
        //     SetForegroundWindow(existingWnd);
        // }
        return 0;  // Already running, exit
    }
    
    // Create a message-only window for handling tray icon messages and hotkeys
    WNDCLASSEXW wc = {0}; // Changed to WNDCLASSEXW
    wc.cbSize = sizeof(WNDCLASSEXW); 
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"YTCopierApp"; // Unique class name, L"" used
    // wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Not needed for message-only window
    // wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION); // Not needed for message-only window

    if (!RegisterClassExW(&wc)) { // Changed to RegisterClassExW
        MessageBoxW(nullptr, L"Could not register window class!", L"Error", MB_ICONERROR);
        CloseHandle(hMutex); 
        return 1;
    }
    
    // Create a message-only window (does not need to be visible)
    g_hWnd = CreateWindowExW(
        0, L"YTCopierApp", L"YouTube Transcript Copier Hidden Window", 0, // No visual style
        0, 0, 0, 0, // Position and size are irrelevant for message-only
        HWND_MESSAGE, // Special value for message-only windows
        nullptr, hInstance, nullptr
    );

    if (!g_hWnd) {
        MessageBoxW(nullptr, L"Could not create window!", L"Error", MB_ICONERROR);
        CloseHandle(hMutex); 
        UnregisterClassW(L"YTCopierApp", hInstance); 
        return 1;
    }
    
    if (!RegisterHotKey(g_hWnd, HOTKEY_ID_COPY_TRANSCRIPT, MOD_CONTROL, 'Y')) {
        MessageBoxW(g_hWnd, L"Could not register hotkey (Ctrl+Y). It might be in use by another application.", L"Warning", MB_ICONWARNING);
        // Program can continue, but hotkey won't work.
    }
    
    AddTrayIcon();
    
    // Main message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) { // Check GetMessage return value
        TranslateMessage(&msg); // For keyboard input
        DispatchMessage(&msg);
    }
    
    // Cleanup
    UnregisterHotKey(g_hWnd, HOTKEY_ID_COPY_TRANSCRIPT);
    Shell_NotifyIconW(NIM_DELETE, &nid); // Ensure icon is removed
    CloseHandle(hMutex); // Release the mutex
    UnregisterClassW(L"YTCopierApp", hInstance);
    
    return (int)msg.wParam; // Return exit code from PostQuitMessage
}
