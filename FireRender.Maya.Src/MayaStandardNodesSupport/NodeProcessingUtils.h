#pragma once

#include <functional>
namespace MayaStandardNodeConverters
{

struct ConverterParams;

// function to perform action for each element of array plug arrayPlug
// - use dataContainer to pass data
// - return false if detects error to execute or if passed action function returns false
template <typename T>
bool ForEachPlugInArrayPlug(MPlug& arrayPlug, T& dataContainer, std::function<bool(MPlug& elementPlug, T& dataContainer)> actionFunc)
{
	MStatus status;
	bool isArray = arrayPlug.isArray(&status);
	if ((status != MStatus::kSuccess) || (!isArray))
		return false;

	unsigned int count = arrayPlug.numElements();
	for (unsigned int arr_idx = 0; arr_idx < count; ++arr_idx)
	{
		MPlug rampElementPlug = arrayPlug[arr_idx];
		bool res = actionFunc(rampElementPlug, dataContainer);
		if (!res)
			return false;
	}

	return true;
}

// function to perform action for each element of compound plug compoundPlug
// - use dataContainer to pass data
// - return false if detects error or if passed action function returns false
template <typename T>
bool ForEachPlugInCompoundPlug(MPlug& compoundPlug, T& dataContainer, std::function<bool(MPlug& childPlug, T& dataContainer)> actionFunc)
{
	MStatus status;
	bool isCompound = compoundPlug.isCompound(&status);
	if ((status != MStatus::kSuccess) || (!isCompound))
		return false;

	unsigned int numChildren = compoundPlug.numChildren(&status);
	for (unsigned int child_idx = 0; child_idx < numChildren; ++child_idx)
	{
		MPlug childPlug = compoundPlug.child(child_idx, &status);
		if (status != MStatus::kSuccess)
			return false;

		bool res = actionFunc(childPlug, dataContainer);
		if (!res)
			return false;
	}

	return true;
}

frw::Value GetSamplerNodeForValue(const ConverterParams& params,
	const MString& plugName,
	frw::Value valueInput,
	frw::Value inputMin = 0.0,
	frw::Value inputMax = 1.0,
	frw::Value outputMin = 0.0,
	frw::Value outputMax = 1.0);
}

// note that we can have either MColor OR connected node as ramp control point data value
template <typename T>
void TranslateControlPoints(frw::RampNode& rampNode, const FireMaya::Scope& scope, const std::vector<T>& outRampCtrlPoints)
{
	// control points values; need to set them for both color and node inputs
	std::vector<float> ctrlPointsVals;
	ctrlPointsVals.reserve(outRampCtrlPoints.size() * 4);

	unsigned countMaterialInputs = 0; // necessary to pass this for RPRRampNode inputs logic

	// iterate through control points array
	for (const T& tCtrl : outRampCtrlPoints)
	{
		ctrlPointsVals.push_back(tCtrl.position);

		MObject ctrlPointConnectedObject = std::get<MObject>(tCtrl.ctrlPointData);
		if (ctrlPointConnectedObject == MObject::kNullObj)
		{
			// control point is a plain color and not a connected node
			const MColor& colorValue = std::get<MColor>(tCtrl.ctrlPointData);
			ctrlPointsVals.push_back(colorValue.r);
			ctrlPointsVals.push_back(colorValue.g);
			ctrlPointsVals.push_back(colorValue.b);
			continue;
		}

		// control point is a connected node and must be processed accordingly
		// - set color override values (so that RPR knows it must override color input with node input)
		ctrlPointsVals.push_back(-1.0f);
		ctrlPointsVals.push_back(-1.0f);
		ctrlPointsVals.push_back(-1.0f);

		// - add node input
		frw::Value materialNode = scope.GetValue(ctrlPointConnectedObject, std::get<MString>(tCtrl.ctrlPointData));
		rampNode.SetMaterialControlPointValue(countMaterialInputs++, materialNode);
	}

	rampNode.SetControlPoints(ctrlPointsVals.data(), ctrlPointsVals.size());
}
