import logging
import os
from typing import Annotated, Optional

import vtk
import itk

import slicer
from slicer.i18n import tr as _
from slicer.i18n import translate
from slicer.ScriptedLoadableModule import *
from slicer.util import VTKObservationMixin
from slicer.parameterNodeWrapper import (
    parameterNodeWrapper,
    WithinRange,
)

from slicer import vtkMRMLScalarVolumeNode


#
# ITK2ProjRegPy
#


class ITK2ProjRegPy(ScriptedLoadableModule):
    """Uses ScriptedLoadableModule base class, available at:
    https://github.com/Slicer/Slicer/blob/main/Base/Python/slicer/ScriptedLoadableModule.py
    """

    def __init__(self, parent):
        ScriptedLoadableModule.__init__(self, parent)
        self.parent.title = _("ITK2ProjRegPy")  # TODO: make this more human readable by adding spaces
        # TODO: set categories (folders where the module shows up in the module selector)
        self.parent.categories = [translate("qSlicerAbstractCoreModule", "Radiotherapy")]
        self.parent.dependencies = []  # TODO: add here list of module names that this module requires
        self.parent.contributors = ["John Doe (AnyWare Corp.)"]  # TODO: replace with "Firstname Lastname (Organization)"
        # TODO: update with short description of the module and a link to online module documentation
        # _() function marks text as translatable to other languages
        self.parent.helpText = _("""
This is an example of scripted loadable module bundled in an extension.
See more information in <a href="https://github.com/organization/projectname#ITK2ProjRegPy">module documentation</a>.
""")
        # TODO: replace with organization, grant and thanks
        self.parent.acknowledgementText = _("""
This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc., Andras Lasso, PerkLab,
and Steve Pieper, Isomics, Inc. and was partially funded by NIH grant 3P41RR013218-12S1.
""")

        # Additional initialization step after application startup is complete
        slicer.app.connect("startupCompleted()", registerSampleData)


#
# Register sample data sets in Sample Data module
#


def registerSampleData():
    """Add data sets to Sample Data module."""
    # It is always recommended to provide sample data for users to make it easy to try the module,
    # but if no sample data is available then this method (and associated startupCompeted signal connection) can be removed.

    import SampleData

    iconsPath = os.path.join(os.path.dirname(__file__), "Resources/Icons")

    # To ensure that the source code repository remains small (can be downloaded and installed quickly)
    # it is recommended to store data sets that are larger than a few MB in a Github release.

    # ITK2ProjRegPy1
    SampleData.SampleDataLogic.registerCustomSampleDataSource(
        # Category and sample name displayed in Sample Data module
        category="ITK2ProjRegPy",
        sampleName="ITK2ProjRegPy1",
        # Thumbnail should have size of approximately 260x280 pixels and stored in Resources/Icons folder.
        # It can be created by Screen Capture module, "Capture all views" option enabled, "Number of images" set to "Single".
        thumbnailFileName=os.path.join(iconsPath, "ITK2ProjRegPy1.png"),
        # Download URL and target file name
        uris="https://github.com/Slicer/SlicerTestingData/releases/download/SHA256/998cb522173839c78657f4bc0ea907cea09fd04e44601f17c82ea27927937b95",
        fileNames="ITK2ProjRegPy1.nrrd",
        # Checksum to ensure file integrity. Can be computed by this command:
        #  import hashlib; print(hashlib.sha256(open(filename, "rb").read()).hexdigest())
        checksums="SHA256:998cb522173839c78657f4bc0ea907cea09fd04e44601f17c82ea27927937b95",
        # This node name will be used when the data set is loaded
        nodeNames="ITK2ProjRegPy1",
    )

    # ITK2ProjRegPy2
    SampleData.SampleDataLogic.registerCustomSampleDataSource(
        # Category and sample name displayed in Sample Data module
        category="ITK2ProjRegPy",
        sampleName="ITK2ProjRegPy2",
        thumbnailFileName=os.path.join(iconsPath, "ITK2ProjRegPy2.png"),
        # Download URL and target file name
        uris="https://github.com/Slicer/SlicerTestingData/releases/download/SHA256/1a64f3f422eb3d1c9b093d1a18da354b13bcf307907c66317e2463ee530b7a97",
        fileNames="ITK2ProjRegPy2.nrrd",
        checksums="SHA256:1a64f3f422eb3d1c9b093d1a18da354b13bcf307907c66317e2463ee530b7a97",
        # This node name will be used when the data set is loaded
        nodeNames="ITK2ProjRegPy2",
    )


#
# ITK2ProjRegPyParameterNode
#


@parameterNodeWrapper
class ITK2ProjRegPyParameterNode:
    """
    The parameters needed by module.
    """

    CTNode: vtkMRMLScalarVolumeNode
    DRRNode_1: vtkMRMLScalarVolumeNode
    DRRNode_2: vtkMRMLScalarVolumeNode

#
# ITK2ProjRegPyWidget
#


class ITK2ProjRegPyWidget(ScriptedLoadableModuleWidget, VTKObservationMixin):
    """Uses ScriptedLoadableModuleWidget base class, available at:
    https://github.com/Slicer/Slicer/blob/main/Base/Python/slicer/ScriptedLoadableModule.py
    """

    def __init__(self, parent=None) -> None:
        """Called when the user opens the module the first time and the widget is initialized."""
        ScriptedLoadableModuleWidget.__init__(self, parent)
        VTKObservationMixin.__init__(self)  # needed for parameter node observation
        self.logic = None
        self._parameterNode = None
        self._parameterNodeGuiTag = None

    def setup(self) -> None:
        """Called when the user opens the module the first time and the widget is initialized."""
        ScriptedLoadableModuleWidget.setup(self)

        # Load widget from .ui file (created by Qt Designer).
        # Additional widgets can be instantiated manually and added to self.layout.
        uiWidget = slicer.util.loadUI(self.resourcePath("UI/ITK2ProjRegPy.ui"))
        self.layout.addWidget(uiWidget)
        self.ui = slicer.util.childWidgetVariables(uiWidget)

        # Set scene in MRML widgets. Make sure that in Qt designer the top-level qMRMLWidget's
        # "mrmlSceneChanged(vtkMRMLScene*)" signal in is connected to each MRML widget's.
        # "setMRMLScene(vtkMRMLScene*)" slot.
        uiWidget.setMRMLScene(slicer.mrmlScene)

        # Create logic class. Logic implements all computations that should be possible to run
        # in batch mode, without a graphical user interface.
        self.logic = ITK2ProjRegPyLogic()

        # Connections

        # These connections ensure that we update parameter node when scene is closed
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.StartCloseEvent, self.onSceneStartClose)
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.EndCloseEvent, self.onSceneEndClose)

        # Buttons
        self.ui.PushButton_ApplyReg.connect("clicked(bool)", self.onApplyButton)

        # Make sure parameter node is initialized (needed for module reload)
        self.initializeParameterNode()

    def cleanup(self) -> None:
        """Called when the application closes and the module widget is destroyed."""
        self.removeObservers()

    def enter(self) -> None:
        """Called each time the user opens this module."""
        # Make sure parameter node exists and observed
        self.initializeParameterNode()

    def exit(self) -> None:
        """Called each time the user opens a different module."""
        # Do not react to parameter node changes (GUI will be updated when the user enters into the module)
        if self._parameterNode:
            self._parameterNode.disconnectGui(self._parameterNodeGuiTag)
            self._parameterNodeGuiTag = None
            self.removeObserver(self._parameterNode, vtk.vtkCommand.ModifiedEvent, self._checkCanApply)

    def onSceneStartClose(self, caller, event) -> None:
        """Called just before the scene is closed."""
        # Parameter node will be reset, do not use it anymore
        self.setParameterNode(None)

    def onSceneEndClose(self, caller, event) -> None:
        """Called just after the scene is closed."""
        # If this module is shown while the scene is closed then recreate a new parameter node immediately
        if self.parent.isEntered:
            self.initializeParameterNode()

    def initializeParameterNode(self) -> None:
        """Ensure parameter node exists and observed."""
        # Parameter node stores all user choices in parameter values, node selections, etc.
        # so that when the scene is saved and reloaded, these settings are restored.

        self.setParameterNode(self.logic.getParameterNode())

        # Select default input nodes if nothing is selected yet to save a few clicks for the user
        #if not self._parameterNode.inputVolume:
        #    firstVolumeNode = slicer.mrmlScene.GetFirstNodeByClass("vtkMRMLScalarVolumeNode")
        #    if firstVolumeNode:
        #        self._parameterNode.inputVolume = firstVolumeNode

    def setParameterNode(self, inputParameterNode: Optional[ITK2ProjRegPyParameterNode]) -> None:
        """
        Set and observe parameter node.
        Observation is needed because when the parameter node is changed then the GUI must be updated immediately.
        """

        if self._parameterNode:
            self._parameterNode.disconnectGui(self._parameterNodeGuiTag)
            self.removeObserver(self._parameterNode, vtk.vtkCommand.ModifiedEvent, self._checkCanApply)
        self._parameterNode = inputParameterNode
        if self._parameterNode:
            # Note: in the .ui file, a Qt dynamic property called "SlicerParameterName" is set on each
            # ui element that needs connection.
            self._parameterNodeGuiTag = self._parameterNode.connectGui(self.ui)
            self.addObserver(self._parameterNode, vtk.vtkCommand.ModifiedEvent, self._checkCanApply)
            self._checkCanApply()

    def _checkCanApply(self, caller=None, event=None) -> None:
        if self._parameterNode and self._parameterNode.CTNode and self._parameterNode.DRRNode_1 and self._parameterNode.DRRNode_2:
            self.ui.PushButton_ApplyReg.toolTip = _("Apply Reg")
            self.ui.PushButton_ApplyReg.enabled = True
        else:
            self.ui.PushButton_ApplyReg.toolTip = _("Select nodes")
            self.ui.PushButton_ApplyReg.enabled = False

    def onApplyButton(self) -> None:
        """Run processing when user clicks "Apply" button."""
        with slicer.util.tryWithErrorDisplay(_("Failed to compute results."), waitCursor=True):
            # Compute output
            self.logic.runRegistration(
        self._parameterNode.CTNode,
        self._parameterNode.DRRNode_1,
        self._parameterNode.DRRNode_2
        )



#
# ITK2ProjRegPyLogic
#


class ITK2ProjRegPyLogic(ScriptedLoadableModuleLogic):
    """This class should implement all the actual
    computation done by your module.  The interface
    should be such that other python code can import
    this class and make use of the functionality without
    requiring an instance of the Widget.
    Uses ScriptedLoadableModuleLogic base class, available at:
    https://github.com/Slicer/Slicer/blob/main/Base/Python/slicer/ScriptedLoadableModule.py
    """

    def __init__(self) -> None:
        """Called when the logic class is instantiated. Can be used for initializing member variables."""
        ScriptedLoadableModuleLogic.__init__(self)

    def getParameterNode(self):
        return ITK2ProjRegPyParameterNode(super().getParameterNode())

    def vtkMRML_to_itk(self,volumeNode, pixel_type=itk.F):
        """
        Convert a vtkMRMLScalarVolumeNode to a 3D ITK image.
        Uses itk.image_from_vtk_image() bridge function.
        Always returns itk.Image[pixel_type, 3].

        Args:
        volumeNode: vtkMRMLScalarVolumeNode
        pixel_type: ITK pixel type (default itk.F = float)

        Returns:
        itk_image_3d: itk.Image[pixel_type, 3]
        """
        if volumeNode is None:
            slicer.util.errorDisplay("Input volume node is None")
            return None

        vtkImage = volumeNode.GetImageData()
        if vtkImage is None:
            return None

        # Convert VTK -> ITK
        itk_image_any = itk.image_from_vtk_image(vtkImage)

        # If image dimension is not 3, we promote *after* casting
        dim = itk_image_any.GetImageDimension()

        # First, cast pixel type (keep same dimension)
        if dim == 2:
            InputImageType  = type(itk_image_any)
            OutputImageType = itk.Image[pixel_type, 2]
        elif dim == 3:
            InputImageType  = type(itk_image_any)
            OutputImageType = itk.Image[pixel_type, 3]
        else:
            raise RuntimeError(f"Unsupported image dimension {dim}")

        CastFilter = itk.CastImageFilter[InputImageType, OutputImageType]
        caster = CastFilter.New()
        caster.SetInput(itk_image_any)
        caster.Update()
        casted_itk = caster.GetOutput()

        # If the image is already 3D, return it
        if dim == 3:
            return casted_itk

        # Otherwise (2D), promote to 3D with z=1 using NumPy
        arr2d = itk.array_from_image(casted_itk)  # shape (y,x)

        # Create a 3D array (z,y,x) with depth=1
        arr3d = arr2d[np.newaxis, ...]            # shape (1,y,x)
        arr3d = np.transpose(arr3d, (2,1,0))      # shape (x,y,z)

        itk_image_3d = itk.image_from_array(arr3d, is_vector=False)

        # Set spacing & origin
        spacing2d = volumeNode.GetSpacing()
        origin2d  = volumeNode.GetOrigin()
        spacing3d = (spacing2d[0], spacing2d[1], 1.0)
        origin3d  = (origin2d[0], origin2d[1], 0.0)

        itk_image_3d.SetSpacing(spacing3d)
        itk_image_3d.SetOrigin(origin3d)

        return itk_image_3d

    def runRegistration(self, CTNode, DRRNode_1, DRRNode_2):
        """
        Converts volumes and runs a simple 2D/3D registration pipeline.
        """
        if CTNode is None or not hasattr(CTNode, "GetImageData"):
            slicer.util.errorDisplay("CTNode is not a volume node!")
            return
        if DRRNode_1 is None or not hasattr(DRRNode_1, "GetImageData"):
            slicer.util.errorDisplay("DRRNode_1 is not a volume node!")
            return
        if DRRNode_2 is None or not hasattr(DRRNode_2, "GetImageData"):
            slicer.util.errorDisplay("DRRNode_2 is not a volume node!")
            return

        # Convert selected volumes

        CTItk    = self.vtkMRML_to_itk(CTNode)
        DRRItk_1 = self.vtkMRML_to_itk(DRRNode_1)
        DRRItk_2 = self.vtkMRML_to_itk(DRRNode_2)

        if CTItk is None or DRRItk_1 is None or DRRItk_2 is None:
            slicer.util.errorDisplay("Failed to convert volumes to ITK images.")
            return

        # Define ITK image types
        ImageType3D = itk.Image[itk.F, 3]

        # Instantiate registration classes
        RegistrationType = itk.TwoProjectionImageRegistrationMethod[ImageType3D,ImageType3D]
        registration = RegistrationType.New()

        MetricType = itk.NormalizedCorrelationTwoImageToOneImageMetric[ImageType3D,ImageType3D]
        metric = MetricType.New()

        OptimizerType = itk.PowellOptimizer
        optimizer = OptimizerType.New()

        TransformType = itk.Euler3DTransform[itk.D]
        transform = TransformType.New()

        InterpolatorType = itk.SiddonJacobsRayCastInterpolateImageFunction[ImageType3D,itk.D]
        interpolator1 = InterpolatorType.New()
        interpolator2 = InterpolatorType.New()

        # Wire up
        registration.SetMetric(metric)
        registration.SetOptimizer(optimizer)
        registration.SetTransform(transform)
        registration.SetInterpolator1(interpolator1)
        registration.SetInterpolator2(interpolator2)
        registration.SetFixedImage1(DRRItk_1)
        registration.SetFixedImage2(DRRItk_2)
        registration.SetMovingImage(CTItk)

        # TODO: Set an initial guess, optimizer settings, etc.

        # Run registration
        registration.StartRegistration()
        resultParams = registration.GetLastTransformParameters()

        slicer.util.infoDisplay(f"Registration finished: {resultParams}")


    # def process(self,
    #             inputVolume: vtkMRMLScalarVolumeNode,
    #             outputVolume: vtkMRMLScalarVolumeNode,
    #             imageThreshold: float,
    #             invert: bool = False,
    #             showResult: bool = True) -> None:
    #     """
    #     Run the processing algorithm.
    #     Can be used without GUI widget.
    #     :param inputVolume: volume to be thresholded
    #     :param outputVolume: thresholding result
    #     :param imageThreshold: values above/below this threshold will be set to 0
    #     :param invert: if True then values above the threshold will be set to 0, otherwise values below are set to 0
    #     :param showResult: show output volume in slice viewers
    #     """

    #     if not inputVolume or not outputVolume:
    #         raise ValueError("Input or output volume is invalid")

    #     import time

    #     startTime = time.time()
    #     logging.info("Processing started")

    #     # Compute the thresholded output volume using the "Threshold Scalar Volume" CLI module
    #     cliParams = {
    #         "InputVolume": inputVolume.GetID(),
    #         "OutputVolume": outputVolume.GetID(),
    #         "ThresholdValue": imageThreshold,
    #         "ThresholdType": "Above" if invert else "Below",
    #     }
    #     cliNode = slicer.cli.run(slicer.modules.thresholdscalarvolume, None, cliParams, wait_for_completion=True, update_display=showResult)
    #     # We don't need the CLI module node anymore, remove it to not clutter the scene with it
    #     slicer.mrmlScene.RemoveNode(cliNode)

    #     stopTime = time.time()
    #     logging.info(f"Processing completed in {stopTime-startTime:.2f} seconds")


#
# ITK2ProjRegPyTest
#


class ITK2ProjRegPyTest(ScriptedLoadableModuleTest):
    """
    This is the test case for your scripted module.
    Uses ScriptedLoadableModuleTest base class, available at:
    https://github.com/Slicer/Slicer/blob/main/Base/Python/slicer/ScriptedLoadableModule.py
    """

    def setUp(self):
        """Do whatever is needed to reset the state - typically a scene clear will be enough."""
        slicer.mrmlScene.Clear()

    def runTest(self):
        """Run as few or as many tests as needed here."""
        self.setUp()
        self.test_ITK2ProjRegPy1()

    def test_ITK2ProjRegPy1(self):
        """Ideally you should have several levels of tests.  At the lowest level
        tests should exercise the functionality of the logic with different inputs
        (both valid and invalid).  At higher levels your tests should emulate the
        way the user would interact with your code and confirm that it still works
        the way you intended.
        One of the most important features of the tests is that it should alert other
        developers when their changes will have an impact on the behavior of your
        module.  For example, if a developer removes a feature that you depend on,
        your test should break so they know that the feature is needed.
        """

        self.delayDisplay("Starting the test")

        # Get/create input data

        import SampleData

        registerSampleData()
        inputVolume = SampleData.downloadSample("ITK2ProjRegPy1")
        self.delayDisplay("Loaded test data set")

        inputScalarRange = inputVolume.GetImageData().GetScalarRange()
        self.assertEqual(inputScalarRange[0], 0)
        self.assertEqual(inputScalarRange[1], 695)

        outputVolume = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLScalarVolumeNode")
        threshold = 100

        # Test the module logic

        logic = ITK2ProjRegPyLogic()

        # Test algorithm with non-inverted threshold
        logic.process(inputVolume, outputVolume, threshold, True)
        outputScalarRange = outputVolume.GetImageData().GetScalarRange()
        self.assertEqual(outputScalarRange[0], inputScalarRange[0])
        self.assertEqual(outputScalarRange[1], threshold)

        # Test algorithm with inverted threshold
        logic.process(inputVolume, outputVolume, threshold, False)
        outputScalarRange = outputVolume.GetImageData().GetScalarRange()
        self.assertEqual(outputScalarRange[0], inputScalarRange[0])
        self.assertEqual(outputScalarRange[1], inputScalarRange[1])

        self.delayDisplay("Test passed")
