#include "FireRenderArithmetic.h"
#include "FireRenderMaterial.h"

#include <maya/MFnEnumAttribute.h>
#include <maya/MFloatVector.h>

namespace
{
	namespace Attribute
	{
		MObject	inputA;
		MObject	inputB;
		MObject operation;

		MObject	output;
	}
}


MStatus FireMaya::Arithmetic::initialize()
{
	MFnNumericAttribute nAttr;
	MFnEnumAttribute eAttr;

	Attribute::inputA = nAttr.createPoint("inputA", "a");
	MAKE_INPUT(nAttr);
	Attribute::inputB = nAttr.createPoint("inputB", "b");
	MAKE_INPUT(nAttr);

	Attribute::operation = eAttr.create("operation", "fn", frw::OperatorAdd);

	eAttr.addField("Add", frw::OperatorAdd);
	eAttr.addField("Subtract", frw::OperatorSubtract);
	eAttr.addField("Multiply", frw::OperatorMultiply);
	eAttr.addField("Divide", frw::OperatorDivide);
	eAttr.addField("Absolute", frw::OperatorAbs);
	eAttr.addField("Magnitude", frw::OperatorLength);
	eAttr.addField("Pow", frw::OperatorPow);
	eAttr.addField("Mix", frw::OperatorAverage);

	eAttr.addField("Dot Product", frw::OperatorDot);
	eAttr.addField("Cross Product", frw::OperatorCross);

	eAttr.addField("Sin", frw::OperatorSin);
	eAttr.addField("Cos", frw::OperatorCos);
	eAttr.addField("Tan", frw::OperatorTan);
	eAttr.addField("ArcSin", frw::OperatorArcSin);
	eAttr.addField("ArcCos", frw::OperatorArcCos);
	eAttr.addField("ArcTan", frw::OperatorArcTan);

	eAttr.addField("Select X", frw::OperatorSelectX);
	eAttr.addField("Select Y", frw::OperatorSelectY);
	eAttr.addField("Select Z", frw::OperatorSelectZ);

	eAttr.addField("Combine", frw::OperatorCombine);
	eAttr.addField("Brightness", frw::OperatorComponentAverage);

	MAKE_INPUT_CONST(eAttr);

	Attribute::output = nAttr.createPoint("out", "o");
	MAKE_OUTPUT(nAttr);

	CHECK_MSTATUS(addAttribute(Attribute::inputA));
	CHECK_MSTATUS(addAttribute(Attribute::inputB));
	CHECK_MSTATUS(addAttribute(Attribute::operation));
	CHECK_MSTATUS(addAttribute(Attribute::output));

	CHECK_MSTATUS(attributeAffects(Attribute::inputA, Attribute::output));
	CHECK_MSTATUS(attributeAffects(Attribute::inputB, Attribute::output));
	CHECK_MSTATUS(attributeAffects(Attribute::operation, Attribute::output));

	return MS::kSuccess;
}


void* FireMaya::Arithmetic::creator()
{
	return new Arithmetic;
}

frw::Value FireMaya::Arithmetic::GetValue(Scope& scope)
{
	MFnDependencyNode shaderNode(thisMObject());

	frw::Operator op = frw::OperatorAdd;

	MPlug plug = shaderNode.findPlug(Attribute::operation);
	if (!plug.isNull())
	{
		int n = 0;
		if (MStatus::kSuccess == plug.getValue(n))
			op = static_cast<frw::Operator>(n);
	}

	auto a = scope.GetValue(shaderNode.findPlug(Attribute::inputA));
	auto b = scope.GetValue(shaderNode.findPlug(Attribute::inputB));

	frw::ArithmeticNode valueNode(scope.MaterialSystem(), op, a, b);

	return valueNode;
}
