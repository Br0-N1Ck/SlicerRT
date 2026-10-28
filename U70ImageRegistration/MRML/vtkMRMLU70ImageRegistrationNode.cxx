// Qt includes
//#include <QDebug>

//#include <vtkTransform.h>
// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
//#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

#include <cstdlib>
#include <string>

#include "vtkMRMLU70ImageRegistrationNode.h"

//------------------------------------------------------------------------------
namespace
{
    static const char* HORIZONTAL_TRANSFORM_NODE_REFERENCE_ROLE = "horizontalTransformNodeRef";
    static const char* VERTICAL_TRANSFORM_NODE_REFERENCE_ROLE = "verticalTransformNodeRef";
    static const char* HORIZONTAL_SCALE_NODE_REFERENCE_ROLE = "horizontalScaleNodeRef";
    static const char* VERTICAL_SCALE_NODE_REFERENCE_ROLE = "verticalScaleNodeRef";
    static const char* DRR_NODE_REFERENCE_ROLE = "drrNodeRef";
    static const char* XRAY_NODE_REFERENCE_ROLE = "xrayNodeRef";
    static const char* ACTIVE_ORIENTATION_ATTRIBUTE_NAME = "ActiveOrientation";

}

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLU70ImageRegistrationNode);

vtkMRMLU70ImageRegistrationNode::vtkMRMLU70ImageRegistrationNode()
{
 //   vtkNew< vtkTransform > horizTransform, verticalTransform, horizScale, verticalScale;
 //   horizTransform->Identity();
 //   verticalTransform->Identity();
 //   horizScale->Identity();
 //   verticalScale->Identity();
}

//----------------------------------------------------------------------------
vtkMRMLU70ImageRegistrationNode::~vtkMRMLU70ImageRegistrationNode()
{
    this->SetAndObserveHorizontalTransformNode(nullptr);
    this->SetAndObserveVerticalTransformNode(nullptr);
    this->SetAndObserveHorizontalScaleNode(nullptr);
    this->SetAndObserveVerticalScaleNode(nullptr);
    this->SetAndObserveDrrNode(nullptr);
    this->SetAndObserveXrayNode(nullptr);
}

void vtkMRMLU70ImageRegistrationNode::WriteXML(ostream& of, int nIndent)
{
    Superclass::WriteXML(of, nIndent);

    // Write all MRML node attributes into output stream
    vtkMRMLWriteXMLBeginMacro(of);


     // add new parameters here
    vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLU70ImageRegistrationNode::ReadXMLAttributes(const char** atts)
{
    int disabledModify = this->StartModify();
    vtkMRMLNode::ReadXMLAttributes(atts);

    vtkMRMLReadXMLBeginMacro(atts);



      // add new parameters here
    vtkMRMLReadXMLEndMacro();

    this->EndModify(disabledModify);

    // Note: ReportString is not read from XML, it is a strictly temporary value
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
void vtkMRMLU70ImageRegistrationNode::Copy(vtkMRMLNode* anode)
{
    int disabledModify = this->StartModify();

    Superclass::Copy(anode);

    vtkMRMLU70ImageRegistrationNode* node = vtkMRMLU70ImageRegistrationNode::SafeDownCast(anode);
    if (!node)
    {
        return;
    }

    this->DisableModifiedEventOn();

    vtkMRMLCopyBeginMacro(node);

    this->SetActiveOrientation(node->GetActiveOrientation());
    vtkMRMLCopyEndMacro();

    this->EndModify(disabledModify);

    this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLU70ImageRegistrationNode::CopyContent(vtkMRMLNode* anode, bool deepCopy/*=true*/)
{
    MRMLNodeModifyBlocker blocker(this);
    Superclass::CopyContent(anode, deepCopy);

    vtkMRMLU70ImageRegistrationNode* node = vtkMRMLU70ImageRegistrationNode::SafeDownCast(anode);
    if (!node)
    {
        return;
    }

    vtkMRMLCopyBeginMacro(node);

    this->SetActiveOrientation(node->GetActiveOrientation());
    vtkMRMLCopyEndMacro();
}

void vtkMRMLU70ImageRegistrationNode::PrintSelf(ostream& os, vtkIndent indent)
{
    Superclass::PrintSelf(os, indent);

    vtkMRMLPrintBeginMacro(os, indent);

    os << indent << "HorizontalTransformNode: " << (this->GetHorizontalTransformNode() ? this->GetHorizontalTransformNode()->GetName() : "null") << "\n";
    os << indent << "VerticalTransformNode: " << (this->GetVerticalTransformNode() ? this->GetVerticalTransformNode()->GetName() : "null") << "\n";
    os << indent << "HorizontalScaleNode: " << (this->GetHorizontalScaleNode() ? this->GetHorizontalScaleNode()->GetName() : "null") << "\n";
    os << indent << "VerticalScaleNode: " << (this->GetVerticalScaleNode() ? this->GetVerticalScaleNode()->GetName() : "null") << "\n";
    os << indent << "DrrNode: " << (this->GetDrrNode() ? this->GetDrrNode()->GetName() : "null") << "\n";
    os << indent << "XrayNode: " << (this->GetXrayNode() ? this->GetXrayNode()->GetName() : "null") << "\n";
    os << indent << "ActiveOrientation: " << this->GetActiveOrientation() << "\n";

    // add new parameters here
    vtkMRMLPrintEndMacro();

}
/*
void vtkMRMLU70ImageRegistrationNode::ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData)
{
  Superclass::ProcessMRMLEvents(caller, eventID, callData);

  if (!this->Scene)
  {
    vtkErrorMacro("ProcessMRMLEvents: Invalid MRML scene");
    return;
  }
  if (this->Scene->IsBatchProcessing())
  {
    return;
  }

}
*/

int vtkMRMLU70ImageRegistrationNode::GetActiveOrientation()
{
    const char* orientationAttributeValue = this->GetAttribute(ACTIVE_ORIENTATION_ATTRIBUTE_NAME);
    if (!orientationAttributeValue)
    {
      return vtkMRMLU70ImageRegistrationNode::OrientationHorizontal;
    }
    return atoi(orientationAttributeValue);
}

vtkMRMLLinearTransformNode* vtkMRMLU70ImageRegistrationNode::GetHorizontalTransformNode()
{
    return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(HORIZONTAL_TRANSFORM_NODE_REFERENCE_ROLE));
}

vtkMRMLLinearTransformNode* vtkMRMLU70ImageRegistrationNode::GetVerticalTransformNode()
{
    return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(VERTICAL_TRANSFORM_NODE_REFERENCE_ROLE));
}

vtkMRMLLinearTransformNode* vtkMRMLU70ImageRegistrationNode::GetHorizontalScaleNode()
{
    return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(HORIZONTAL_SCALE_NODE_REFERENCE_ROLE));
}

vtkMRMLLinearTransformNode* vtkMRMLU70ImageRegistrationNode::GetVerticalScaleNode()
{
    return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(VERTICAL_SCALE_NODE_REFERENCE_ROLE));
}
vtkMRMLScalarVolumeNode* vtkMRMLU70ImageRegistrationNode::GetDrrNode()
{
    return vtkMRMLScalarVolumeNode::SafeDownCast(this->GetNodeReference(DRR_NODE_REFERENCE_ROLE));
}

vtkMRMLScalarVolumeNode* vtkMRMLU70ImageRegistrationNode::GetXrayNode()
{
    return vtkMRMLScalarVolumeNode::SafeDownCast(this->GetNodeReference(XRAY_NODE_REFERENCE_ROLE));
}


void vtkMRMLU70ImageRegistrationNode::SetActiveOrientation(int orientation)
{
    this->SetAttribute(ACTIVE_ORIENTATION_ATTRIBUTE_NAME, std::to_string(orientation).c_str());
}

void vtkMRMLU70ImageRegistrationNode::SetAndObserveHorizontalTransformNode(vtkMRMLLinearTransformNode* node)
{
    if (node && this->Scene != node->GetScene())
    {
        vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
        return;
    }

    this->SetNodeReferenceID(HORIZONTAL_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

void vtkMRMLU70ImageRegistrationNode::SetAndObserveVerticalTransformNode(vtkMRMLLinearTransformNode* node)
{
    if (node && this->Scene != node->GetScene())
    {
        vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
        return;
    }

    this->SetNodeReferenceID(VERTICAL_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

void vtkMRMLU70ImageRegistrationNode::SetAndObserveHorizontalScaleNode(vtkMRMLLinearTransformNode* node)
{
    if (node && this->Scene != node->GetScene())
    {
        vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
        return;
    }
    this->SetNodeReferenceID(HORIZONTAL_SCALE_NODE_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

void vtkMRMLU70ImageRegistrationNode::SetAndObserveVerticalScaleNode(vtkMRMLLinearTransformNode* node)
{
    if (node && this->Scene != node->GetScene())
    {
        vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
        return;
    }
    this->SetNodeReferenceID(VERTICAL_SCALE_NODE_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}
void vtkMRMLU70ImageRegistrationNode::SetAndObserveDrrNode(vtkMRMLScalarVolumeNode* node)
{
    if (node && this->Scene != node->GetScene())
    {
        vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
        return;
    }

    this->SetNodeReferenceID(DRR_NODE_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}

void vtkMRMLU70ImageRegistrationNode::SetAndObserveXrayNode(vtkMRMLScalarVolumeNode* node)
{
    if (node && this->Scene != node->GetScene())
    {
        vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
        return;
    }

    this->SetNodeReferenceID(XRAY_NODE_REFERENCE_ROLE, (node ? node->GetID() : nullptr));
}