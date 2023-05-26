#pragma once

#include <functional>
//#include "MayaStandardNodesSupport/RampNodeConverter.h"
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

// this is called for every element of Ramp control points array;
// in Maya control points of the Ramp attribute are stored as an array attribute
template <typename T>
bool ProcessRampArrayPlugElement(MPlug& elementPlug, T& out)
{
	static_assert(std::is_same<typename std::remove_reference<decltype(out)>::type, typename std::vector<CtrlPointT>::iterator>::value, "data container type mismatch!");

	bool success = ForEachPlugInCompoundPlug<CtrlPointDataT>(elementPlug, out->ctrlPointData, ProcessCompundPlugElement<CtrlPointDataT>);

	out++;

	return success;
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

// this is called for every element of compound plug of a Ramp attribute;
// in Maya a single control point of a Ramp attribute is represented as a compound plug;
// thus Ramp attribute is an array of compound plugs
template <typename T>
bool ProcessCompundPlugElement(MPlug& childPlug, T& out)
{
	static_assert(std::is_same<CtrlPointDataT, T>::value, "data container type mismatch!");

	// find color input plug
	std::string elementName = childPlug.name().asChar();
	if (elementName.find("inputRamp_Color") == std::string::npos)
		return true; // this is not element that we are looking for => caller will continue processing elements of compound plug

	// get connections
	MStatus status;
	MPlugArray connections;
	bool connectedTo = childPlug.connectedTo(connections, true, true, &status);
	if (!connectedTo)
	{
		std::get<MString>(out) = "";
		std::get<MObject>(out) = MObject::kNullObj;

		return true; // no connections found
	}

	// color input has connected node => save its name and object to output
	assert(connections.length() == 1);

	for (auto& it : connections)
	{
		// save connected node
		std::get<MObject>(out) = it.node();

		// get connected node output name (is needed to setup proper connection for material node tree)
		std::string attrName = it.name().asChar();
		MFnDependencyNode depN(it.node());
		std::string connectedNodeName = depN.name().asChar();
		connectedNodeName += ".";
		attrName.erase(attrName.find(connectedNodeName), connectedNodeName.length());

		MPlug outPlug = depN.findPlug(attrName.c_str(), &status);
		assert(status == MStatus::kSuccess);
		assert(!outPlug.isNull());
		std::string plugName = outPlug.partialName().asChar();

		// save connected node output name
		std::get<MString>(out) = plugName.c_str();
	}

	return true; // connection found and processed
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

// iterate through all control points of the Ramp and save connected nodes data to corresponding control points array entries if such nodes exist
template <typename RampCtrlPointDataT>
bool GetConnectedCtrlPointsObjects(MPlug& rampPlug, std::vector<RampCtrlPointDataT>& rampCtrlPoints)
{
	MStatus status;

	// ensure valid input
	bool isArray = rampPlug.isArray(&status);
	assert(isArray);
	if (!isArray)
		return false;

	bool doArraysMatch = rampCtrlPoints.size() == rampPlug.numElements();
	if (!doArraysMatch)
	{
		rampCtrlPoints.pop_back(); // remove "technical" control point created by helper if necessary
	}
	doArraysMatch = rampCtrlPoints.size() == rampPlug.numElements();
	assert(doArraysMatch);
	if (!doArraysMatch)
		return false;

	// iterate through control points; passing iterator as a data container here to avoid extra unnecessary data copy
	auto currCtrlPointIt = rampCtrlPoints.begin();
	using containerIterT = decltype(currCtrlPointIt);
	static_assert(std::is_same<containerIterT, typename std::vector<RampCtrlPointDataT>::iterator>::value, "data container type mismatch!");

	bool success = MayaStandardNodeConverters::ForEachPlugInArrayPlug<containerIterT>(rampPlug, currCtrlPointIt, MayaStandardNodeConverters::ProcessRampArrayPlugElement<containerIterT>);
	assert(success);
	assert(currCtrlPointIt == rampCtrlPoints.end());

	return success;
}
