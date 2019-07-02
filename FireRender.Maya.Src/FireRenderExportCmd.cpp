#include "FireRenderExportCmd.h"
#include "FireRenderContext.h"
#include "FireRenderUtils.h"
#include <maya/MArgDatabase.h>
#include <maya/MItDag.h>
#include <maya/MDagPath.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MObjectArray.h>
#include <maya/MArgList.h>
#include <maya/MStatus.h>
#include <maya/MDagPathArray.h>
#include <maya/MTime.h>
#include <maya/MAnimControl.h>
#include <maya/MRenderUtil.h>
#include <maya/MCommonRenderSettingsData.h>

#include <fstream>

#ifdef __linux__
	#include <../RprLoadStore.h>
#else
	#include <../inc/RprLoadStore.h>
#endif

FireRenderExportCmd::FireRenderExportCmd()
{
}

FireRenderExportCmd::~FireRenderExportCmd()
{
}

void * FireRenderExportCmd::creator()
{
	return new FireRenderExportCmd;
}

MSyntax FireRenderExportCmd::newSyntax()
{
	MStatus status;
	MSyntax syntax;

	CHECK_MSTATUS(syntax.addFlag(kMaterialFlag, kMaterialFlagLong, MSyntax::kString));
	CHECK_MSTATUS(syntax.addFlag(kFilePathFlag, kFilePathFlagLong, MSyntax::kString));
	CHECK_MSTATUS(syntax.addFlag(kSelectionFlag, kSelectionFlagLong, MSyntax::kNoArg));
	CHECK_MSTATUS(syntax.addFlag(kAllFlag, kAllFlagLong, MSyntax::kNoArg));
	CHECK_MSTATUS(syntax.addFlag(kFramesFlag, kFramesFlagLong, MSyntax::kBoolean, MSyntax::kLong, MSyntax::kLong, MSyntax::kBoolean));
	CHECK_MSTATUS(syntax.addFlag(kCompressionFlag, kCompressionFlagLong, MSyntax::kString));

	return syntax;
}

MStatus FireRenderExportCmd::doIt(const MArgList & args)
{
	MStatus status;

	MArgDatabase argData(syntax(), args);

	// for the moment, the LoadStore library of RPR only supports the export/import of all scene
	if (  !argData.isFlagSet(kAllFlag)  )
	{
		MGlobal::displayError("This feature is not supported.");
		return MS::kFailure;
	}

	MString filePath;
	if (argData.isFlagSet(kFilePathFlag))
	{
		argData.getFlagArgument(kFilePathFlag, 0, filePath);
	}
	else
	{
		MGlobal::displayError("File path is missing, use -file flag");
		return MS::kFailure;
	}

	MString materialName;
	if (argData.isFlagSet(kMaterialFlag))
	{
		argData.getFlagArgument(kMaterialFlag, 0, materialName);

		MSelectionList sList;
		sList.add(materialName);

		MObject node;
		sList.getDependNode(0, node);
		if (node.isNull())
		{
			MGlobal::displayError("Invalid shader");
			return MS::kFailure;
		}

		FireRenderContext context;
		context.setCallbackCreationDisabled(true);

		rpr_int res = context.initializeContext();
		if (res != RPR_SUCCESS)
		{
			MString msg;
			FireRenderError(res, msg, true);
			return MS::kFailure;
		}

		context.setCallbackCreationDisabled(true);

		auto shader = context.GetShader(node);
		if (!shader)
		{
			MGlobal::displayError("Invalid shader");
			return MS::kFailure;
		}

		MGlobal::displayError("This feature is not supported.\n");
		return MS::kFailure;
	}

	bool isSequenceExportEnabled = false;
	int firstFrame = 1;
	int lastFrame = 1;
	bool isExportAsSingleFileEnabled = false;
	if (argData.isFlagSet(kFramesFlag))
	{
		argData.getFlagArgument(kFramesFlag, 0, isSequenceExportEnabled);
		argData.getFlagArgument(kFramesFlag, 1, firstFrame);
		argData.getFlagArgument(kFramesFlag, 2, lastFrame);
		argData.getFlagArgument(kFramesFlag, 3, isExportAsSingleFileEnabled);
	}

	MString compressionOption = "None";
	if (argData.isFlagSet(kCompressionFlag))
	{
		argData.getFlagArgument(kCompressionFlag, 0, compressionOption);
	}

	if (argData.isFlagSet(kAllFlag))
	{
		// initialize
		MCommonRenderSettingsData settings;
		MRenderUtil::getCommonRenderSettings(settings);

		FireRenderContext context;
		context.SetRenderType(RenderType::ProductionRender);
		context.buildScene();

		context.setResolution(settings.width, settings.height, true);

		MDagPathArray cameras = getRenderableCameras();

		if (cameras.length() == 0)
		{
			MGlobal::displayError("Renderable cameras haven't been found! Using default camera!");

			MDagPath cameraPath = getDefaultCamera();
			MString cameraName = getNameByDagPath(cameraPath);
			context.setCamera(cameraPath, true);
		}
		else  // (cameras.length() >= 1)
		{
			context.setCamera(cameras[0], true);
		}

		// setup frame ranges
		if (!isSequenceExportEnabled)
		{
			firstFrame = lastFrame = 1;
		}

		// process file path
		MString name;
		MString ext;

		int pos = filePath.rindexW(L".");
		if (pos == -1)
		{
			name = filePath;
			ext = ".rpr";
		}
		else
		{
			name = filePath.substringW(0, pos - 1);
			ext = filePath.substringW(pos, filePath.length() - 1);

			if (ext != ".rpr")
			{
				name += ext;
				ext = ".rpr";
			}
		}

		// process each frame
		for (int frame = firstFrame; frame <= lastFrame; ++frame)
		{
			// Move the animation to the next frame.
			MTime time;
			time.setValue(static_cast<double>(frame));
			MStatus isTimeSet = MAnimControl::setCurrentTime(time);
			CHECK_MSTATUS(isTimeSet);

			// Refresh the context so it matches the
			// current animation state and start the render.
			context.Freshen();

			// update file path
			MString newFilePath;
			if (isSequenceExportEnabled)
			{
				newFilePath = name;
				newFilePath += "_";
				newFilePath += frame;
				newFilePath += ext;
			}
			else
			{
				newFilePath = filePath;
			}

			unsigned int exportFlags = 0;
			if (!isExportAsSingleFileEnabled)
			{
				exportFlags = RPRLOADSTORE_EXPORTFLAG_EXTERNALFILES;
			}

			if (compressionOption == "None")
			{
				// don't set any flag
				// this line exists for better logic readibility; also maybe will need to set flag here in the future
			}
			else if (compressionOption == "Level 1")
			{
				exportFlags = exportFlags | RPRLOADSTORE_EXPORTFLAG_COMPRESS_IMAGE_LEVEL_1;
			}
			else if (compressionOption == "Level 2")
			{
				exportFlags = exportFlags | RPRLOADSTORE_EXPORTFLAG_COMPRESS_IMAGE_LEVEL_2;
			}
			else if (compressionOption == "Level 3")
			{
				exportFlags = exportFlags | RPRLOADSTORE_EXPORTFLAG_COMPRESS_IMAGE_LEVEL_2 | RPRLOADSTORE_EXPORTFLAG_COMPRESS_FLOAT_TO_HALF_NORMALS | RPRLOADSTORE_EXPORTFLAG_COMPRESS_FLOAT_TO_HALF_UV;
			}

			// launch export
			rpr_int statuExport = rprsExport(newFilePath.asChar(), context.context(), context.scene(),
				0, 0, 0, 0, 0, 0, exportFlags);

			if (statuExport != RPR_SUCCESS)
			{
				MGlobal::displayError("Unable to export fire render scene\n");
				return MS::kFailure;
			}
		}

		return MS::kSuccess;
	}

	if (argData.isFlagSet(kSelectionFlag))
	{
		//initialize
		FireRenderContext context;
		context.setCallbackCreationDisabled(true);
		context.buildScene();

		MDagPathArray cameras = getRenderableCameras();
		if ( cameras.length() >= 1 )
		{
			context.setCamera(cameras[0]);
		}

		MSelectionList sList;
		std::vector<rpr_shape> shapes;
		std::vector<rpr_light> lights;
		rpr_light envLight = nullptr;

		MGlobal::getActiveSelectionList(sList);
		for (unsigned int i = 0; i < sList.length(); i++)
		{
			MDagPath path;

			sList.getDagPath(i, path);
			path.extendToShape();

			MObject node = path.node();
			if (node.isNull())
				continue;

			MFnDependencyNode nodeFn(node);

			FireRenderObject* frObject = context.getRenderObject(getNodeUUid(node));
			if (!frObject)
				continue;

			if (auto light = dynamic_cast<FireRenderLight*>(frObject))
			{
				if (light->data().isAreaLight && light->data().areaLight)
				{
					shapes.push_back(light->data().areaLight.Handle());
				}
				else
				{
					if (light->data().light)
					{
						lights.push_back(light->data().light.Handle());
					}
				}
			}

			else if (auto mesh = dynamic_cast<FireRenderMesh*>(frObject))
			{
				for (auto element : mesh->Elements())
				{
					shapes.push_back(element.shape.Handle());
				}
			}
			else if (auto frEnvLight = static_cast<FireRenderEnvLight*>(frObject))
			{
				envLight = frEnvLight->data().Handle();
			}

		}

		MGlobal::displayError("This feature is not supported.\n");
		return MS::kFailure;
	}

	return MS::kFailure;
}
