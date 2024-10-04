#include <stdHeaderFiles.h>
#include <vtkHeaderFiles.h>

#include <openvdb/openvdb.h>

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <Eigen/IterativeLinearSolvers>

#include <App/CustomTrackballStyle.h>
#include <App/Utility.h>

#include <Debugging/VisualDebugging.h>

// Key press event handler function
void OnKeyPress(vtkObject* caller, long unsigned int eventId, void* clientData, void* callData) {
    vtkRenderWindowInteractor* interactor = static_cast<vtkRenderWindowInteractor*>(caller);
    std::string key = interactor->GetKeySym(); // Get the key symbol pressed

    printf("%s\n", key.c_str());

    if (key == "r") {
        std::cout << "Key 'r' was pressed. Resetting camera." << std::endl;
        vtkRenderer* renderer = static_cast<vtkRenderer*>(clientData);

        vtkCamera* camera = renderer->GetActiveCamera();
        renderer->ResetCamera();

        // Reset the camera position, focal point, and view up vector to their defaults
        camera->SetPosition(0, 0, 1);          // Reset position to default
        camera->SetFocalPoint(0, 0, 0);        // Reset focal point to origin
        camera->SetViewUp(0, 1, 0);            // Reset the up vector to default (Y-axis up)

        interactor->Render(); // Render after camera reset
    }
    else if (key == "Escape") {
        std::cout << "Key 'Escape' was pressed. Exiting." << std::endl;
        interactor->TerminateApp();
    }
}

class TimerCallback : public vtkCommand
{
public:
    static TimerCallback* New() { return new TimerCallback; }

    TimerCallback() = default;

    virtual void Execute(vtkObject* caller, unsigned long eventId, void* vtkNotUsed(callData)) override
        //virtual void Execute(vtkObject *vtkNotUsed(caller),
        //                     unsigned long vtkNotUsed(eventId),
        //                     void *vtkNotUsed(callData)) override
    {
        if (eventId == vtkCommand::TimerEvent) {
            animate();
        }
        else {
            std::cerr << "Unexpected event ID: " << eventId << std::endl;
        }
    }

private:
    void animate() { VisualDebugging::Update(); }
};


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
    //vtkNew<vtkInteractorStyleTrackballCamera> trackballStyle;
    //interactor->SetInteractorStyle(trackballStyle);
    vtkNew<CustomTrackballStyle> customTrackballStyle;
    interactor->SetInteractorStyle(customTrackballStyle);
    interactor->SetRenderWindow(renderWindow);
    interactor->Initialize();

    VisualDebugging::Initialize(renderer);

    VisualDebugging::AddLine("axes", { 0, 0, 0 }, { 100, 0, 0 }, 255, 0, 0);
    VisualDebugging::AddLine("axes", { 0, 0, 0 }, { 0, 100, 0 }, 0, 255, 0);
    VisualDebugging::AddLine("axes", { 0, 0, 0 }, { 0, 0, 100 }, 0, 0, 255);

    // Maximize VTK rendering window on 2nd monitor
    MaximizeVTKWindowOnMonitor(renderWindow, 2); // Monitor index starts from 0

    // Read the input points from a PLY file
    //auto inputPoints = ReadPLY("C:\\Resources\\Debug\\HD\\patch_1.ply");
    auto inputPoints = ReadPLY("C:\\Resources\\Debug\\ZeroCrossingPoints.ply");
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

    {
        for (size_t i = 0; i < inputPoints->GetNumberOfPoints(); i++)
        {
            auto p = inputPoints->GetPoint(i);
            VisualDebugging::AddSphere("Spheres", { (float)p[0], (float)p[1], (float)p[2] }, { 1, 1, 1 }, { 0, 0, 0 }, 255, 255, 255);
        }
    }

    // Create a key press event handler
    vtkSmartPointer<vtkCallbackCommand> keyPressCallback = vtkSmartPointer<vtkCallbackCommand>::New();
    keyPressCallback->SetCallback(OnKeyPress);
    keyPressCallback->SetClientData(renderer); // Pass the renderer to the callback

    vtkSmartPointer<TimerCallback> timerCallback = vtkSmartPointer<TimerCallback>::New();

    interactor->AddObserver(vtkCommand::TimerEvent, timerCallback);
    int timerId = interactor->CreateRepeatingTimer(16);
    if (timerId < 0) {
        std::cerr << "Error: Timer was not created!" << std::endl;
    }

    // Add the observer to the interactor to listen for key presses
    interactor->AddObserver(vtkCommand::KeyPressEvent, keyPressCallback);

    renderWindow->Render();
    interactor->Start();

    VisualDebugging::Terminate();

    return 0;
}
