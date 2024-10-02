#include <openvdb/openvdb.h>
#include <vtkKdTree.h>
#include <vtkSmartPointer.h>
#include <vtkPLYReader.h>
#include <vtkPLYWriter.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkProperty.h>
#include <vtkTriangle.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkDelaunay3D.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <windows.h>
#include <shellapi.h>
using namespace std;

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <Eigen/IterativeLinearSolvers>

chrono::steady_clock::time_point Now()
{
    return chrono::high_resolution_clock::now();
}
string Miliseconds(const chrono::steady_clock::time_point beginTime, const char* tag = nullptr);
#define TS(name) auto time_##name = Now();

string Miliseconds(const chrono::steady_clock::time_point beginTime, const char* tag)
{
    auto now = Now();
    auto timeSpan = chrono::duration_cast<chrono::nanoseconds>(now - beginTime).count();
    stringstream ss;
    ss << "[[[ ";
    if (nullptr != tag)
    {
        ss << tag << " - ";
    }
    ss << (float)timeSpan / 1000000.0 << " ms ]]]";
    return ss.str();
}
#define TE(name) std::cout << Miliseconds(time_##name, #name) << std::endl;

vtkSmartPointer<vtkPolyData> ReadPLY(const std::string& filePath) {
    vtkSmartPointer<vtkPLYReader> reader = vtkSmartPointer<vtkPLYReader>::New();
    reader->SetFileName(filePath.c_str());
    reader->Update();
    return reader->GetOutput();
}

void WritePLY(vtkSmartPointer<vtkPolyData> data, const std::string& filePath) {
    vtkSmartPointer<vtkPLYWriter> writer = vtkSmartPointer<vtkPLYWriter>::New();
    writer->SetFileName(filePath.c_str());
    writer->SetInputData(data);
    writer->Update();
}

// Helper structure for storing monitor information
struct MonitorInfo {
    HMONITOR hMonitor;
    MONITORINFO monitorInfo;
};

// Function to enumerate monitors and get monitor information
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    std::vector<MonitorInfo>* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(dwData);
    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfo(hMonitor, &monitorInfo)) {
        monitors->push_back({ hMonitor, monitorInfo });
    }
    return TRUE;
}

// Function to maximize the console window on a specific monitor (by index)
void MaximizeConsoleWindowOnMonitor(int monitorIndex) {
    HWND consoleWindow = GetConsoleWindow();
    if (!consoleWindow) return;

    std::vector<MonitorInfo> monitors;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));

    if (monitorIndex >= 0 && monitorIndex < monitors.size()) {
        const MonitorInfo& monitor = monitors[monitorIndex];
        RECT workArea = monitor.monitorInfo.rcWork;

        MoveWindow(consoleWindow, workArea.left, workArea.top,
            workArea.right - workArea.left,
            workArea.bottom - workArea.top, TRUE);
        ShowWindow(consoleWindow, SW_MAXIMIZE);
    }
}

// Function to maximize the VTK rendering window on a specific monitor (by index)
void MaximizeVTKWindowOnMonitor(vtkSmartPointer<vtkRenderWindow> renderWindow, int monitorIndex) {
    std::vector<MonitorInfo> monitors;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));

    if (monitorIndex >= 0 && monitorIndex < monitors.size()) {
        const MonitorInfo& monitor = monitors[monitorIndex];
        RECT workArea = monitor.monitorInfo.rcWork;

        // Set the position and size of the VTK window to match the monitor's work area
        //renderWindow->SetPosition(workArea.left, workArea.top);
        //renderWindow->SetSize(workArea.right - workArea.left, workArea.bottom - workArea.top);

        HWND hwnd = (HWND)renderWindow->GetGenericWindowId();

        MoveWindow(hwnd, workArea.left, workArea.top,
            workArea.right - workArea.left,
            workArea.bottom - workArea.top, TRUE);

        ShowWindow(hwnd, SW_MAXIMIZE);
    }
}

// Key press event handler function
void OnKeyPress(vtkObject* caller, long unsigned int eventId, void* clientData, void* callData) {
    vtkRenderWindowInteractor* interactor = static_cast<vtkRenderWindowInteractor*>(caller);
    std::string key = interactor->GetKeySym(); // Get the key symbol pressed

    printf("%s\n", key.c_str());

    if (key == "r") {
        std::cout << "Key 'r' was pressed. Resetting camera." << std::endl;
        vtkRenderer* renderer = static_cast<vtkRenderer*>(clientData);

        vtkCamera* camera = renderer->GetActiveCamera();

        // Reset the camera position, focal point, and view up vector to their defaults
        camera->SetPosition(0, 0, 1);          // Reset position to default
        camera->SetFocalPoint(0, 0, 0);        // Reset focal point to origin
        camera->SetViewUp(0, 1, 0);            // Reset the up vector to default (Y-axis up)

        renderer->ResetCamera();
        interactor->Render(); // Render after camera reset
    }
    else if (key == "Escape") {
        std::cout << "Key 'Escape' was pressed. Exiting." << std::endl;
        interactor->TerminateApp();
    }
}

int main() {
    openvdb::initialize();

    // Maximize console window on 3rd monitor
    MaximizeConsoleWindowOnMonitor(1); // Monitor index starts from 0

    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->SetBackground(0.3, 0.5, 0.7);

    vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->SetSize(1920, 1080);
    renderWindow->AddRenderer(renderer);

    vtkSmartPointer<vtkRenderWindowInteractor> interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    vtkNew<vtkInteractorStyleTrackballCamera> trackballStyle;
    interactor->SetInteractorStyle(trackballStyle);
    interactor->SetRenderWindow(renderWindow);

    // Maximize VTK rendering window on 2nd monitor
    MaximizeVTKWindowOnMonitor(renderWindow, 2); // Monitor index starts from 0

    // Read the input points from a PLY file
    auto inputPoints = ReadPLY("C:\\Resources\\Debug\\HD\\patch_1.ply");

    {
        vtkSmartPointer<vtkVertexGlyphFilter> vertexFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
        vertexFilter->SetInputData(inputPoints);
        vertexFilter->Update();

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(vertexFilter->GetOutputPort());

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(1.0, 1.0, 1.0);

        renderer->AddActor(actor);
    }

    // Create a key press event handler
    vtkSmartPointer<vtkCallbackCommand> keyPressCallback = vtkSmartPointer<vtkCallbackCommand>::New();
    keyPressCallback->SetCallback(OnKeyPress);
    keyPressCallback->SetClientData(renderer); // Pass the renderer to the callback

    // Add the observer to the interactor to listen for key presses
    interactor->AddObserver(vtkCommand::KeyPressEvent, keyPressCallback);

    renderWindow->Render();
    interactor->Start();

    return 0;
}
