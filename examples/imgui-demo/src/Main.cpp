// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

#include <stdio.h>
#include <fstream>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "hscpp/Filesystem.h"
#include "hscpp/Hotswapper.h"
#include "hscpp/Util.h"
#include "hscpp-example-utils/MemoryManager.h"
#include "hscpp-example-utils/Ref.h"
#include "imgui-demo/Widget.h"
#include "imgui-demo/Globals.h"

const static hscpp::fs::path DEMO_PATH = hscpp::util::GetHscppExamplesPath() / "imgui-demo";

static bool SetupGlfw(GLFWwindow*& pWindow)
{
    if (!glfwInit())
    {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    pWindow = glfwCreateWindow(1280, 720, "ImGui Demo", nullptr, nullptr);
    if (pWindow == nullptr)
    {
        return false;
    }

    glfwMakeContextCurrent(pWindow);
    glfwSwapInterval(1); // Enable vsync.

    return true;
}

static bool SetupGl3w()
{
    if (gl3wInit() != 0)
    {
        return false;
    }

    return true;
}

static bool SetupImGui(GLFWwindow* pWindow, ImGuiContext*& pImGuiContext)
{
    IMGUI_CHECKVERSION();

    pImGuiContext = ImGui::CreateContext();

    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(pWindow, true))
    {
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 150"))
    {
        return false;
    }

    return true;
}

int main(int, char**)
{
    hscpp::Hotswapper swapper;

    auto srcPath = DEMO_PATH / "src";
    auto includePath = DEMO_PATH / "include";
    auto exampleUtilsIncludePath = hscpp::util::GetHscppExamplesPath() / "hscpp-example-utils" / "include";
    auto imguiIncludePath = hscpp::util::GetHscppExamplesPath()  / "lib" / "imgui";

    swapper.AddSourceDirectory(srcPath);
    swapper.AddIncludeDirectory(includePath);
    swapper.AddIncludeDirectory(exampleUtilsIncludePath);
    swapper.AddIncludeDirectory(imguiIncludePath);

//#ifdef _DEBUG
//    std::string configuration = "Debug";
//#else
//    std::string configuration = "Release";
//#endif

    // TODO fix configuration

    // We can link additional libraries.
    auto imguiLibraryPath = hscpp::fs::current_path().parent_path()
        / "lib" / "imgui" / "imgui.lib";
    auto exampleUtilsLibraryPath = hscpp::fs::current_path().parent_path()
        / "hscpp-example-utils" / "hscpp-example-utils.lib";
    
    swapper.AddLibrary(imguiLibraryPath);
    swapper.AddLibrary(exampleUtilsLibraryPath);

    GLFWwindow* pWindow = nullptr;
    if (!SetupGlfw(pWindow))
    {
        std::cout << "Failed to create GLFW window." << std::endl;
        return -1;
    }

    if (!SetupGl3w())
    {
        std::cout << "Failed to setup gl3w." << std::endl;
        return -1;
    }

    ImGuiContext* pImGuiContext = nullptr;
    if (!SetupImGui(pWindow, pImGuiContext))
    {
        std::cout << "Failed to setup ImGui." << std::endl;
        return -1;
    }

    hscpp::AllocationResolver* pAllocationResolver = swapper.GetAllocationResolver();
    Ref<MemoryManager> memoryManager = MemoryManager::Create(pAllocationResolver);

    swapper.SetAllocator(&memoryManager);

    // Refs can only refer to memory within our memory allocator. The "Place" function allows the
    // MemoryManager to keep track of memory without owning it.
    Ref<ImGuiContext> imguiContext = memoryManager->Place(pImGuiContext);

    // Statics and globals are per-module, hence we must make use of ModuleSharedState. To avoid
    // making the whole codebase dependent on hscpp, we can wrap our globals into a Globals class.
    Globals::Init(memoryManager, imguiContext);

    // Globals is now shared as s_pGlobalUserData in ModuleSharedState.
    swapper.SetGlobalUserData(&Globals::Instance());

    Ref<Widget> widget = memoryManager->Allocate<Widget>();
    widget->Init("<root>", "Widget");

    while (!glfwWindowShouldClose(pWindow))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        swapper.Update();
        widget->Update();

        ImGui::Render();

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(pWindow, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.45f, 0.55f, 0.6f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(pWindow);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(pWindow);
    glfwTerminate();

    memoryManager->Free(widget);

    return 0;
}