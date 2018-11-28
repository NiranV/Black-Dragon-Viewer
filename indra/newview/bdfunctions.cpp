/**
*
* Copyright (C) 2018, NiranV Dean
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
*/

#include "llviewerprecompiledheaders.h"

#include "bdfunctions.h"

#include "lluictrlfactory.h"
#include "llviewercontrol.h"

#include "message.h"

BDFunctions gDragonLibrary;


BDFunctions::BDFunctions()
{
}

BDFunctions::~BDFunctions()
{
}

//BD - Array Debugs
void BDFunctions::onCommitX(LLUICtrl* ctrl, const LLSD& param)
{
	LLControlVariable* control = gSavedSettings.getControl(param.asString());
	eControlType type = control->type();
	LLVector4 vec4 = LLVector4(control->getValue());
	F32 value = ctrl->getValue().asReal();
	//BD - Option is in link mode.
	if (control->isLocked())
	{
		//BD - Attempt to find the other controls.
		//     Usually we name them control_name + array (e.g RenderDebug_X)
		LLView* parent_view = ctrl->getParent();
		if (parent_view)
		{
			LLUICtrl* other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_Y");
			if (other_ctrl)
			{
				other_ctrl->setValue(value);
			}

			//BD - Don't even attempt to find Z or W if our control is not a vector3 or vector4
			//     respectively. Nothing bad happens if we do this but we don't need a pointless
			//     UI warning either.
			if (type == TYPE_VEC3 || type == TYPE_VEC3D
				|| type == TYPE_VEC4)
			{
				other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_Z");
				if (other_ctrl)
				{
					other_ctrl->setValue(value);
				}
			}

			if (type == TYPE_VEC4)
			{
				other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_W");
				if (other_ctrl)
				{
					other_ctrl->setValue(value);
				}
			}
		}

		vec4.mV[VY] = value;
		vec4.mV[VZ] = value;
		vec4.mV[VW] = value;
	}
	vec4.mV[VX] = value;
	gSavedSettings.setUntypedValue(param.asString(), vec4.getValue());
}

void BDFunctions::onCommitY(LLUICtrl* ctrl, const LLSD& param)
{
	LLControlVariable* control = gSavedSettings.getControl(param.asString());
	eControlType type = control->type();
	LLVector4 vec4 = LLVector4(control->getValue());
	F32 value = ctrl->getValue().asReal();
	//BD - Option is in link mode.
	if (control->isLocked())
	{
		//BD - Attempt to find the other controls.
		//     Usually we name them control_name + array (e.g RenderDebug_X)
		LLView* parent_view = ctrl->getParent();
		if (parent_view)
		{
			LLUICtrl* other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_X");
			if (other_ctrl)
			{
				other_ctrl->setValue(value);
			}

			//BD - Don't even attempt to find Z or W if our control is not a vector3 or vector4
			//     respectively. Nothing bad happens if we do this but we don't need a pointless
			//     UI warning either.
			if (type == TYPE_VEC3 || type == TYPE_VEC3D
				|| type == TYPE_VEC4)
			{
				other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_Z");
				if (other_ctrl)
				{
					other_ctrl->setValue(value);
				}
			}

			if (type == TYPE_VEC4)
			{
				other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_W");
				if (other_ctrl)
				{
					other_ctrl->setValue(value);
				}
			}
		}

		vec4.mV[VX] = value;
		vec4.mV[VZ] = value;
		vec4.mV[VW] = value;
	}
	vec4.mV[VY] = value;
	gSavedSettings.setUntypedValue(param.asString(), vec4.getValue());
}

void BDFunctions::onCommitZ(LLUICtrl* ctrl, const LLSD& param)
{
	LLControlVariable* control = gSavedSettings.getControl(param.asString());
	eControlType type = control->type();
	LLVector4 vec4 = LLVector4(control->getValue());
	F32 value = ctrl->getValue().asReal();
	//BD - Option is in link mode.
	if (control->isLocked())
	{
		//BD - Attempt to find the other controls.
		//     Usually we name them control_name + array (e.g RenderDebug_X)
		LLView* parent_view = ctrl->getParent();
		if (parent_view)
		{
			LLUICtrl* other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_X");
			if (other_ctrl)
			{
				other_ctrl->setValue(value);
			}

			other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_Y");
			if (other_ctrl)
			{
				other_ctrl->setValue(value);
			}

			//BD - Don't even attempt to find W if our control is not a vector4. Nothing bad 
			//     happens if we do this but we don't need a pointless UI warning either.
			if (type == TYPE_VEC4)
			{
				other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_W");
				if (other_ctrl)
				{
					other_ctrl->setValue(value);
				}
			}
		}

		vec4.mV[VX] = value;
		vec4.mV[VY] = value;
		vec4.mV[VW] = value;
	}
	vec4.mV[VZ] = value;
	gSavedSettings.setUntypedValue(param.asString(), vec4.getValue());
}

//BD - Vector4
void BDFunctions::onCommitW(LLUICtrl* ctrl, const LLSD& param)
{
	LLControlVariable* control = gSavedSettings.getControl(param.asString());
	LLVector4 vec4 = LLVector4(control->getValue());
	F32 value = ctrl->getValue().asReal();
	//BD - Option is in link mode.
	if (control->isLocked())
	{
		//BD - Attempt to find the other controls.
		//     Usually we name them control_name + array (e.g RenderDebug_X)
		LLView* parent_view = ctrl->getParent();
		if (parent_view)
		{
			LLUICtrl* other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_X");
			if (other_ctrl)
			{
				other_ctrl->setValue(value);
			}

			other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_Y");
			if (other_ctrl)
			{
				other_ctrl->setValue(value);
			}

			other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_Z");
			if (other_ctrl)
			{
				other_ctrl->setValue(value);
			}
		}

		vec4.mV[VX] = value;
		vec4.mV[VY] = value;
		vec4.mV[VZ] = value;
	}
	vec4.mV[VW] = value;
	gSavedSettings.setVector4(param.asString(), vec4);
}

//BD - Array Lock feature
void BDFunctions::onControlLock(LLUICtrl* ctrl, const LLSD& param)
{
	LLControlVariable* control = gSavedSettings.getControl(param.asString());
	control->setLocked(ctrl->getValue().asBoolean());
}

//BD - Revert to Default
BOOL BDFunctions::resetToDefault(LLUICtrl* ctrl)
{
	LLControlVariable* control = ctrl->getControlVariable();
	if (control)
	{
		control->resetToDefault(true);
		return TRUE;
	}
	return FALSE;
}

//BD - Revert to Default
void BDFunctions::invertValue(LLUICtrl* ctrl)
{
	LLControlVariable* control = ctrl->getControlVariable();
	F32 val;
	//BD - If we find a debug setting linked to the UI widget, change that.
	//     Should we find no debug setting linked we'll change the widget value it will hopefully
	//     trigger whatever is necessary to apply the change or use the new value.
	if (control)
	{
		val = control->getValue().asReal();
		control->setValue(-val);
	}
	else
	{
		val = ctrl->getValue().asReal();
		ctrl->setValue(-val);
	}
}

//BD - Escape string
std::string BDFunctions::escapeString(const std::string& str)
{
	//BD - Don't use LLURI::escape() because it doesn't encode '-' characters
	//     which may break handling of some poses.
	char* curl_str = curl_escape(str.c_str(), str.size());
	std::string escaped_str(curl_str);
	curl_free(curl_str);

	return escaped_str;
}