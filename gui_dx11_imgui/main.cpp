#include <iostream>
#include <d3d11.h>
#include <tchar.h>
#include "external/imgui/imconfig.h"
#include "external/imgui/imgui_impl_win32.h"
#include "external/imgui/imgui_impl_dx11.h"

#include <ui_module/ui_base.h>



// Data
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);



#include <vector>
#include <list>
#include <functional>
#include <algorithm>
#include <type_traits>
#include <unordered_map>
#include <queue>
#include <mutex>

struct View {
    virtual void draw() = 0;
    virtual void setParent(View* parent) = 0;
};

typedef std::function<void(const View& const)> OnClickListener;

template <typename T>
class UseListener {
    using NT = typename std::remove_pointer<T>::type;
    using PT = typename std::add_pointer<T>::type;
    using RT = typename std::add_lvalue_reference<T>::type;

    // ÉâÉÄÉ_éÆÇ≈ìnÇ≥ÇÍÇΩèÍçáÇÃäiî[êÊ
    NT _listenerImpl;
    // ÉäÉXÉiÅ[Ç÷ÇÃéQè∆
    RT _listener;

public:
    UseListener() : _listener(_listenerImpl)
    {}
    void setListener(NT listener) {
        _listenerImpl = std::move(listener);
        _listener = _listenerImpl;
    }
    void setListener(PT listener) {
        _listener = *listener;
    }
    const RT const get() {
        return _listener;
    }
};


class Button : public View {
    UseListener<OnClickListener> _onClickListener;
    std::string _name;
public:
    Button(std::string name) : _name(name)
    {}
    void setOnClickListenr(OnClickListener listener) {
        _onClickListener.setListener(listener);
    }
    void setOnClickListenr(OnClickListener* listener) {
        _onClickListener.setListener(listener);
    }
    void draw() override {
        if (ImGui::Button(_name.c_str())) {
            if (_onClickListener.get()) {
                _onClickListener.get()(*this);
            }
        }
    }
    virtual void setParent(View* parent) override {
        ;
    }
};

class ViewGroup : public View {
    std::list<std::unique_ptr<View>> _childs;
    View* _parent;
public:
    std::string _name;
    ViewGroup(std::string name) : _name(name), _parent(NULL)
    {}
    virtual ~ViewGroup() {
        _childs.clear();
        _parent = NULL;
    }
    virtual void addChild(View* child) {
        child->setParent(this);
        _childs.push_back(std::unique_ptr<View>(child));
    }
    virtual void setParent(View* parent) override {
        _parent = parent;
    }
    virtual void draw() override {
        /*if (!_parent) {
            ImGui::Begin(_name.c_str());
        }*/
        drawImpl();
       /* if (!_parent) {
            ImGui::End();
        }  */
    }
protected:
    std::list<std::unique_ptr<View>>& getChild() {
        return _childs;
    }
    virtual void drawImpl() {
        for (const auto& child : _childs) {
            child.get()->draw();
        }
    }
};

class LinerVerticalLayout : public ViewGroup {
public:
    LinerVerticalLayout(std::string name) : ViewGroup(name)
    {}
};

class LinerHorizonLayout : public ViewGroup {
public:
    LinerHorizonLayout(std::string name) : ViewGroup(name)
    {}
protected:
    virtual void drawImpl() {
        auto last = getChild().end();
        last--;
        for (auto child = getChild().begin(); child != getChild().end(); ++child) {
            child->get()->draw();
            if (last != child) {
                ImGui::SameLine();
            }
        }
    }
};

class Widget;
struct RouterBase {
public:
    virtual void add(std::string name, Widget* w) = 0;
    virtual void addDisplay(std::string name) = 0;
    virtual void removeDisplay(std::string name) = 0;
};

class UseRouter {
    RouterBase* _router;
public:
    void setRouter(RouterBase* router) {
        _router = router;
    }
    RouterBase* getRouter() {
        return _router;
    }
};

class Widget : public UseRouter {
    ViewGroup* _rootView;
public:
    void onCreate() {
        _rootView = onCreateView();
    }
    virtual ViewGroup* onCreateView() = 0;
    virtual void onDraw() {
        _rootView->draw();
    }
};

class RootWidget : public Widget {
public:
    virtual ViewGroup* onCreateView() override
    {
        ViewGroup* pageG = new LinerVerticalLayout("pageG");
        ViewGroup* page1 = new LinerHorizonLayout("page1");
        {
            Button* a1 = new Button("aa1");
            a1->setOnClickListenr([this](const View& const v)
                {
                    std::cout << (&v) << "(aa1)" << std::endl;
                    RouterBase* r = this->getRouter();
                    r->addDisplay("P1");
                }
            );
            page1->addChild(a1);
            Button* a2 = new Button("aa2");
            a2->setOnClickListenr([this](const View& const v)
                {
                    std::cout << (&v) << "(aa2)" << std::endl;
                    RouterBase* r = this->getRouter();
                    r->addDisplay("P2");
                }
            );
            page1->addChild(a2);
            Button* a3 = new Button("aa3");
            page1->addChild(a3);
        }
        pageG->addChild(page1);

        ViewGroup* page2 = new LinerHorizonLayout("page2");
        {
            Button* a12 = new Button("aaa1");
            Button* a22 = new Button("aaa2");
            Button* a32 = new Button("aaa3");
            page2->addChild(a12);
            page2->addChild(a22);
            page2->addChild(a32);
        }
        pageG->addChild(page2);
        return pageG;
    }
    void onDraw() override {
        ImGui::Begin("aaa");
        Widget::onDraw();
        ImGui::End();
    }
};

class RootWidget2 : public Widget {
public:
    virtual ViewGroup* onCreateView() override
    {
        ViewGroup* pageG = new LinerVerticalLayout("pageG");
        ViewGroup* page2 = new LinerHorizonLayout("page2");
        {
            Button* a12 = new Button("aaa1");
            a12->setOnClickListenr([this](const View& const v)
                {
                    std::cout << (&v) << "(aa1)" << std::endl;
                    RouterBase* r = this->getRouter();
                    r->removeDisplay("P1");
                }
            );
            Button* a22 = new Button("aaa2");
            Button* a32 = new Button("aaa3");
            page2->addChild(a12);
            page2->addChild(a22);
            page2->addChild(a32);
        }
        pageG->addChild(page2);
        return pageG;
    }
    void onDraw() override {
        ImGui::Begin("bbb");
        Widget::onDraw();
        ImGui::End();
    }
};

class RootWidget3 : public Widget {
public:
    virtual ViewGroup* onCreateView() override
    {
        ViewGroup* pageG = new LinerVerticalLayout("pageG");
        ViewGroup* page2 = new LinerHorizonLayout("page2");
        {
            Button* a12 = new Button("aaa1");
            a12->setOnClickListenr([this](const View& const v)
                {
                    std::cout << (&v) << "(aa1)" << std::endl;
                    RouterBase* r = this->getRouter();
                    r->removeDisplay("P2");
                }
            );
            Button* a22 = new Button("aaa2");
            Button* a32 = new Button("aaa3");
            page2->addChild(a12);
            page2->addChild(a22);
            page2->addChild(a32);
        }
        pageG->addChild(page2);
        return pageG;
    }
    void onDraw() override {
        ImGui::Begin("ccc");
        Widget::onDraw();
        ImGui::End();
    }
};


class Router : public RouterBase {
    using PWidget = typename std::shared_ptr<Widget>;
    struct Page {
        PWidget widget;
    };
    std::unordered_map<std::string, PWidget> _widget;
    std::unordered_map<std::string, PWidget> _displayWidget;
    std::unordered_map<std::string, PWidget> _addRequest;
    std::unordered_map<std::string, PWidget> _removeRequest;
public:
    void add(std::string name, Widget* w) override {
        w->setRouter(this);
        _widget.emplace(name, PWidget(w));
    }
    void addDisplay(std::string name) override {
        if (_displayWidget.find(name) == _displayWidget.end()) {
            //_displayWidget.emplace(name, _widget.at(name));
            _addRequest.emplace(name, _widget.at(name));
        }
    }
    void removeDisplay(std::string name) override {
        if (_displayWidget.find(name) != _displayWidget.end()) {
            //_displayWidget.erase(name);
            _removeRequest.emplace(name, _widget.at(name));
        }
    }
    void onDraw() {
        for (auto pair : _addRequest) {
            _displayWidget.emplace(pair.first, pair.second);
        }
        _addRequest.clear();
        for (auto pair : _removeRequest) {
            _displayWidget.erase(pair.first);
        }
        _removeRequest.clear();
        for (auto pair : _displayWidget) {
            pair.second.get()->onDraw();
        }
    }
};

int main(int argc, char** argv) {
	std::cout << argv[0] << std::endl;


    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Dear ImGui DirectX11 Example"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
    //io.ConfigViewportsNoDefaultParent = true;
    //io.ConfigDockingAlwaysTabBar = true;
    //io.ConfigDockingTransparentPayload = true;
    //io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // FIXME-DPI: Experimental. THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T USE IN USER APP!
    //io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI: Experimental.

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    Button b("b1");
    OnClickListener bb = [](const View& const v) { std::cout << (&v) << "(S)" << std::endl; };
    b.setOnClickListenr([](const View& const v)  { std::cout << (&v) << "(SS)" << std::endl; });


    //LinerHorizonLayout page1("page1");
    ////{
    //    Button a1("aa1");
    //    Button a2("aa2");
    //    Button a3("aa3");
    //    page1.addChild(&a1);
    //    page1.addChild(&a2);
    //    page1.addChild(&a3);
    ////}

    //LinerHorizonLayout page2("page2");
    ////{
    //    Button a12("bb1");
    //    Button a22("bb2");
    //    Button a32("bb3");
    //    page2.addChild(&a12);
    //    page2.addChild(&a22);
    //    page2.addChild(&a32);
    ////}

    //LinerVerticalLayout pageG("pageG");
    //pageG.addChild(&page1);
    //pageG.addChild(&page2);


    Widget* r1 = new RootWidget();
    r1->onCreate();
    Widget* r2 = new RootWidget2();
    r2->onCreate();
    Widget* r3 = new RootWidget3();
    r3->onCreate();

    Router router;
    router.add("ROOT", r1);
    router.add("P1", r2);
    router.add("P2", r3);
    router.addDisplay("ROOT");
    //router.removeDisplay("ROOT");
    //router.addDisplay("P1");


    std::list<std::unique_ptr<View>> v;
    for (const auto& vv : v) {
        vv.get()->draw();
    }


    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        {

            //ImGui::Begin("A");
            //ImGui::Button("a");
            //{
            //    ImGui::BeginChild("B");
            //    {
            //        ImGui::BeginChild("C");
            //        ImGui::Button("c");
            //        ImGui::EndChild();
            //    }
            //    ImGui::EndChild();
            //}

            //ImGui::Begin("A2");
            //{
            //    ImGui::BeginChild("B2");
            //    {
            //        ImGui::BeginChild("C2");
            //        ImGui::Button("c");
            //        ImGui::EndChild();
            //    }
            //    ImGui::EndChild();
            //}
            //ImGui::End();
            //ImGui::End();

            //page.draw();
            //ImGui::Begin("A2");
            //pageG.draw();
            //ImGui::End();

            router.onDraw();
        }


        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
        {
            //const int dpi = HIWORD(wParam);
            //printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
            const RECT* suggested_rect = (RECT*)lParam;
            ::SetWindowPos(hWnd, NULL, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
