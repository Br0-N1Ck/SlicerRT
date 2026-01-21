/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes
#include <QDebug>

// CarmXrayBeam Widgets includes
#include "qSlicerCarmXrayBeamWidget.h"
#include "ui_qSlicerCarmXrayBeamWidget.h"

#include <qSlicerLayoutManager.h>
#include <qSlicerApplication.h>
#include <qMRMLSliceWidget.h>
#include <qSlicerSubjectHierarchyFolderPlugin.h>
#include <qSlicerSubjectHierarchyPluginHandler.h>
#include <qMRMLThreeDWidget.h>
#include <qMRMLThreeDView.h>

// MRML includes
#include <vtkMRMLChannel26GeometryNode.h>
#include <vtkMRMLDrrImageComputationNode.h>

#include <vtkMRMLRTPlanNode.h>
#include <vtkMRMLRTChannel26Cabin3BeamNode.h>

#include <vtkMRMLScalarVolumeNode.h>


#include <vtkMRMLScene.h>
#include <vtkMRMLLayoutNode.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLLinearTransformNode.h>

#include <vtkMRMLMarkupsFiducialNode.h>

// Beams inlcudes
#include <vtkMRMLRTBeamNode.h>

// Logic includes
#include <vtkSlicerPatientPositioningLogic.h>
#include <vtkSlicerDrrImageComputationLogic.h>

#include <vtkMRMLSliceLogic.h>

// VTK includes
#include <vtkVector.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>

#include <vtkTimerLog.h>
#include <vtkImageCast.h>

// ITK includes
#include <itkResampleImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkFlipImageFilter.h>


#include <itkEuler3DTransform.h>
#include <itkNormalizedCorrelationTwoImageToOneImageMetric.h>

// This is an intensity based registration algorithm so ray casting is
// used to project the 3D volume onto pixels in the target 2D image.
#include <itkSiddonJacobsRayCastInterpolateImageFunction.h>

#include <itkTwoProjectionImageRegistrationMethod.h>
#include <itkPowellOptimizer.h>
#include <itkNormalizedCorrelationImageToImageMetric.h>
//#include <itkRigid2D3DTransform.h>

#include <itkVTKImageToImageFilter.h>

//-----------------------------------------------------------------------------
class qSlicerCarmXrayBeamWidgetPrivate : public Ui_qSlicerCarmXrayBeamWidget
{
  Q_DECLARE_PUBLIC(qSlicerCarmXrayBeamWidget);
protected:
  qSlicerCarmXrayBeamWidget* const q_ptr;

public:
  qSlicerCarmXrayBeamWidgetPrivate(
    qSlicerCarmXrayBeamWidget& object);
  virtual void setupUi(qSlicerCarmXrayBeamWidget*);
  vtkMRMLPatientPositioningNode::CarmProjectionOrientation getCurrentOrientation();
  bool getTransformSlidersPosition(double pos[3]);
  vtkMRMLLinearTransformNode* getRegistrationLinearTransformNode();
  vtkLinearTransform* getRegistrationTransformToParent();
  void setNewPosition(double pos[3]);

  void init();

  vtkWeakPointer< vtkMRMLPatientPositioningNode > ParameterNode;
  vtkWeakPointer< vtkMRMLChannel26GeometryNode > Channel26GeoNode;
  vtkWeakPointer< vtkSlicerPatientPositioningLogic > PatientPositioningLogic;
};

// --------------------------------------------------------------------------
qSlicerCarmXrayBeamWidgetPrivate::qSlicerCarmXrayBeamWidgetPrivate(
  qSlicerCarmXrayBeamWidget& object)
  : q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidgetPrivate
::setupUi(qSlicerCarmXrayBeamWidget* widget)
{
  this->Ui_qSlicerCarmXrayBeamWidget::setupUi(widget);
}

// --------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidgetPrivate::init()
{
  Q_Q(qSlicerCarmXrayBeamWidget);

  QObject::connect( this->PushButton_ComputeCarmXrayDrr, SIGNAL(clicked()), q, SLOT(onComputeDrrClicked()));
  QObject::connect( this->MRMLNodeComboBox_DrrImageNode, SIGNAL(currentNodeChanged(vtkMRMLNode*)), 
    q, SLOT(onDrrImageNodeChanged(vtkMRMLNode*)));
  QObject::connect( this->MRMLNodeComboBox_CarmXrayImageNode, SIGNAL(currentNodeChanged(vtkMRMLNode*)), 
    q, SLOT(onCarmXrayImageNodeChanged(vtkMRMLNode*)));
  QObject::connect( this->CheckBox_ShowRtImageView, SIGNAL(toggled(bool)), q, SLOT(onSetImagesToSliceViewToggled(bool)));
  
  QObject::connect( this->MRMLTransformSliders_RegistrationTranslate, SIGNAL(valuesChanged()),
    q, SLOT(onTranslateSlidersValuesChanged()));
  QObject::connect( this->MRMLTransformSliders_RegistrationTranslate, SIGNAL(rangeChanged()),
    q, SLOT(onTranslateSlidersRangeChanged()));

  QObject::connect( this->PushButton_TransformCarmRawImage, SIGNAL(clicked()), q, SLOT(onTransformCarmRawImageClicked()));
  QObject::connect( this->PushButton_Up, SIGNAL(clicked()), q, SLOT(onMoveUpClicked()));
  QObject::connect( this->PushButton_Down, SIGNAL(clicked()), q, SLOT(onMoveDownClicked()));
  QObject::connect( this->PushButton_Left, SIGNAL(clicked()), q, SLOT(onMoveLeftClicked()));
  QObject::connect( this->PushButton_Right, SIGNAL(clicked()), q, SLOT(onMoveRightClicked()));

  QObject::connect(this->PushButton_IsocenterDiffUpdate, SIGNAL(clicked()), q, SLOT(onIsocenterDiffUpdateClicked()));

  QObject::connect(this->PushButton_ItkRegister, SIGNAL(clicked()), q, SLOT(onItkRegisterClicked()));

  q->onTranslateSlidersRangeChanged();
}

// --------------------------------------------------------------------------
bool qSlicerCarmXrayBeamWidgetPrivate::getTransformSlidersPosition(double pos[3])
{
  Q_Q(qSlicerCarmXrayBeamWidget);
  vtkMRMLLinearTransformNode* transNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->MRMLTransformSliders_RegistrationTranslate->mrmlTransformNode());
  if (!transNode)
  {
    return false;
  }
  vtkNew< vtkMatrix4x4 > mat;
  transNode->GetMatrixTransformToParent(mat);
  pos[0] = mat->GetElement(0, 3);
  pos[1] = mat->GetElement(1, 3);
  pos[2] = mat->GetElement(2, 3);
  return true;
}

// --------------------------------------------------------------------------
vtkMRMLLinearTransformNode* qSlicerCarmXrayBeamWidgetPrivate::getRegistrationLinearTransformNode()
{
  Q_Q(qSlicerCarmXrayBeamWidget);
  vtkMRMLLinearTransformNode* transNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->MRMLTransformSliders_RegistrationTranslate->mrmlTransformNode());
  if (!transNode)
  {
    return nullptr;
  }
  return transNode;
}

// --------------------------------------------------------------------------
vtkLinearTransform* qSlicerCarmXrayBeamWidgetPrivate::getRegistrationTransformToParent()
{
  Q_Q(qSlicerCarmXrayBeamWidget);
  vtkMRMLLinearTransformNode* transNode = this->getRegistrationLinearTransformNode();
  if (!transNode)
  {
    return nullptr;
  }
  return vtkLinearTransform::SafeDownCast(transNode->GetTransformToParent());
}

// --------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidgetPrivate::setNewPosition(double pos[3])
{
  Q_Q(qSlicerCarmXrayBeamWidget);
  vtkMRMLLinearTransformNode* transNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->MRMLTransformSliders_RegistrationTranslate->mrmlTransformNode());
  if (!transNode)
  {
    return;
  }
  vtkNew< vtkMatrix4x4 > mat;
  transNode->GetMatrixTransformToParent(mat);
  mat->SetElement(0, 3, pos[0]);
  mat->SetElement(1, 3, pos[1]);
  mat->SetElement(2, 3, pos[2]);
  transNode->SetMatrixTransformToParent(mat);
  this->MRMLCoordinatesWidget_TranslatePosition->setCoordinates(pos);
}

// --------------------------------------------------------------------------
vtkMRMLPatientPositioningNode::CarmProjectionOrientation qSlicerCarmXrayBeamWidgetPrivate::getCurrentOrientation()
{
  Q_Q(qSlicerCarmXrayBeamWidget);
  vtkMRMLPatientPositioningNode::CarmProjectionOrientation projType =
    vtkMRMLPatientPositioningNode::CarmProjectionOrientation_Last;

  if (this->RadioButton_OrientationVertical->isChecked())
  {
    projType = vtkMRMLPatientPositioningNode::ORIENTATION_VERTICAL;
  }
  else if (this->RadioButton_OrientationHorizontal->isChecked())
  {
    projType = vtkMRMLPatientPositioningNode::ORIENTATION_HORIZONTAL;
  }
  else if (this->RadioButton_OrientationAngle->isChecked())
  {
    projType = vtkMRMLPatientPositioningNode::ORIENTATION_ANGLE;
  }
  return projType;
}

//-----------------------------------------------------------------------------
// qSlicerCarmXrayBeamWidget methods

//-----------------------------------------------------------------------------
qSlicerCarmXrayBeamWidget::qSlicerCarmXrayBeamWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerCarmXrayBeamWidgetPrivate(*this) )
{
  Q_D(qSlicerCarmXrayBeamWidget);
  d->setupUi(this);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerCarmXrayBeamWidget::~qSlicerCarmXrayBeamWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidget::setParameterNode(vtkMRMLNode* node)
{
  Q_D(qSlicerCarmXrayBeamWidget);

  vtkMRMLPatientPositioningNode* parameterNode = vtkMRMLPatientPositioningNode::SafeDownCast(node);
  // Each time the node is modified, the UI widgets are updated
  qvtkReconnect( d->ParameterNode, parameterNode, vtkCommand::ModifiedEvent, 
    this, SLOT( updateWidgetFromMRML() ) );
  qvtkReconnect( d->Channel26GeoNode, parameterNode->GetChannel26GeometryNode(), vtkCommand::ModifiedEvent, 
    this, SLOT( updateWidgetFromMRML() ) );

  d->ParameterNode = parameterNode;
  if (parameterNode)
  {
    d->Channel26GeoNode = parameterNode->GetChannel26GeometryNode();
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidget::setPatientPositioningLogic(vtkSlicerPatientPositioningLogic* logic)
{
  Q_D(qSlicerCarmXrayBeamWidget);
  d->PatientPositioningLogic = logic;
}

//-----------------------------------------------------------------------------
/*
void qSlicerCarmXrayBeamWidget::setDrrImageComputationNode(vtkMRMLDrrImageComputationNode* node)
{
  Q_D(qSlicerCarmXrayBeamWidget);
  if (!node)
  {
    return;
  }
  d->MRMLNodeComboBox_DrrNode->setCurrentNode(node);
  this->updateWidgetFromMRML();
}
*/
//-----------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidget::onComputeDrrClicked()
{
  qWarning() << Q_FUNC_INFO << "Tryin' to compute DRR";
  Q_D(qSlicerCarmXrayBeamWidget);
  if (!d->PatientPositioningLogic)
  {
    qWarning() << Q_FUNC_INFO << "PatientPositioning logic is invalid";
    return;
  }

  if (!d->ParameterNode)
  {
    qWarning() << Q_FUNC_INFO << "Parameter node is invalid";
    return;
  }

  vtkMRMLScalarVolumeNode* ctInputVolumeNode = nullptr;
  vtkMRMLRTBeamNode* beamNode = d->ParameterNode->GetBeamNode(); // ion beam node
  if (beamNode)
  {
    vtkMRMLRTPlanNode* planNode = beamNode->GetParentPlanNode();
    if (planNode)
    {
        ctInputVolumeNode = planNode->GetReferenceVolumeNode(); // TODO: Something's wrong here
      if (ctInputVolumeNode)
      {
          qWarning() << "ctInputVolumeNode GET";
      }
      else
      {
          qWarning() << "!ctInputVolumeNode";
      }
    }
  }
//  vtkMRMLScene* scene = d->PatientPositioningLogic->GetMRMLScene();
  vtkMRMLNode* node = d->MRMLNodeComboBox_DrrNode->currentNode();
  vtkMRMLDrrImageComputationNode* drrNode = nullptr;
  if (!node)
  {
    qCritical() << Q_FUNC_INFO << "DRR image computation node is invalid";
    return;
  }
  if (node)
  {
    drrNode = vtkMRMLDrrImageComputationNode::SafeDownCast(node);
    if (drrNode && ctInputVolumeNode)
    {
      vtkMRMLRTBeamNode* carmXrayBeamNode = d->ParameterNode->GetCarmXrayBeamNode();
      drrNode->SetAndObserveBeamNode(carmXrayBeamNode);
    }
  }

  if (drrNode && ctInputVolumeNode)
  {
    // compute DRR image here
    vtkSlicerDrrImageComputationLogic* drrLogic = d->PatientPositioningLogic->GetDrrImageComputationLogic();
    drrLogic->UpdateMarkupsNodes(drrNode);
    drrLogic->UpdateNormalAndVupVectors(drrNode);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    qWarning() << "Computing DRR";

    vtkMRMLScalarVolumeNode* drrImageNode = drrLogic->ComputePlastimatchDRR( drrNode, ctInputVolumeNode, true);
    if (drrImageNode)
    {
      // node is OK
        qWarning() << "Computed DRR";
      QApplication::restoreOverrideCursor();
      return;
    }
    else 
    {
        qWarning() << Q_FUNC_INFO << "Didn't compute DRR";
    }
    QApplication::restoreOverrideCursor();
  }
  else
  {
      qWarning() << Q_FUNC_INFO << "!drrNode or !ctInputVolumeNode";
  }
}

//-----------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerCarmXrayBeamWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  if (!d->PatientPositioningLogic)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid PatientPositioning logic";
    return;
  }
  vtkMRMLDrrImageComputationNode* drrNode = d->ParameterNode->GetDrrComputationNode();
  if (drrNode)
  {
    d->MRMLNodeComboBox_DrrNode->setCurrentNode(drrNode);
    int res[2] = { -1, -1 };
    drrNode->GetImagerResolution(res);
    double spacing[2] = { -0.1, -0.1 };
    drrNode->GetImagerSpacing(spacing);
    double sad = drrNode->GetBeamNode()->GetSAD();
    double isoImagerDistance = drrNode->GetIsocenterImagerDistance();
    d->Label_DrrResolution->setText(tr("%1 x %2").arg(res[0]).arg(res[1]));
    d->Label_DrrSpacing->setText(tr("%1 x %2").arg(spacing[0]).arg(spacing[1]));
    d->Label_DrrSAD->setText(tr("%1").arg(sad));
    d->Label_DrrSID->setText(tr("%1").arg(sad + isoImagerDistance));
  }
  else
  {
    d->Label_DrrResolution->setText("");
    d->Label_DrrSpacing->setText("");
    d->Label_DrrSAD->setText("");
    d->Label_DrrSID->setText("");
  }
  vtkMRMLPatientPositioningNode::CarmProjectionOrientation proj = d->getCurrentOrientation();
  vtkTransform* registrationTransform = d->ParameterNode->GetRegistrationTransform(proj);
  vtkMRMLLinearTransformNode* transformNode = d->PatientPositioningLogic->GetDefaultRegistrationTransformNode();
  if (transformNode)
  {
    transformNode->SetAndObserveTransformToParent(registrationTransform);
    d->MRMLTransformSliders_RegistrationTranslate->setMRMLTransformNode(transformNode);
    d->MRMLMatrixWidget_TransformMatrix->setMRMLTransformNode(transformNode);
  }
}

//-----------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidget::onSetImagesToSliceViewToggled(bool maximize)
{
  Q_D(qSlicerCarmXrayBeamWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }

  vtkMRMLPatientPositioningNode::CarmProjectionOrientation projType = d->getCurrentOrientation();

  vtkMRMLScalarVolumeNode* carmDrrImageNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    d->MRMLNodeComboBox_DrrImageNode->currentNode()); // moved image
  vtkMRMLScalarVolumeNode* carmXrayImageNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    d->MRMLNodeComboBox_CarmXrayImageNode->currentNode()); // static image

  // Manage "Red" slice for DRR
  qSlicerApplication* slicerApplication = qSlicerApplication::application();
  qMRMLSliceWidget* sliceWidget = slicerApplication->layoutManager()->sliceWidget("Red");

  if (!sliceWidget)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid slice widget";
    return;
  }

  if (!carmDrrImageNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid DRR image node";
    return;
  }

  bool isMaximized = false;
  bool canBeMaximized = false;
  vtkMRMLSliceNode* sliceNode = sliceWidget->mrmlSliceNode();
  vtkMRMLLayoutNode* layoutNode = sliceNode->GetMaximizedState(isMaximized, canBeMaximized);

  // When toggled, set DRR image as background and maximize slice
  if (maximize && layoutNode)
  {
    vtkMRMLSliceLogic* sliceLogic = sliceWidget->sliceLogic();
    sliceLogic->GetSliceCompositeNode()->SetBackgroundVolumeID(carmDrrImageNode->GetID());
    sliceLogic->RotateSliceToLowestVolumeAxes(); // Reformat

    sliceLogic->FitSliceToAll();
    sliceNode->UpdateMatrices();
    if (canBeMaximized && !isMaximized)
    {
      layoutNode->AddMaximizedViewNode(sliceNode);
    }
  }
  else if (!maximize && layoutNode && isMaximized) // When released, restore view layout
  {
    layoutNode->RemoveMaximizedViewNode(sliceNode);
  }

  emit registrationRtImagePairChanged(projType, carmXrayImageNode, carmDrrImageNode);
}

//-----------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidget::onDrrImageNodeChanged(vtkMRMLNode* drrNode)
{
  Q_D(qSlicerCarmXrayBeamWidget);

  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }
  vtkMRMLPatientPositioningNode::CarmProjectionOrientation proj = d->getCurrentOrientation();
  vtkMRMLScalarVolumeNode* drrImageNode = vtkMRMLScalarVolumeNode::SafeDownCast(drrNode);
  if (drrImageNode)
  {
    qDebug() << Q_FUNC_INFO << "C-arm x-ray image is valid: " << drrImageNode->GetName();
    d->ParameterNode->SetRegistrationImages(proj, nullptr, drrImageNode);
  }
}

//-----------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidget::onCarmXrayImageNodeChanged(vtkMRMLNode* xrayNode)
{
  Q_D(qSlicerCarmXrayBeamWidget);
  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }
  vtkMRMLPatientPositioningNode::CarmProjectionOrientation proj = d->getCurrentOrientation();

  vtkTransform* regTransform = d->ParameterNode->GetRegistrationTransform(proj);
  double regOffset[3] = {};
  if (regTransform)
  {
    regTransform->GetPosition(regOffset);
    qDebug() << Q_FUNC_INFO << "SetupGeometry: Reg offset: " << regOffset[0] << ' ' << regOffset[1] << ' ' << regOffset[2];
  }

  vtkMRMLScalarVolumeNode* xrayImageNode = vtkMRMLScalarVolumeNode::SafeDownCast(xrayNode);
  if (xrayImageNode)
  {
    qDebug() << Q_FUNC_INFO << "C-arm x-ray image is valid: " << xrayImageNode->GetName();
    d->ParameterNode->SetRegistrationImages(proj, xrayImageNode, nullptr);
  }
}

//-----------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidget::onTransformCarmRawImageClicked()
{
  Q_D(qSlicerCarmXrayBeamWidget);
  if (!d->ParameterNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
    return;
  }
  vtkMRMLNode* node = d->MRMLNodeComboBox_CarmRawVolume->currentNode();
  vtkMRMLScalarVolumeNode* imageNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  if (!imageNode)
  {
    return;
  }
  d->ParameterNode->GetDrrComputationNode()->SetAndObserveRtImageVolumeNode(imageNode);
  if (d->PatientPositioningLogic->ApplyCarmXrayDetectorTransformToXrayImage(d->ParameterNode, imageNode))
  {
    d->MRMLNodeComboBox_CarmXrayImageNode->setCurrentNode(imageNode);
  }
}

//-----------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidget::onMoveUpClicked()
{
  Q_D(qSlicerCarmXrayBeamWidget);
  double pos[3] = {};
  if (d->getTransformSlidersPosition(pos))
  {
    pos[1] += 0.05;
    d->setNewPosition(pos);
  }
}

//-----------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidget::onMoveDownClicked()
{
  Q_D(qSlicerCarmXrayBeamWidget);
  double pos[3] = {};
  if (d->getTransformSlidersPosition(pos))
  {
    pos[1] -= 0.05;
    d->setNewPosition(pos);
  }
}

//-----------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidget::onMoveLeftClicked()
{
  Q_D(qSlicerCarmXrayBeamWidget);
  double pos[3] = {};
  if (d->getTransformSlidersPosition(pos))
  {
    pos[0] -= 0.05;
    d->setNewPosition(pos);
  }
}

//-----------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidget::onMoveRightClicked()
{
  Q_D(qSlicerCarmXrayBeamWidget);
  double pos[3] = {};
  if (d->getTransformSlidersPosition(pos))
  {
    pos[0] += 0.05;
    d->setNewPosition(pos);
  }
}

//-----------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidget::onTranslateSlidersValuesChanged()
{
  Q_D(qSlicerCarmXrayBeamWidget);
  double pos[3] = {};
  if (d->getTransformSlidersPosition(pos))
  {
    d->MRMLCoordinatesWidget_TranslatePosition->setCoordinates(pos);
  }
}

//-----------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidget::onTranslateSlidersRangeChanged()
{
  Q_D(qSlicerCarmXrayBeamWidget);
  double min = d->MRMLCoordinatesWidget_TranslatePosition->minimum();
  double max = d->MRMLCoordinatesWidget_TranslatePosition->maximum();
  qDebug() << Q_FUNC_INFO << "Min: " << min << ", max: " << max;
  d->MRMLMatrixWidget_TransformMatrix->setRange(min, max);
  d->MRMLCoordinatesWidget_TranslatePosition->setRange(min, max);
}

//-----------------------------------------------------------------------------
void qSlicerCarmXrayBeamWidget::onIsocenterDiffUpdateClicked()
{
    Q_D(qSlicerCarmXrayBeamWidget);
    if (!d->ParameterNode)
    {
        qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
        return;
    }

    // Get patient isocenter
    double isocenterPatient[4] = { 0, 0, 0, 1 };
    vtkMRMLRTBeamNode* beamNode = d->ParameterNode->GetBeamNode(); // ion beam node

    if (!beamNode)
    {
        qCritical() << Q_FUNC_INFO << ": Invalid beam node";
        return;
    }

    vtkMRMLRTPlanNode* planNode = beamNode->GetParentPlanNode();
    if (!planNode)
    {
        qCritical() << Q_FUNC_INFO << ": Invalid plan node";
        return; 
    }
    planNode->GetIsocenterPosition(isocenterPatient);

    // Get fixed isocenter

    double isocenterFixed[4] = { 0, 0, 0, 1 };
    vtkMRMLMarkupsFiducialNode* isocenterFixedNode= d->ParameterNode->GetCabin3IsocenterFiducialNode();
    if (!isocenterFixedNode)
    {
        qCritical() << Q_FUNC_INFO << ": Invalid or empty isocenter fixed node";
        return;
    }
    isocenterFixedNode->GetNthControlPointPositionWorld(0, isocenterFixed);

    qDebug() << "isocenterPatient:" << isocenterPatient[0] << isocenterPatient[1] << isocenterPatient[2];
    qDebug() << "isocenterFixed:" << isocenterFixed[0] << isocenterFixed[1] << isocenterFixed[2];
        
    // Apply transforms to isocenters

    double isocenterPatient_Fixed[4] = { 0 };
    double isocenterPatient_TableTop[4] = { 0 };
    double isocenterPatient_Flange[4] = { 0 };

    double isocenterFixed_Fixed[4] = { 0 };
    double isocenterFixed_TableTop[4] = { 0 };
    double isocenterFixed_Flange[4] = { 0 };

    vtkNew<vtkMatrix4x4> transformFixed, transformTableTop, transformFlange;

    d->PatientPositioningLogic->GetChannel26RobotsTransformLogic()->GetFixedReferenceTransform()->GetMatrixTransformFromParent(transformFixed);
    d->PatientPositioningLogic->GetChannel26RobotsTransformLogic()->GetTableTopTransform()->GetMatrixTransformFromParent(transformTableTop);
    d->PatientPositioningLogic->GetChannel26RobotsTransformLogic()->GetTableFlangeTransform()->GetMatrixTransformFromParent(transformFlange);

    transformFixed->MultiplyPoint(isocenterPatient, isocenterPatient_Fixed);
    transformTableTop->MultiplyPoint(isocenterPatient, isocenterPatient_TableTop);
    transformFlange->MultiplyPoint(isocenterPatient, isocenterPatient_Flange);

    transformFixed->MultiplyPoint(isocenterFixed, isocenterFixed_Fixed);
    transformTableTop->MultiplyPoint(isocenterFixed, isocenterFixed_TableTop);
    transformFlange->MultiplyPoint(isocenterFixed, isocenterFixed_Flange);


    // Calculate difference
    double isocenterDiff_Fixed[4] = {
    isocenterPatient_Fixed[0] - isocenterFixed_Fixed[0],
    isocenterPatient_Fixed[1] - isocenterFixed_Fixed[1],
    isocenterPatient_Fixed[2] - isocenterFixed_Fixed[2],
    1
    };

    double isocenterDiff_TableTop[4] = {
    isocenterPatient_TableTop[0] - isocenterFixed_TableTop[0],
    isocenterPatient_TableTop[1] - isocenterFixed_TableTop[1],
    isocenterPatient_TableTop[2] - isocenterFixed_TableTop[2],
    1
    };

    double isocenterDiff_Flange[4] = {
    isocenterPatient_Flange[0] - isocenterFixed_Flange[0],
    isocenterPatient_Flange[1] - isocenterFixed_Flange[1],
    isocenterPatient_Flange[2] - isocenterFixed_Flange[2],
    1
    };

    // Enter into widgets

    d->MRMLCoordinatesWidget_IsocenterDiff_Fixed->setCoordinates(isocenterDiff_Fixed);
    d->MRMLCoordinatesWidget_IsocenterDiff_TableTop->setCoordinates(isocenterDiff_TableTop);
    d->MRMLCoordinatesWidget_IsocenterDiff_Flange->setCoordinates(isocenterDiff_Flange);

}

void qSlicerCarmXrayBeamWidget::onItkRegisterClicked()
{
    Q_D(qSlicerCarmXrayBeamWidget);
    if (!d->ParameterNode)
    {
        qCritical() << Q_FUNC_INFO << ": Invalid parameter node";
        return;
    }
    vtkMRMLScalarVolumeNode* ctNode = vtkMRMLScalarVolumeNode::SafeDownCast(d->MRMLNodeComboBox_CTNode->currentNode());
    if (!ctNode)
    {
        qCritical() << Q_FUNC_INFO << ": Invalid CT node";
        return;
    }
    vtkMRMLScalarVolumeNode* drrImageNode_1 = vtkMRMLScalarVolumeNode::SafeDownCast(d->MRMLNodeComboBox_DrrImageNode_1->currentNode());
    if (!drrImageNode_1)
    {
        qCritical() << Q_FUNC_INFO << ": Invalid DRR image 1 node";
        return;
    }
    vtkMRMLScalarVolumeNode* drrImageNode_2 = vtkMRMLScalarVolumeNode::SafeDownCast(d->MRMLNodeComboBox_DrrImageNode_2->currentNode());
    if (!drrImageNode_2)
    {
        qCritical() << Q_FUNC_INFO << ": Invalid DRR image 2 node";
        return;
    }


    // Convert to ITK Image< PixelType, Dimension >

    constexpr unsigned int Dimension = 3;
    using InternalPixelType = float;
    using PixelType3D = short;

    using ImageType3D = itk::Image<PixelType3D, Dimension>;

    using OutputPixelType = unsigned char;
    using OutputImageType = itk::Image<OutputPixelType, Dimension>;

    using InternalImageType = itk::Image<InternalPixelType, Dimension>;

    // CT (Moving) Image

    double spacing[3];
    
    vtkImageData* ctVtkImage = ctNode->GetImageData();

    using Moving_VTKToITKFilterType = itk::VTKImageToImageFilter<ImageType3D>;
    auto moving_vtkToItkFilter = Moving_VTKToITKFilterType::New();

    vtkNew<vtkImageCast> vtkCaster3D;
    vtkCaster3D->SetInputData(ctVtkImage);
    vtkCaster3D->SetOutputScalarTypeToShort();
    vtkCaster3D->Update();

    vtkImageData* floatVtkImage = vtkCaster3D->GetOutput();

    moving_vtkToItkFilter->SetInput(floatVtkImage);
    moving_vtkToItkFilter->Update();

    ImageType3D::Pointer movingItkImage = moving_vtkToItkFilter->GetOutput();


    
    ctNode->GetSpacing(spacing);
    movingItkImage->SetSpacing(spacing);

    // To simply Siddon-Jacob's fast ray-tracing algorithm, we force the origin of the CT image
    // to be (0,0,0). Because we align the CT isocenter with the central axis, the projection
    // geometry is fully defined. The origin of the CT image becomes irrelavent.
    ImageType3D::PointType image3DOrigin;
    image3DOrigin[0] = 0.0;
    image3DOrigin[1] = 0.0;
    image3DOrigin[2] = 0.0;
    movingItkImage->SetOrigin(image3DOrigin);



    // DRR (Fixed) Images
    vtkImageData* drrVtkImage_1 = drrImageNode_1->GetImageData();
    // Cast to float
    vtkNew<vtkImageCast> drrCaster1;
    drrCaster1->SetInputData(drrVtkImage_1);
    drrCaster1->SetOutputScalarTypeToFloat();
    drrCaster1->Update();
    
    vtkImageData* drrVtkImage_2 = drrImageNode_2->GetImageData();
    vtkNew<vtkImageCast> drrCaster2;
    drrCaster2->SetInputData(drrVtkImage_2);
    drrCaster2->SetOutputScalarTypeToFloat();
    drrCaster2->Update();

    using Fixed_VTKToITKFilterType = itk::VTKImageToImageFilter<InternalImageType>;

    auto fixed_vtkToItkFilter_1 = Fixed_VTKToITKFilterType::New();
    auto fixed_vtkToItkFilter_2 = Fixed_VTKToITKFilterType::New();
    fixed_vtkToItkFilter_1->SetInput(drrCaster1->GetOutput());
    fixed_vtkToItkFilter_1->Update();
    InternalImageType::Pointer fixedItkImage_1 = fixed_vtkToItkFilter_1->GetOutput();
    fixed_vtkToItkFilter_2->SetInput(drrCaster2->GetOutput());
    fixed_vtkToItkFilter_2->Update();
    InternalImageType::Pointer fixedItkImage_2 = fixed_vtkToItkFilter_2->GetOutput();

    drrImageNode_1->GetSpacing(spacing);
    fixedItkImage_1->SetSpacing(spacing);

    drrImageNode_2->GetSpacing(spacing);
    fixedItkImage_2->SetSpacing(spacing);

    // The following lines define each of the components used in the
    // registration: The transform, optimizer, metric, interpolator and
    // the registration method itself.

    using TransformType = itk::Euler3DTransform<double>;

    using OptimizerType = itk::PowellOptimizer;

    // using MetricType = itk::GradientDifferenceTwoImageToOneImageMetric<
    using MetricType = itk::NormalizedCorrelationTwoImageToOneImageMetric<InternalImageType, InternalImageType>;

    using InterpolatorType = itk::SiddonJacobsRayCastInterpolateImageFunction<InternalImageType, double>;


    using RegistrationType = itk::TwoProjectionImageRegistrationMethod<InternalImageType, InternalImageType>;


    // Each of the registration components are instantiated in the
    // usual way...

    MetricType::Pointer       metric = MetricType::New();
    TransformType::Pointer    transform = TransformType::New();
    OptimizerType::Pointer    optimizer = OptimizerType::New();
    InterpolatorType::Pointer interpolator1 = InterpolatorType::New();
    InterpolatorType::Pointer interpolator2 = InterpolatorType::New();
    RegistrationType::Pointer registration = RegistrationType::New();

    metric->ComputeGradientOff();
    metric->SetSubtractMean(true);

    // and passed to the registration method:

    registration->SetMetric(metric);
    registration->SetOptimizer(optimizer);
    registration->SetTransform(transform);
    registration->SetInterpolator1(interpolator1);
    registration->SetInterpolator2(interpolator2);

    // The input 2D images were loaded as 3D images. They were considered
    // as a single slice from a 3D volume. By default, images stored on the
    // disk are treated as if they have RAI orientation. After view point
    // transformation, the order of 2D image pixel reading is equivalent to
    // from inferior to superior. This is contradictory to the traditional
    // 2D x-ray image storage, in which a typical 2D image reader reads and
    // writes images from superior to inferior. Thus the loaded 2D DICOM
    // images should be flipped in y-direction. This was done by using a.
    // FilpImageFilter.
    using FlipFilterType = itk::FlipImageFilter<InternalImageType>;
    FlipFilterType::Pointer flipFilter1 = FlipFilterType::New();
    FlipFilterType::Pointer flipFilter2 = FlipFilterType::New();

    using FlipAxesArrayType = FlipFilterType::FlipAxesArrayType;
    FlipAxesArrayType flipArray;
    flipArray[0] = false;
    flipArray[1] = true;
    flipArray[2] = false;

    flipFilter1->SetFlipAxes(flipArray);
    flipFilter2->SetFlipAxes(flipArray);

    flipFilter1->SetInput(fixedItkImage_1);
    flipFilter2->SetInput(fixedItkImage_2);

    //  The 3D CT dataset is casted to the internal image type using
    //  {CastImageFilters}.

    using CastFilterType3D = itk::CastImageFilter<ImageType3D, InternalImageType>;

    CastFilterType3D::Pointer caster3D = CastFilterType3D::New();
    caster3D->SetInput(movingItkImage);
    caster3D->Update();
  
    registration->SetFixedImage1(flipFilter1->GetOutput());
    registration->SetFixedImage2(flipFilter2->GetOutput());
    registration->SetMovingImage(caster3D->GetOutput());

    metric->DebugOn();
    transform->DebugOn();
    optimizer->DebugOn();
    interpolator1->DebugOn();
    interpolator2->DebugOn();
    registration->DebugOn();



    // Initialise transform
    transform->SetComputeZYX(true);

    TransformType::ParametersType initialParameters(
      transform->GetNumberOfParameters());
    initialParameters.Fill(0.0);
    
    transform->SetParameters(initialParameters);
    //transform->SetFixedParameters(transform->GetFixedParameters());

    // Set transform center
    InternalImageType::PointType center;
    auto region = movingItkImage->GetLargestPossibleRegion();
    auto size   = region.GetSize();

    for (int i = 0; i < 3; ++i)
    {
      center[i] =
        movingItkImage->GetOrigin()[i] +
        movingItkImage->GetSpacing()[i] * size[i] / 2.0;
    }

    transform->SetCenter(center);

    std::cout << "Transform: " << transform << std::endl;


    // Initialize the ray cast interpolator
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // The ray cast interpolator is used to project the 3D volume. It
    // does this by casting rays from the (transformed) focal point to
    // each (transformed) pixel coordinate in the 2D image.
    //
    // In addition a threshold may be specified to ensure that only
    // intensities greater than a given value contribute to the
    // projected volume. This can be used, for instance, to remove soft
    // tissue from projections of CT data and force the registration
    // to find a match which aligns bony structures in the images.

    // constant for converting degrees to radians
    const double dtr = (atan(1.0) * 4.0) / 180.0;
    const double projAngle1 = 0;
    const double projAngle2 = 90;
    const double scd = 1000.;
    const double threshold = 0;

    // 2D Image 1
    interpolator1->SetProjectionAngle(dtr * projAngle1);
    interpolator1->SetFocalPointToIsocenterDistance(scd);
    interpolator1->SetThreshold(threshold);
    interpolator1->SetTransform(transform);

    interpolator1->Initialize();

    // 2D Image 2
    interpolator2->SetProjectionAngle(dtr * projAngle2);
    interpolator2->SetFocalPointToIsocenterDistance(scd);
    interpolator2->SetThreshold(threshold);
    interpolator2->SetTransform(transform);

    interpolator2->Initialize();


    // Set the origin of the 2D image
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    double origin2D1[Dimension];
    double origin2D2[Dimension];

    // Note: Two 2D images may have different image sizes and pixel dimensions, although
    // scd are the same.

    const itk::Vector<double, 3> resolution2D1 = fixedItkImage_1->GetSpacing();
    const itk::Vector<double, 3> resolution2D2 = fixedItkImage_2->GetSpacing();

    using ImageRegionType2D = InternalImageType::RegionType;
    using SizeType2D = ImageRegionType2D::SizeType;

    ImageRegionType2D region2D1 = fixedItkImage_1->GetBufferedRegion();
    ImageRegionType2D region2D2 = fixedItkImage_2->GetBufferedRegion();
    SizeType2D        size2D1 = region2D1.GetSize();
    SizeType2D        size2D2 = region2D2.GetSize();

    double image1centerX = 0.0;
    double image1centerY = 0.0;
    double image2centerX = 0.0;
    double image2centerY = 0.0;

    // Central axis positions are not given by the user. Use the image centers
         // as the central axis position.
    image1centerX = ((double)size2D1[0] - 1.) / 2.;
    image1centerY = ((double)size2D1[1] - 1.) / 2.;
    image2centerX = ((double)size2D2[0] - 1.) / 2.;
    image2centerY = ((double)size2D2[1] - 1.) / 2.;


    // 2D Image 1
    origin2D1[0] = -resolution2D1[0] * image1centerX;
    origin2D1[1] = -resolution2D1[1] * image1centerY;
    origin2D1[2] = -scd;

    //rescaler2D1->GetOutput()->SetOrigin(origin2D1);
    fixedItkImage_1->SetOrigin(origin2D1);

    // 2D Image 2
    origin2D2[0] = -resolution2D2[0] * image2centerX;
    origin2D2[1] = -resolution2D2[1] * image2centerY;
    origin2D2[2] = -scd;

    //rescaler2D2->GetOutput()->SetOrigin(origin2D2);
    fixedItkImage_2->SetOrigin(origin2D2);

    registration->SetFixedImageRegion1(fixedItkImage_1->GetBufferedRegion());
    registration->SetFixedImageRegion2(fixedItkImage_2->GetBufferedRegion());

    qDebug() << "CT Image properties:";
    qDebug() << "  Size:" << size[0] << size[1] << size[2];
    qDebug() << "  Spacing:" << spacing[0] << spacing[1] << spacing[2];
    qDebug() << "  Origin:" << movingItkImage->GetOrigin()[0] 
            << movingItkImage->GetOrigin()[1] 
            << movingItkImage->GetOrigin()[2];

    qDebug() << "DRR Image 1 properties:";
    qDebug() << "  Size:" << size2D1[0] << size2D1[1];
    qDebug() << "  Spacing:" << resolution2D1[0] << resolution2D1[1];
    qDebug() << "  Origin:" << fixedItkImage_1->GetOrigin()[0]
            << fixedItkImage_1->GetOrigin()[1] 
            << fixedItkImage_1->GetOrigin()[2];
    qDebug() << "DRR Image 2 properties:";
    qDebug() << "  Size:" << size2D2[0] << size2D2[1];
    qDebug() << "  Spacing:" << resolution2D2[0] << resolution2D2[1];
    qDebug() << "  Origin:" << fixedItkImage_2->GetOrigin()[0]
            << fixedItkImage_2->GetOrigin()[1] 
            << fixedItkImage_2->GetOrigin()[2];


    // Set up the transform and start position
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // The registration start position is intialised using the
    // transformation parameters.

    registration->SetInitialTransformParameters(transform->GetParameters());

    // We wish to minimize the negative normalized correlation similarity measure.

    // optimizer->SetMaximize( true );  // for GradientDifferenceTwoImageToOneImageMetric
    optimizer->SetMaximize(false); // for NCC

    optimizer->SetMaximumIteration(10);
    optimizer->SetMaximumLineIteration(4); // for Powell's method
    optimizer->SetStepLength(4);
    optimizer->SetStepTolerance(0.02);
    optimizer->SetValueTolerance(0.001);

    // The optimizer weightings are set such that one degree equates to
    // one millimeter.

    itk::Optimizer::ScalesType weightings(transform->GetNumberOfParameters());

    weightings[0] = 1. / dtr;
    weightings[1] = 1. / dtr;
    weightings[2] = 1. / dtr;
    weightings[3] = 1.;
    weightings[4] = 1.;
    weightings[5] = 1.;

    optimizer->SetScales(weightings);

    optimizer->Print( std::cout );

    // Run registration
    try
    {
      QApplication::setOverrideCursor(Qt::WaitCursor);
      registration->StartRegistration();
      QApplication::restoreOverrideCursor();
    }
    catch (itk::ExceptionObject& err)
    {
      QApplication::restoreOverrideCursor();
      qCritical() << err.what();
      return;
    }
    // Print registration results
    using ParametersType = RegistrationType::ParametersType;
    ParametersType finalParameters = registration->GetLastTransformParameters();

    const double RotationAlongX = finalParameters[0] / dtr; // Convert radian to degree
    const double RotationAlongY = finalParameters[1] / dtr;
    const double RotationAlongZ = finalParameters[2] / dtr;
    const double TranslationAlongX = finalParameters[3];
    const double TranslationAlongY = finalParameters[4];
    const double TranslationAlongZ = finalParameters[5];

    const int numberOfIterations = optimizer->GetCurrentIteration();

    const double bestValue = optimizer->GetValue();

    qInfo() << "Result = ";
    qInfo() << " Rotation Along X = " << RotationAlongX << " deg";
    qInfo() << " Rotation Along Y = " << RotationAlongY << " deg";
    qInfo() << " Rotation Along Z = " << RotationAlongZ << " deg";
    qInfo() << " Translation X = " << TranslationAlongX << " mm";
    qInfo() << " Translation Y = " << TranslationAlongY << " mm";
    qInfo() << " Translation Z = " << TranslationAlongZ << " mm";
    qInfo() << " Number Of Iterations = " << numberOfIterations;
    qInfo() << " Metric value  = " << bestValue;

    // TODO: Create transform node from result

    // vtkNew<vtkMatrix4x4> vtkMatrix;

    // vtkNew<vtkMRMLLinearTransformNode> transformNode;
    // transformNode->SetName("RegistrationTransform");

    // vtkMRMLScene* scene = this->mrmlScene();
    // scene->AddNode(transformNode);
}
