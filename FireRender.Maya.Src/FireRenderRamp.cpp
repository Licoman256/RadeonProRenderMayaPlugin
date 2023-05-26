/**********************************************************************
Copyright 2020 Advanced Micro Devices, Inc
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
	http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************/
#include "FireRenderRamp.h"
#include "FireRenderUtils.h"
#include "MayaStandardNodesSupport/RampNodeConverter.h"
#include "MayaStandardNodesSupport/NodeProcessingUtils.h"

#include <maya/MFnNumericAttribute.h>
#include <maya/MColor.h>

namespace
{
	namespace Attribute
	{
		MObject rampInterpolationMode;
		MObject rampUVType;
		MObject inputRamp;

		MObject	output;
	}
}

MStatus FireMaya::RPRRamp::initialize()
{
	MStatus status;

	MFnEnumAttribute eAttr;
	Attribute::rampInterpolationMode = eAttr.create("rampInterpolationMode", "rinm", frw::InterpolationModeLinear);
	eAttr.addField("None", frw::InterpolationModeNone);
	eAttr.addField("Linear", frw::InterpolationModeLinear);
	MAKE_INPUT_CONST(eAttr);
	status = MPxNode::addAttribute(Attribute::rampInterpolationMode);
	CHECK_MSTATUS(status);

	Attribute::rampUVType = eAttr.create("rampUVType", "ruvt", VRamp);
	eAttr.addField("V Ramp", VRamp);
	eAttr.addField("U Ramp", URamp);
	eAttr.addField("Diagonal Ramp", DiagonalRamp);
	eAttr.addField("Circular Ramp", CircularRamp);
	MAKE_INPUT_CONST(eAttr);
	status = MPxNode::addAttribute(Attribute::rampUVType);
	CHECK_MSTATUS(status);

	MRampAttribute rAttr;
	Attribute::inputRamp = rAttr.createColorRamp("inputRamp", "rinp", &status);
	CHECK_MSTATUS(status);
	status = MPxNode::addAttribute(Attribute::inputRamp);
	CHECK_MSTATUS(status);

	MFnNumericAttribute nAttr;
	Attribute::output = nAttr.createPoint("out", "rout");
	MAKE_OUTPUT(nAttr);
	status = addAttribute(Attribute::output);
	CHECK_MSTATUS(status);

	status = attributeAffects(Attribute::inputRamp, Attribute::output);
	CHECK_MSTATUS(status);
	status = attributeAffects(Attribute::rampInterpolationMode, Attribute::output);
	CHECK_MSTATUS(status);
	status = attributeAffects(Attribute::rampUVType, Attribute::output);
	CHECK_MSTATUS(status);

	return MS::kSuccess;
}

void* FireMaya::RPRRamp::creator()
{
	return new RPRRamp;
}

// control point can be MColor OR it can be connection to other node; in this case we need to save both node and connection plug name
using CtrlPointDataT = std::tuple<MColor, MString, MObject>; 
using CtrlPointT = RampCtrlPoint<CtrlPointDataT>;

frw::Value FireMaya::RPRRamp::GetValue(const Scope& scope) const
{
	MFnDependencyNode shaderNode(thisMObject());

	// create node
	frw::RampNode rampNode(scope.MaterialSystem());

	// process ramp parameters
	frw::RampInterpolationMode mode = frw::InterpolationModeNone;

	MPlug plug = shaderNode.findPlug(Attribute::rampInterpolationMode, false);
	if (plug.isNull())
		return frw::Value();

	int temp = 0;
	if (MStatus::kSuccess == plug.getValue(temp))
		mode = static_cast<frw::RampInterpolationMode>(temp);

	rampNode.SetInterpolationMode(mode);

	// read input ramp
	std::vector<CtrlPointT> outRampCtrlPoints;
	MPlug ctrlPointsPlug = shaderNode.findPlug(Attribute::inputRamp, false);

	// - read simple ramp control point values
	bool isRampParced = GetRampValues<MColorArray>(ctrlPointsPlug, outRampCtrlPoints);
	if (!isRampParced)
		return frw::Value();

	// - read and process node ramp control point values
	bool success = GetConnectedCtrlPointsObjects(ctrlPointsPlug, outRampCtrlPoints);
	if (!success)
		return frw::Value();

	// - translate control points to RPR representation
	TranslateControlPoints(rampNode, scope, outRampCtrlPoints);

	// get proper lookup
	MPlug rampPlug = shaderNode.findPlug(Attribute::rampUVType, false);
	if (rampPlug.isNull())
		return frw::Value();

	temp = 1;
	RampUVType rampType = VRamp;
	if (MStatus::kSuccess == rampPlug.getValue(temp))
		rampType = static_cast<RampUVType>(temp);

	frw::ArithmeticNode lookupTree = GetRampNodeLookup(scope, rampType);
	rampNode.SetLookup(lookupTree);

	return rampNode;
}

void FireMaya::RPRRamp::postConstructor()
{
	// we now set default values at AERPRCreateDefaultRampColors, called from AERPRRampTemplate
	// setting them here causes some bugs related to overwriting black points and other strange problems

	ValueNode::postConstructor();
}
