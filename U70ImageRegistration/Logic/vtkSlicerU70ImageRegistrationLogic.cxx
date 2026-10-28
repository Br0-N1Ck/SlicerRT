/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// U70ImageRegistration Logic includes
#include "vtkSlicerU70ImageRegistrationLogic.h"
#include "vtkMRMLU70ImageRegistrationNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkImageData.h>
#include <vtkImageCast.h>
#include <vtkImageGradient.h>
#include <vtkImageMagnitude.h>

// STD includes
#include <cassert>
//#include <string>

namespace
{
vtkMRMLLinearTransformNode* GetOrCreateNamedTransformNode(vtkMRMLScene* scene, const char* name)
{
  if (!scene || !name)
  {
    return nullptr;
  }

  vtkMRMLLinearTransformNode* linearTransformNode =
    vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName(name));
  if (linearTransformNode)
  {
    return linearTransformNode;
  }

  vtkNew<vtkMRMLLinearTransformNode> newTransformNode;
  newTransformNode->SetName(name);
  scene->AddNode(newTransformNode);
  return newTransformNode.GetPointer();
}
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerU70ImageRegistrationLogic);

//----------------------------------------------------------------------------
vtkSlicerU70ImageRegistrationLogic::vtkSlicerU70ImageRegistrationLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerU70ImageRegistrationLogic::~vtkSlicerU70ImageRegistrationLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerU70ImageRegistrationLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerU70ImageRegistrationLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerU70ImageRegistrationLogic::RegisterNodes()
{
    vtkMRMLScene* scene = this->GetMRMLScene();
    if (!scene)
    {
        return;
    }

    if (!scene->IsNodeClassRegistered("vtkMRMLU70ImageRegistrationNode"))
    {
        scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLU70ImageRegistrationNode>::New());
    }
}

//---------------------------------------------------------------------------
void vtkSlicerU70ImageRegistrationLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerU70ImageRegistrationLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
    if (!node || !this->GetMRMLScene())
    {
        vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or input node");
        return;
    }

    if (node->IsA("vtkMRMLU70ImageRegistrationNode"))
    {
        vtkNew<vtkIntArray> events;
        events->InsertNextValue(vtkCommand::ModifiedEvent);
        vtkObserveMRMLNodeEventsMacro(node, events);
    }
}

//---------------------------------------------------------------------------
void vtkSlicerU70ImageRegistrationLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
vtkMRMLU70ImageRegistrationNode* vtkSlicerU70ImageRegistrationLogic::GetOrCreateU70ImageRegistrationNode()
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    return nullptr;
  }

  vtkMRMLU70ImageRegistrationNode* imageRegistrationNode = vtkMRMLU70ImageRegistrationNode::SafeDownCast(
    scene->GetFirstNodeByClass("vtkMRMLU70ImageRegistrationNode"));
  if (imageRegistrationNode)
  {
    return imageRegistrationNode;
  }

  vtkNew<vtkMRMLU70ImageRegistrationNode> newU70ImageRegistrationNode;
  scene->AddNode(newU70ImageRegistrationNode);
  return newU70ImageRegistrationNode.GetPointer();
}

//---------------------------------------------------------------------------
//void vtkSlicerU70ImageRegistrationLogic::SetDrrNode(vtkMRMLScalarVolumeNode* drrNode)
//{
//  vtkMRMLU70ImageRegistrationNode* imagePositioningNode = this->GetOrCreateImagePositioningNode();
//  if (!imagePositioningNode)
//  {
//    return;
//  }
//  imagePositioningNode->SetAndObserveDrrNode(drrNode);
//}

//---------------------------------------------------------------------------
void vtkSlicerU70ImageRegistrationLogic::SetXrayNode(vtkMRMLScalarVolumeNode* xrayNode)
{
  vtkMRMLU70ImageRegistrationNode* imageRegistrationNode = this->GetOrCreateU70ImageRegistrationNode();
  if (!imageRegistrationNode)
  {
    return;
  }
  imageRegistrationNode->SetAndObserveXrayNode(xrayNode);
  if (!xrayNode)
  {
    return;
  }

  vtkMRMLScene* scene = xrayNode->GetScene();
  if (!scene)
  {
    return;
  }

  vtkMRMLLinearTransformNode* horizontalTransformNode =
    GetOrCreateNamedTransformNode(scene, "XrayTransformHorizontal");
  vtkMRMLLinearTransformNode* verticalTransformNode =
    GetOrCreateNamedTransformNode(scene, "XrayTransformVertical");
  vtkMRMLLinearTransformNode* horizontalScaleNode =
    GetOrCreateNamedTransformNode(scene, "XrayScaleHorizontal");
  vtkMRMLLinearTransformNode* verticalScaleNode =
    GetOrCreateNamedTransformNode(scene, "XrayScaleVertical");
  imageRegistrationNode->SetAndObserveHorizontalTransformNode(horizontalTransformNode);
  imageRegistrationNode->SetAndObserveVerticalTransformNode(verticalTransformNode);
  imageRegistrationNode->SetAndObserveHorizontalScaleNode(horizontalScaleNode);
  imageRegistrationNode->SetAndObserveVerticalScaleNode(verticalScaleNode);

  vtkMRMLLinearTransformNode* activeTransformNode = this->GetActiveXrayTransformNode();
  vtkMRMLLinearTransformNode* activeScaleNode = this->GetActiveXrayScaleNode();
  if (activeTransformNode && activeScaleNode)
  {
 
    activeScaleNode->SetAndObserveTransformNodeID(activeTransformNode->GetID());
    xrayNode->SetAndObserveTransformNodeID(activeScaleNode->GetID());
  }
}

//---------------------------------------------------------------------------
void vtkSlicerU70ImageRegistrationLogic::SetActiveOrientation(int orientation)
{
  vtkMRMLU70ImageRegistrationNode* imageRegistrationNode = this->GetOrCreateU70ImageRegistrationNode();
  if (!imageRegistrationNode)
  {
    return;
  }
  imageRegistrationNode->SetActiveOrientation(orientation);

  vtkMRMLScalarVolumeNode* xrayNode = imageRegistrationNode->GetXrayNode();
  vtkMRMLLinearTransformNode* activeTransformNode = this->GetActiveXrayTransformNode();
  vtkMRMLLinearTransformNode* activeScaleNode = this->GetActiveXrayScaleNode();
  if (xrayNode && activeTransformNode && activeScaleNode)
  {
    activeScaleNode->SetAndObserveTransformNodeID(activeTransformNode->GetID());
    xrayNode->SetAndObserveTransformNodeID(activeScaleNode->GetID());
  }
}

//---------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkSlicerU70ImageRegistrationLogic::GetActiveXrayTransformNode()
{
  vtkMRMLU70ImageRegistrationNode* imageRegistrationNode = this->GetOrCreateU70ImageRegistrationNode();
  if (!imageRegistrationNode)
  {
    return nullptr;
  }

  if (imageRegistrationNode->GetActiveOrientation() == vtkMRMLU70ImageRegistrationNode::OrientationVertical)
  {
    return imageRegistrationNode->GetVerticalTransformNode();
  }
  return imageRegistrationNode->GetHorizontalTransformNode();
}

//---------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkSlicerU70ImageRegistrationLogic::GetActiveXrayScaleNode()
{
  vtkMRMLU70ImageRegistrationNode* imageRegistrationNode = this->GetOrCreateU70ImageRegistrationNode();
  if (!imageRegistrationNode)
  {
    return nullptr;
  }
  if (imageRegistrationNode->GetActiveOrientation() == vtkMRMLU70ImageRegistrationNode::OrientationVertical)
  {
    return imageRegistrationNode->GetVerticalScaleNode();
  }
  return imageRegistrationNode->GetHorizontalScaleNode();
}

//---------------------------------------------------------------------------
void vtkSlicerU70ImageRegistrationLogic::ApplyXray2DTransform(
  double horizontalOffsetMm, double verticalOffsetMm, double rotationDeg, int orientation)
{
  vtkMRMLU70ImageRegistrationNode* imageRegistrationNode = this->GetOrCreateU70ImageRegistrationNode();
  if (!imageRegistrationNode)
  {
    return;
  }

  vtkMRMLScalarVolumeNode* xrayNode = imageRegistrationNode->GetXrayNode();
  vtkMRMLLinearTransformNode* activeTransformNode = this->GetActiveXrayTransformNode();
  if (!xrayNode || !activeTransformNode)
  {
    return;
  }

  vtkImageData* imageData = xrayNode->GetImageData();
  if (!imageData)
  {
    return;
  }

  // Basis vectors depending on orientation
  double eHorizontal[3] = { 0.0, 0.0, 1.0 };
  double eVertical[3] = { 0.0, 1.0, 0.0 };
  double normal[3] = { -1.0, 0.0, 0.0 };
  if (orientation == vtkMRMLU70ImageRegistrationNode::OrientationVertical)
  {
    eHorizontal[0] = 1.0; eHorizontal[1] = 0.0; eHorizontal[2] = 0.0; // L-R
    eVertical[0] = 0.0; eVertical[1] = 0.0; eVertical[2] = 1.0;       // S-I
    normal[0] = 0.0; normal[1] = -1.0; normal[2] = 0.0;               // A-P
  }

  const double translationRAS[3] =
  {
    horizontalOffsetMm * eHorizontal[0] + verticalOffsetMm * eVertical[0],
    horizontalOffsetMm * eHorizontal[1] + verticalOffsetMm * eVertical[1],
    horizontalOffsetMm * eHorizontal[2] + verticalOffsetMm * eVertical[2]
  };

  int dimensions[3] = { 0, 0, 0 };
  imageData->GetDimensions(dimensions);
  double centerIJK[4] =
  {
    0.5 * static_cast<double>(dimensions[0] - 1),
    0.5 * static_cast<double>(dimensions[1] - 1),
    0.5 * static_cast<double>(dimensions[2] - 1),
    1.0
  };
  vtkNew<vtkMatrix4x4> ijkToRASMatrix;
  xrayNode->GetIJKToRASMatrix(ijkToRASMatrix);
  double centerRAS4[4] = { 0.0, 0.0, 0.0, 1.0 };
  ijkToRASMatrix->MultiplyPoint(centerIJK, centerRAS4);


  // Translate and rotate around image center
  vtkNew<vtkTransform> xrayTransform;
  xrayTransform->Identity();
  xrayTransform->Translate(centerRAS4[0], centerRAS4[1], centerRAS4[2]);
  xrayTransform->Translate(translationRAS[0], translationRAS[1], translationRAS[2]);
  xrayTransform->RotateWXYZ(rotationDeg, normal);
  xrayTransform->Translate(-centerRAS4[0], -centerRAS4[1], -centerRAS4[2]);

  activeTransformNode->SetMatrixTransformToParent(xrayTransform->GetMatrix());
}

//---------------------------------------------------------------------------
void vtkSlicerU70ImageRegistrationLogic::AlignAndScaleXrayToDrr(double sourceToImagerMm)
{
  vtkMRMLU70ImageRegistrationNode* imageRegistrationNode = this->GetOrCreateU70ImageRegistrationNode();
  if (!imageRegistrationNode)
  {
    return;
  }

  vtkMRMLScalarVolumeNode* xrayNode = imageRegistrationNode->GetXrayNode();
  vtkMRMLScalarVolumeNode* drrNode = imageRegistrationNode->GetDrrNode();
  if (!xrayNode || !drrNode)
  {
    return;
  }
  vtkMRMLScene* scene = xrayNode->GetScene();
  if (!scene)
  {
      return;
  }

  // Determine slice normal depending on orientation
  int orientation = imageRegistrationNode->GetActiveOrientation();
  double normal[3] = { -1.0, 0.0, 0.0 };
  if (orientation == vtkMRMLU70ImageRegistrationNode::OrientationVertical)
  {
    normal[0] = 0.0; normal[1] = -1.0; normal[2] = 0.0;
  }

  // Get isocenter plane (slice) origin from the XrayDetectorSlice slice node
  //vtkMRMLScene* scene = imagePositioningNode->GetScene();
  //vtkMRMLSliceNode* sliceNode = nullptr;
  //if (scene)
  //{
  //  sliceNode = vtkMRMLSliceNode::SafeDownCast(scene->GetFirstNodeByName("XrayDetectorSlice"));
  //}
  //if (!sliceNode)
  //{
  //  return;
  //}
  //vtkMatrix4x4* sliceToRAS = sliceNode->GetSliceToRAS();
  //double sliceOrigin[3] = { sliceToRAS->GetElement(0,3), sliceToRAS->GetElement(1,3), sliceToRAS->GetElement(2,3) };

  // Compute center of Xray in RAS (volume IJK->RAS)
  vtkImageData* xrayImageData = xrayNode->GetImageData();
  if (!xrayImageData)
  {
    return;
  }
  int xrayDimensions[3] = { 0, 0, 0 };
  xrayImageData->GetDimensions(xrayDimensions);
  double xrayCenterIJK[4] =
  {
    0.5 * static_cast<double>(xrayDimensions[0] - 1),
    0.5 * static_cast<double>(xrayDimensions[1] - 1),
    0.5 * static_cast<double>(xrayDimensions[2] - 1),
    1.0
  };
  vtkNew<vtkMatrix4x4> ijkToRASMatrix;
  xrayNode->GetIJKToRASMatrix(ijkToRASMatrix);
  double xrayCenterRAS4[4] = { 0.0, 0.0, 0.0, 1.0 };
  ijkToRASMatrix->MultiplyPoint(xrayCenterIJK, xrayCenterRAS4);

  // Compute DRR center in RAS
  vtkImageData* drrImageData = drrNode->GetImageData();
  if (!drrImageData)
  {
    return;
  }
  int drrDimensions[3] = { 0, 0, 0 };
  drrImageData->GetDimensions(drrDimensions);
  double drrCenterIJK[4] =
  {
    0.5 * static_cast<double>(drrDimensions[0] - 1),
    0.5 * static_cast<double>(drrDimensions[1] - 1),
    0.5 * static_cast<double>(drrDimensions[2] - 1),
    1.0
  };
  vtkNew<vtkMatrix4x4> drrIJKToRAS;
  drrNode->GetIJKToRASMatrix(drrIJKToRAS);
  double drrCenterRAS4[4] = { 0.0, 0.0, 0.0, 1.0 };
  drrIJKToRAS->MultiplyPoint(drrCenterIJK, drrCenterRAS4);

  double vecToSlice[3] = { 
      drrCenterRAS4[0] - xrayCenterRAS4[0],
      drrCenterRAS4[1] - xrayCenterRAS4[1],
      drrCenterRAS4[2] - xrayCenterRAS4[2]
  };
  double isocenterToImagerMm = vecToSlice[0]*normal[0] + vecToSlice[1]*normal[1] + vecToSlice[2]*normal[2];

  if (sourceToImagerMm == 0.0)
  {
    return;
  }

  double scaleFactor = 1.0 - (abs(isocenterToImagerMm) / sourceToImagerMm);

  vtkNew<vtkTransform> transform;
  transform->Identity();
  if (orientation == vtkMRMLU70ImageRegistrationNode::OrientationVertical)
  {
    // translate to A
    transform->Translate(0, -isocenterToImagerMm, 0);
    // scale L-R and S-I
    transform->Scale(scaleFactor, 1, scaleFactor);
  }
  else
  {
    // translate to R
    transform->Translate(-isocenterToImagerMm, 0, 0);
    // scale A-P and S-I
    transform->Scale(1, scaleFactor, scaleFactor);
  }


  vtkMRMLLinearTransformNode* scaleNode = this->GetActiveXrayScaleNode();
  if (!scaleNode)
  {
    return;
  }
  scaleNode->SetMatrixTransformToParent(transform->GetMatrix());
}

vtkMRMLScalarVolumeNode* vtkSlicerU70ImageRegistrationLogic::GetOrCreateImageWithGradientFilter(vtkMRMLScalarVolumeNode* inputVolume)
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    return nullptr;
  }

  std::string outputName = std::string(inputVolume->GetName()) + "_Gradient";

  vtkMRMLScalarVolumeNode* outputVolume = vtkMRMLScalarVolumeNode::SafeDownCast(scene->GetFirstNodeByName(outputName.c_str()));

  if (outputVolume)
  {
    return outputVolume; // Already computed
  }

  vtkImageData* inputData = inputVolume->GetImageData();

  // Compute gradient

  vtkNew<vtkImageGradient> gradient;
  gradient->SetInputData(inputData);
  gradient->SetDimensionality(2);

  // Compute magnitude

  vtkNew<vtkImageMagnitude> magnitude;
  magnitude->SetInputConnection(gradient->GetOutputPort());
  magnitude->Update();

  // Create new Scalar Volume Node
  
  outputVolume = vtkMRMLScalarVolumeNode::SafeDownCast(scene->AddNewNodeByClass("vtkMRMLScalarVolumeNode", outputName));
  outputVolume->CopyOrientation(inputVolume);
  outputVolume->SetAndObserveImageData(magnitude->GetOutput());
  outputVolume->SetAndObserveTransformNodeID(inputVolume->GetTransformNodeID());
  return outputVolume;
}