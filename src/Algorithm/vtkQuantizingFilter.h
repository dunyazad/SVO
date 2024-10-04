#pragma once

#include <Common.h>

class vtkQuantizingFilter : public vtkPolyDataAlgorithm
{
public:
    static vtkQuantizingFilter* New();
    vtkQuantizingFilter(vtkQuantizingFilter, vtkPolyDataAlgorithm);

protected:
    vtkQuantizingFilter() {}
    ~vtkQuantizingFilter() override {}

    int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
};
