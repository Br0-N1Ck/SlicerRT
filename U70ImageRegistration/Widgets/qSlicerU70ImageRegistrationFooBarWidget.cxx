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

// FooBar Widgets includes
#include "qSlicerU70ImageRegistrationFooBarWidget.h"
#include "ui_qSlicerU70ImageRegistrationFooBarWidget.h"

// Qt includes
#include <QDebug>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLLinearTransformNode.h>

// VTK includes
#include <vtkMatrix4x4.h>
#include <vtkLinearTransform.h>
#include <vtkTransform.h>
#include <vtkNew.h>

//-----------------------------------------------------------------------------
class qSlicerU70ImageRegistrationFooBarWidgetPrivate
  : public Ui_qSlicerU70ImageRegistrationFooBarWidget
{
  Q_DECLARE_PUBLIC(qSlicerU70ImageRegistrationFooBarWidget);
protected:
  qSlicerU70ImageRegistrationFooBarWidget* const q_ptr;

public:
  qSlicerU70ImageRegistrationFooBarWidgetPrivate(
    qSlicerU70ImageRegistrationFooBarWidget& object);
  virtual void setupUi(qSlicerU70ImageRegistrationFooBarWidget*);

  bool getTransformSlidersPosition(double pos[3]);
  vtkMRMLLinearTransformNode* getRegistrationLinearTransformNode();
  vtkLinearTransform* getRegistrationTransformToParent();
  void setNewPosition(double pos[3]);

  void init();
};

// --------------------------------------------------------------------------
qSlicerU70ImageRegistrationFooBarWidgetPrivate
::qSlicerU70ImageRegistrationFooBarWidgetPrivate(
  qSlicerU70ImageRegistrationFooBarWidget& object)
  : q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerU70ImageRegistrationFooBarWidgetPrivate
::setupUi(qSlicerU70ImageRegistrationFooBarWidget* widget)
{
  this->Ui_qSlicerU70ImageRegistrationFooBarWidget::setupUi(widget);
}

void qSlicerU70ImageRegistrationFooBarWidgetPrivate::init()
{
  Q_Q(qSlicerU70ImageRegistrationFooBarWidget);


  QObject::connect( this->MRMLTransformSliders_RegistrationTranslate, SIGNAL(valuesChanged()),
    q, SLOT(onTranslateSlidersValuesChanged()));
  //QObject::connect( this->MRMLTransformSliders_RegistrationTranslate, SIGNAL(rangeChanged()),
    //q, SLOT(onTranslateSlidersRangeChanged()));
  QObject::connect(this->MRMLSliderWidget_HorizontalTransform, SIGNAL(valuesChanged()),
      q, SLOT(onTranslateSlidersValuesChanged()));
  QObject::connect(this->MRMLTransformSliders_RegistrationTranslate, SIGNAL(valuesChanged()),
      q, SLOT(onTranslateSlidersValuesChanged()));

//  QObject::connect( this->PushButton_TransformCarmRawImage, SIGNAL(clicked()), q, SLOT(onTransformCarmRawImageClicked()));
  QObject::connect( this->PushButton_Up, SIGNAL(clicked()), q, SLOT(onMoveUpClicked()));
  QObject::connect( this->PushButton_Down, SIGNAL(clicked()), q, SLOT(onMoveDownClicked()));
  QObject::connect( this->PushButton_Left, SIGNAL(clicked()), q, SLOT(onMoveLeftClicked()));
  QObject::connect( this->PushButton_Right, SIGNAL(clicked()), q, SLOT(onMoveRightClicked()));

  q->onTranslateSlidersRangeChanged();
}

bool qSlicerU70ImageRegistrationFooBarWidgetPrivate::getTransformSlidersPosition(double pos[3])
{
  Q_Q(qSlicerU70ImageRegistrationFooBarWidget);
  vtkMRMLLinearTransformNode* transNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->MRMLTransformSliders_RegistrationTranslate->mrmlTransformNode());
  if (!transNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid transform node";
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
vtkMRMLLinearTransformNode* qSlicerU70ImageRegistrationFooBarWidgetPrivate::getRegistrationLinearTransformNode()
{
  Q_Q(qSlicerU70ImageRegistrationFooBarWidget);
  vtkMRMLLinearTransformNode* transNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->MRMLTransformSliders_RegistrationTranslate->mrmlTransformNode());
  if (!transNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid transform node";
    return nullptr;
  }
  return transNode;
}

// --------------------------------------------------------------------------
vtkLinearTransform* qSlicerU70ImageRegistrationFooBarWidgetPrivate::getRegistrationTransformToParent()
{
  Q_Q(qSlicerU70ImageRegistrationFooBarWidget);
  vtkMRMLLinearTransformNode* transNode = this->getRegistrationLinearTransformNode();
  if (!transNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid transform node";
    return nullptr;
  }
  return vtkLinearTransform::SafeDownCast(transNode->GetTransformToParent());
}

// --------------------------------------------------------------------------
void qSlicerU70ImageRegistrationFooBarWidgetPrivate::setNewPosition(double pos[3])
{
  Q_Q(qSlicerU70ImageRegistrationFooBarWidget);
  vtkMRMLLinearTransformNode* transNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->MRMLTransformSliders_RegistrationTranslate->mrmlTransformNode());
  if (!transNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid transform node";
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

//-----------------------------------------------------------------------------
// qSlicerU70ImageRegistrationFooBarWidget methods

//-----------------------------------------------------------------------------
qSlicerU70ImageRegistrationFooBarWidget
::qSlicerU70ImageRegistrationFooBarWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerU70ImageRegistrationFooBarWidgetPrivate(*this) )
{
  Q_D(qSlicerU70ImageRegistrationFooBarWidget);
  d->setupUi(this);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerU70ImageRegistrationFooBarWidget
::~qSlicerU70ImageRegistrationFooBarWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerU70ImageRegistrationFooBarWidget::setXrayNode(vtkMRMLScalarVolumeNode* xrayNode)
{
  Q_D(qSlicerU70ImageRegistrationFooBarWidget);

  vtkMRMLLinearTransformNode* linearTransformNode = nullptr;
  if (xrayNode)
  {
    linearTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(xrayNode->GetParentTransformNode());
    if (!linearTransformNode)
    {
      vtkMRMLScene* scene = xrayNode->GetScene();
      if (!scene)
      {
        qCritical() << Q_FUNC_INFO << ": Xray node has no scene";
      }
      else
      {
        vtkNew<vtkMRMLLinearTransformNode> newTransformNode;
        std::string transformName = "XrayTransform";
        newTransformNode->SetName(transformName.c_str());
        scene->AddNode(newTransformNode);
        linearTransformNode = newTransformNode.GetPointer();
        xrayNode->SetAndObserveTransformNodeID(linearTransformNode->GetID());
      }
    }
  }

  d->MRMLTransformSliders_RegistrationTranslate->setMRMLTransformNode(linearTransformNode);
  d->MRMLTransformSliders->setMRMLTransformNode(linearTransformNode);
  d->MRMLMatrixWidget_TransformMatrix->setMRMLTransformNode(linearTransformNode);

  const bool hasTransform = (linearTransformNode != nullptr);
  d->MRMLTransformSliders_RegistrationTranslate->setEnabled(hasTransform);
  d->MRMLTransformSliders->setEnabled(hasTransform);
  d->MRMLMatrixWidget_TransformMatrix->setEnabled(hasTransform);
  d->MRMLCoordinatesWidget_TranslatePosition->setEnabled(hasTransform);
  d->PushButton_Up->setEnabled(hasTransform);
  d->PushButton_Down->setEnabled(hasTransform);
  d->PushButton_Left->setEnabled(hasTransform);
  d->PushButton_Right->setEnabled(hasTransform);
  d->PushButton_Reset->setEnabled(hasTransform);
}

//-----------------------------------------------------------------------------
void qSlicerU70ImageRegistrationFooBarWidget::onMoveUpClicked()
{
  Q_D(qSlicerU70ImageRegistrationFooBarWidget);
  double pos[3] = {};
  if (d->getTransformSlidersPosition(pos))
  {
    pos[1] += 0.05;
    d->setNewPosition(pos);
  }
}

//-----------------------------------------------------------------------------
void qSlicerU70ImageRegistrationFooBarWidget::onMoveDownClicked()
{
  Q_D(qSlicerU70ImageRegistrationFooBarWidget);
  double pos[3] = {};
  if (d->getTransformSlidersPosition(pos))
  {
    pos[1] -= 0.05;
    d->setNewPosition(pos);
  }
}

//-----------------------------------------------------------------------------
void qSlicerU70ImageRegistrationFooBarWidget::onMoveLeftClicked()
{
  Q_D(qSlicerU70ImageRegistrationFooBarWidget);
  double pos[3] = {};
  if (d->getTransformSlidersPosition(pos))
  {
    pos[0] -= 0.05;
    d->setNewPosition(pos);
  }
}

//-----------------------------------------------------------------------------
void qSlicerU70ImageRegistrationFooBarWidget::onMoveRightClicked()
{
  Q_D(qSlicerU70ImageRegistrationFooBarWidget);
  double pos[3] = {};
  if (d->getTransformSlidersPosition(pos))
  {
    pos[0] += 0.05;
    d->setNewPosition(pos);
  }
}

//-----------------------------------------------------------------------------
void qSlicerU70ImageRegistrationFooBarWidget::onTranslateSlidersValuesChanged()
{
  Q_D(qSlicerU70ImageRegistrationFooBarWidget);
  double pos[3] = {};
  if (d->getTransformSlidersPosition(pos))
  {
    d->MRMLCoordinatesWidget_TranslatePosition->setCoordinates(pos);
  }
}

//-----------------------------------------------------------------------------
void qSlicerU70ImageRegistrationFooBarWidget::onTranslateSlidersRangeChanged()
{
  Q_D(qSlicerU70ImageRegistrationFooBarWidget);
  double min = d->MRMLCoordinatesWidget_TranslatePosition->minimum();
  double max = d->MRMLCoordinatesWidget_TranslatePosition->maximum();
  qDebug() << Q_FUNC_INFO << "Min: " << min << ", max: " << max;
  d->MRMLMatrixWidget_TransformMatrix->setRange(min, max);
  d->MRMLCoordinatesWidget_TranslatePosition->setRange(min, max);
}
