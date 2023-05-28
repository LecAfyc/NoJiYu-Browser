#include "framework.h"
#include "NoJiYu Browser.h"
#include <windows.h>
#include <strsafe.h>
#include <io.h>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <wrl.h>
#include <wil/com.h>
#include <AccCtrl.h>
#include <AclAPI.h>
#include "winuser.h"
#include "shobjidl_core.h"

#include "webView2.h"
using namespace Microsoft::WRL;


// The main window class name.
static TCHAR szWindowClass[] = _T("Microsoft Edge");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("Microsoft Edge");

HINSTANCE hInst;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Pointer to WebViewController
static wil::com_ptr<ICoreWebView2Controller> webviewController;

// Pointer to WebView window
static wil::com_ptr<ICoreWebView2> webviewWindow;

BOOL ShowInTaskbar(HWND hWnd, BOOL bShow)
{
    LPVOID lp = NULL;
    CoInitialize(lp);


    HRESULT hr;
    ITaskbarList* pTaskbarList;
    hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
        IID_ITaskbarList, (void**)&pTaskbarList);
    if (SUCCEEDED(hr))
    {
        pTaskbarList->HrInit();
        if (bShow)
            pTaskbarList->AddTab(hWnd);
        else
            pTaskbarList->DeleteTab(hWnd);
        pTaskbarList->Release();
        return TRUE;
    }

    return FALSE;
}

int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Call to RegisterClassEx failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }
    // Store instance handle in our global variable
    hInst = hInstance;

    // The parameters to CreateWindow explained:
    // szWindowClass: the name of the application
    // szTitle: the text that appears in the title bar
    // WS_OVERLAPPEDWINDOW: the type of window to create
    // CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
    // 500, 100: initial size (width, length)
    // NULL: the parent of this window
    // NULL: this application does not have a menu bar
    // hInstance: the first parameter from WinMain
    // NULL: not used in this application
    HWND hWnd = CreateWindow(
        szWindowClass,
        szTitle,
        WS_EX_APPWINDOW|WS_EX_TOPMOST,
        CW_USEDEFAULT, CW_USEDEFAULT,
        860, 610,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd)
    {
        MessageBox(NULL,
            _T("Call to CreateWindow failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }
    
    // The parameters to ShowWindow explained:
    // hWnd: the value returned from CreateWindow
    // nCmdShow: the fourth parameter from WinMain
    ShowWindow(hWnd,
        nCmdShow);
    UpdateWindow(hWnd);
    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOREPOSITION);
    ShowInTaskbar(hWnd, false);
    HRESULT res = CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hWnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                //MessageBoxA(hWnd, "createView", "", NULL);
                // Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
                env->CreateCoreWebView2Controller(hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [hWnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                        if (controller != nullptr) {
                            webviewController = controller;
                            webviewController->get_CoreWebView2(&webviewWindow);
                        }

                        // Add a few settings for the webview
                        // The demo step is redundant since the values are the default settings
                        ICoreWebView2Settings* Settings;
                        webviewWindow->get_Settings(&Settings);
                        Settings->put_IsScriptEnabled(TRUE);
                        Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                        Settings->put_IsWebMessageEnabled(TRUE);
                        Settings->put_AreDevToolsEnabled(FALSE);

                        // Resize WebView to fit the bounds of the parent window
                        RECT bounds;
                        GetClientRect(hWnd, &bounds);
                        webviewController->put_Bounds(bounds);

                        // Schedule an async task to navigate to Bing

                        HRESULT res = webviewWindow->Navigate(L"https://cn.bing.com");
                        std::string sres = std::to_string(res).c_str();
                        // Step 4 - Navigation events
                        
                        EventRegistrationToken token;
                        webviewWindow->add_NewWindowRequested(Callback<ICoreWebView2NewWindowRequestedEventHandler>(
                            [](ICoreWebView2* webview, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
                                args->put_Handled(TRUE);
                                PWSTR uri;
                                args->get_Uri(&uri);
                                webviewWindow->Navigate(uri);
                                return S_OK;
                            }).Get(), &token);

                        EventRegistrationToken token2;
                        webviewWindow->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
                            [](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                                PWSTR uri;
                                args->get_Uri(&uri);
                                std::wstring source(uri);
                                if (source.substr(0, 34) == L"https://cn.bing.com/search?q=-game") {
                                    webviewWindow->Navigate(L"https://www.msn.cn/zh-cn/play/");
                                }else if (source.substr(0, 34) == L"https://cn.bing.com/search?q=-bili") {
                                    webviewWindow->Navigate(L"https://www.bilibili.com");
                                }else if (source.substr(0, 33) == L"https://cn.bing.com/search?q=-phi") {
                                    webviewWindow->Navigate(L"https://lchzh3473.github.io/sim-phi/");
                                }
                                return S_OK;
                            }).Get(), &token);

                        return S_OK;
                    }).Get());
                return S_OK;
            }).Get());

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_DESTROY  - post a quit message and return
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        if (webviewController != nullptr) {
            RECT bounds;
            GetClientRect(hWnd, &bounds);
            webviewController->put_Bounds(bounds);
        };
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        SetWindowDisplayAffinity(hWnd, 0x00000011); //Hide window
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }
    return 0;
}
