#ifndef __vtkMRMLU70ImageRegistrationNode_h
#define __vtkMRMLU70ImageRegistrationNode_h

#include "vtkSlicerU70ImageRegistrationModuleMRMLExport.h"


// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkSmartPointer.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScalarVolumeNode.h>

class VTK_SLICER_U70IMAGEREGISTRATION_MODULE_MRML_EXPORT vtkMRMLU70ImageRegistrationNode : public vtkMRMLNode
{
public:
    enum OrientationType
    {
      OrientationHorizontal = 0,
      OrientationVertical = 1
    };

    static vtkMRMLU70ImageRegistrationNode* New();
    vtkTypeMacro(vtkMRMLU70ImageRegistrationNode, vtkMRMLNode);
    void PrintSelf(ostream& os, vtkIndent indent) override;
    /// Create instance of a GAD node.
    vtkMRMLNode* CreateNodeInstance() override;

    /// Set node attributes from name/value pairs

    void ReadXMLAttributes(const char** atts) override;

    /// Write this node's information to a MRML file in XML format.
    void WriteXML(ostream& of, int indent) override;

    /// Copy the node's attributes to this object
    void Copy(vtkMRMLNode* node) override;

    /// Copy node content (excludes basic data, such a name and node reference)
    vtkMRMLCopyContentMacro(vtkMRMLU70ImageRegistrationNode);


    /// Get unique node XML tag name
    const char* GetNodeTagName() override { return "U70ImageRegistration"; };

    /// Handles events registered in the observer manager
//    void ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData) override;

    int GetActiveOrientation();
    vtkMRMLLinearTransformNode* GetHorizontalTransformNode();
    vtkMRMLLinearTransformNode* GetVerticalTransformNode();
    vtkMRMLLinearTransformNode* GetHorizontalScaleNode();
    vtkMRMLLinearTransformNode* GetVerticalScaleNode();
    vtkMRMLScalarVolumeNode* GetDrrNode();
    vtkMRMLScalarVolumeNode* GetXrayNode();

    void SetActiveOrientation(int orientation);
    void SetAndObserveHorizontalTransformNode(vtkMRMLLinearTransformNode* node);
    void SetAndObserveVerticalTransformNode(vtkMRMLLinearTransformNode* node);
    void SetAndObserveHorizontalScaleNode(vtkMRMLLinearTransformNode* node);
    void SetAndObserveVerticalScaleNode(vtkMRMLLinearTransformNode* node);
    void SetAndObserveDrrNode(vtkMRMLScalarVolumeNode* node);
    void SetAndObserveXrayNode(vtkMRMLScalarVolumeNode* node);

 //   vtkGetMacro(Height, double);
 //   vtkSetMacro(Height, double);

  protected:
    vtkMRMLU70ImageRegistrationNode();
    virtual ~vtkMRMLU70ImageRegistrationNode();
    vtkMRMLU70ImageRegistrationNode(const vtkMRMLU70ImageRegistrationNode&);
    void operator=(const vtkMRMLU70ImageRegistrationNode&);
};

#endif