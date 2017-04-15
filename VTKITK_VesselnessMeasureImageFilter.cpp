#include <vtkAutoInit.h> 
#include "vtkRenderer.h"  
#include "vtkRenderWindow.h"  
#include "vtkRenderWindowInteractor.h"  
#include "vtkActor.h"  
#include "vtkSmartPointer.h"  
#include "vtkProperty.h"  
#include "vtkCamera.h"  
#include "vtkDICOMImageReader.h"  
#include "vtkImageReader.h"
#include "vtkImageCast.h"  
#include "vtkPiecewiseFunction.h"  
#include "vtkColorTransferFunction.h"  
#include "vtkVolumeProperty.h"  
#include "vtkVolumeRayCastCompositeFunction.h"  
#include "vtkFixedPointVolumeRayCastMapper.h"  
#include "vtkVolumeRayCastMapper.h"  
#include "vtkVolume.h"  
#include "vtkVolumeRayCastMIPFunction.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkStructuredGridReader.h"
#include "vtkImageExport.h"

#include "itkImage.h"
#include "itkImageSeriesReader.h"
#include "itkImageFileWriter.h"
#include "itkImageFileReader.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkNumericSeriesFileNames.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkGDCMImageIO.h"
#include <Windows.h>
#include "itkShiftScaleImageFilter.h"
#include "itkHessianRecursiveGaussianImageFilter.h"
#include "itkHessian3DToVesselnessMeasureImageFilter.h"
#include "itkHessianSmoothed3DToVesselnessMeasureImageFilter.h"
#include "itkImageToVTKImageFilter.h"
#include "itkVTKImageExport.h"
#include "itkVTKImageimport.h"
#include "vtkITKUtility.h"

VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkRenderingFreeType);
VTK_MODULE_INIT(vtkInteractionStyle);

int main(int argc, char* argv[] )
{
	VTK_MODULE_INIT(vtkRenderingOpenGL2);

	typedef double InputPixelType;
	typedef double  OutputPixelType;
	const unsigned int Dimension = 3;
	typedef itk::Image<InputPixelType, Dimension> ImageType;
	typedef itk::Image< OutputPixelType, Dimension >  OutputImageType;

//读取DICOM
	//typedef itk::ImageSeriesReader<ImageType> ReaderType;
	//typedef itk::GDCMImageIO  ImageIOType;
	//ImageIOType::Pointer dicomIO = ImageIOType::New();
	//typedef itk::GDCMSeriesFileNames  NameGeneratorType;
	//NameGeneratorType::Pointer nameGenerator = NameGeneratorType::New();
	///*nameGenerator->SetInputDirectory("G:\\STUDY\\CS\\VTK\\CT_DATA2");*/
	//nameGenerator->SetInputDirectory("G:\\STUDY\\CS\\VTK\\DICOM\\S35500\\S5010");
	//const ReaderType::FileNamesContainer & filenames =
	//	nameGenerator->GetInputFileNames();
	//ReaderType::Pointer reader = ReaderType::New();
	//reader->SetImageIO(dicomIO);
	//reader->SetFileNames(filenames);

//读取nrrd
	const char * inputImage = "G:\\STUDY\\CS\\VTK\\NRRD_DATA\\CT-chest.nrrd";
	typedef itk::ImageFileReader<ImageType> ReaderType;
	typedef itk::ImageFileReader< ImageType >  ReaderType;
	ReaderType::Pointer reader = ReaderType::New();
	reader->SetFileName(inputImage);

	try
	{
		reader->Update();
	}
	catch (itk::ExceptionObject &ex)
	{
		std::cout << ex << std::endl;
		return EXIT_FAILURE;
	}
	std::cout << "reader update" << std::endl;
//二值化
	/*typedef itk::BinaryThresholdImageFilter<ImageType, OutputImageType> FilterType;
	FilterType::Pointer filter = FilterType::New();
	filter->SetInput(reader->GetOutput());
	filter->SetUpperThreshold(180);
	filter->SetLowerThreshold(10);
	filter->SetOutsideValue(255);
	filter->SetInsideValue(0);
	filter->Update();*/



	typedef itk::HessianRecursiveGaussianImageFilter<ImageType> HessianFilterType;
	HessianFilterType::Pointer hessianFilter = HessianFilterType::New();
	hessianFilter->SetSigma(0.3);
	hessianFilter->SetInput(reader->GetOutput());
	//hessianFilter->Update();
	std::cout << "hessian Filter" << std::endl;

	typedef itk::Hessian3DToVesselnessMeasureImageFilter<OutputPixelType> VesselMeasureFilterType;
	VesselMeasureFilterType::Pointer vesselFilter = VesselMeasureFilterType::New();
	vesselFilter->SetInput(hessianFilter->GetOutput());
	vesselFilter->SetAlpha1(0.5);
	vesselFilter->SetAlpha2(0.5);
	//vesselFilter->Update();
	std::cout << "vesselFilter update" << std::endl;

	typedef itk::VTKImageExport< ImageType > ITKExportType;
	ITKExportType::Pointer itk_export = ITKExportType::New();
	itk_export->SetInput(vesselFilter->GetOutput());

	vtkImageImport * vtk_import = vtkImageImport::New();
	ConnectPipelines(itk_export, vtk_import);
	vtk_import->Update();
	std::cout << "vtkimport update" << std::endl;

	typedef itk::ImageFileWriter< ImageType > WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetInput(vesselFilter->GetOutput());
	writer->SetFileName("G:\\STUDY\\CS\\ITK\\vessel.nrrd");
	std::cout << "writer update" << std::endl;
	try
	{
		writer->Update();
	}
	catch (itk::ExceptionObject & error)
	{
		std::cerr << "Error: " << error << std::endl;
		return EXIT_FAILURE;
	}

//VTK------------------------------------------------------------------------------------------------------------------------

	vtkSmartPointer<vtkPiecewiseFunction>opacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
//nrrd	
	/*opacityTransferFunction->AddPoint(120, 0.0);
	opacityTransferFunction->AddPoint(250, 1.0);
	opacityTransferFunction->AddPoint(520, 1.0);
	opacityTransferFunction->AddPoint(650, 0.0);*/

//不透明度DICOM
	opacityTransferFunction->AddPoint(-3024, 0, 0.5, 0.0);
	opacityTransferFunction->AddPoint(-220, 0, .49, .61);
	opacityTransferFunction->AddPoint(625, .71, .5, 0.0);
	opacityTransferFunction->AddPoint(3071, 0.0, 0.5, 0.0);

	vtkSmartPointer<vtkColorTransferFunction>colorTransferFunction = vtkSmartPointer<vtkColorTransferFunction>::New();
//nrrd
	/*colorTransferFunction->AddRGBPoint(120, 255 / 255.0, 98 / 255.0, 98 / 255.0);
	colorTransferFunction->AddRGBPoint(250, 255 / 255.0, 255 / 255.0, 180 / 255.0);
	colorTransferFunction->AddRGBPoint(520, 1.0, 1.0, 1.0);
	colorTransferFunction->AddRGBPoint(650, 1.0, 1.0, 1.0);*/

//颜色DICOM
	colorTransferFunction->AddRGBPoint(-3024, 0, 0, 0, 0.5, 0.0);
	colorTransferFunction->AddRGBPoint(-200, 0.73, 0.25, 0.30, 0.49, .61);
	colorTransferFunction->AddRGBPoint(641, .90, .82, .56, .5, 0.0);
	colorTransferFunction->AddRGBPoint(3071, 1, 1, 1, .5, 0.0);

	vtkSmartPointer<vtkPiecewiseFunction>gradientTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
//灰度值变化梯度与不透明度的关系nrrd
	/*gradientTransferFunction->AddPoint(120, 2.0);
	gradientTransferFunction->AddPoint(250, 2.0);
	gradientTransferFunction->AddPoint(520, 0.1);
	gradientTransferFunction->AddPoint(650, 0.1);*/

//梯度DICOM
	gradientTransferFunction->AddPoint(0, 2.0);
	gradientTransferFunction->AddPoint(500, 2.0);
	gradientTransferFunction->AddSegment(600, 0.73, 900, 0.9);
	gradientTransferFunction->AddPoint(1300, 0.1);

	vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
	volumeProperty->SetColor(colorTransferFunction);
	volumeProperty->SetScalarOpacity(opacityTransferFunction);
	volumeProperty->SetGradientOpacity(gradientTransferFunction);
	volumeProperty->ShadeOn();//阴影  
	volumeProperty->SetInterpolationTypeToLinear();//直线与样条插值之间逐发函数  
	volumeProperty->SetAmbient(0.5);//环境光系数  
	volumeProperty->SetDiffuse(1.0);//漫反射  
	volumeProperty->SetSpecular(0.5);//高光系数  
	volumeProperty->SetSpecularPower(25);//高光强度  
	volumeProperty->SetInterpolationTypeToLinear();

	vtkSmartPointer <vtkSmartVolumeMapper> volumeMapper = vtkSmartPointer <vtkSmartVolumeMapper>::New();
	//volumeMapper->SetInterpolationModeToLinear();

	volumeMapper->SetInputConnection(vtk_import->GetOutputPort());//载入体绘制方法

	vtkSmartPointer<vtkVolume>volume = vtkSmartPointer<vtkVolume>::New();
	volume->SetMapper(volumeMapper);
	volume->SetProperty(volumeProperty);//设置体属性  

	vtkSmartPointer<vtkRenderer>ren1 = vtkSmartPointer<vtkRenderer>::New();
	ren1->AddVolume(volume);
	ren1->SetBackground(1, 1, 1);

	vtkSmartPointer<vtkRenderWindow>renWin = vtkSmartPointer<vtkRenderWindow>::New();
	renWin->AddRenderer(ren1);
	renWin->SetSize(800, 800);

	vtkSmartPointer<vtkRenderWindowInteractor>iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	iren->SetRenderWindow(renWin);

	//volumeMapper->SetRequestedRenderModeToRayCast();
	renWin->Render();
	iren->Start();
}
