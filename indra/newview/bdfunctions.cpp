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

#include "llfloaterreg.h"
#include "llfloaterpreference.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llnotificationsutil.h"
#include "llnotifications.h"
#include "llstartup.h"

#include "message.h"

BDFunctions gDragonLibrary;


BDFunctions::BDFunctions()
	: mAllowWalkingBackwards(TRUE)
	, mAvatarRotateThresholdSlow(2.f)
	, mAvatarRotateThresholdFast(2.f)
	, mAvatarRotateThresholdMouselook(120.f)
	, mMovementRotationSpeed(0.2f)
	, mUseFreezeWorld(FALSE)
	, mDebugAvatarRezTime(FALSE)
{
}

BDFunctions::~BDFunctions()
{
}

void BDFunctions::initializeControls()
{
	mAllowWalkingBackwards = gSavedSettings.getBOOL("AllowWalkingBackwards");
	mAvatarRotateThresholdSlow = gSavedSettings.getF32("AvatarRotateThresholdSlow");
	mAvatarRotateThresholdFast = gSavedSettings.getF32("AvatarRotateThresholdFast");
	mAvatarRotateThresholdMouselook = gSavedSettings.getF32("AvatarRotateThresholdMouselook");
	mMovementRotationSpeed = gSavedSettings.getF32("MovementRotationSpeed");

	mUseFreezeWorld = gSavedSettings.getBOOL("UseFreezeWorld");

	mDebugAvatarRezTime = gSavedSettings.getBOOL("DebugAvatarRezTime");
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

	//BD - Trigger Warning System
	triggerWarning(ctrl, param);
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

	//BD - Trigger Warning System
	triggerWarning(ctrl, param);
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

	//BD - Trigger Warning System
	gDragonLibrary.triggerWarning(ctrl, param);
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

	//BD - Trigger Warning System
	triggerWarning(ctrl, param);
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

//BD - Trigger Warning System
void BDFunctions::triggerWarning(LLUICtrl* ctrl, const LLSD& param)
{
	LLSD val = ctrl->getValue();
	LLControlVariable* control = ctrl->getControlVariable();
	//BD - Debug was not found, maybe it is an account setting.
	if (!control)
	{
		control = ctrl->getControlVariable();

		//BD - Still not found, check whether we can get the debug via the param.
		if (!control)
		{
			control = gSavedSettings.getControl(param.asString());

			//BD - Debug was not found, maybe it is an account setting.
			if (!control)
			{
				control = gSavedPerAccountSettings.getControl(param.asString());

				//BD - Give up.
				if (!control) return;
			}
		}
	}

	eControlType type = control->type();
	std::string name = control->getName();
	F32 max_val = control->getMaxValue();
	F32 min_val = control->getMinValue();

	//BD - We name all our warnings by the control's name, this makes it super easy
	//     to distinguish which warning triggers for which control and when, this also
	//     plays well into the fact that certain widgets will have other functions already
	//     present such as Vector sliders which use the control name in its parameter.
	if (type == TYPE_BOOLEAN)
	{
		if (val.asBoolean())
		{
			LLNotificationsUtil::add(name);
			//BD - I don't think these one-off options should be muted after the first sight.
			//     keep pestering the user that the option they enabled is up to no good.
			//LLNotifications::instance().setIgnored(name, true);
		}
	}
	//else if (type == TYPE_S32 || type == TYPE_U32 || type == TYPE_F32)
	else if (type == TYPE_VEC2 || type == TYPE_VEC3 || type == TYPE_VEC3D || type == TYPE_VEC4
			|| type == TYPE_S32 || type == TYPE_U32 || type == TYPE_F32)
	{
		if (val.asReal() > max_val)
		{
			LLNotificationsUtil::add(name + "_Max");
			//BD - Don't spam the user every step.
			LLNotifications::instance().setIgnored(name + "_Max", true);
			LL_WARNS("Notifications") << "Value set to: " << val.asReal() << " but maximum allowed is: " << max_val << LL_ENDL;
		}
		else if (val.asReal() < min_val)
		{
			LLNotificationsUtil::add(name + "_Min");
			//BD - Don't spam the user every step.
			LLNotifications::instance().setIgnored(name + "_Min", true);
			LL_WARNS("Notifications") << "Value set to: " << val.asReal() << " but minimum allowed is: " << min_val << LL_ENDL;
		}
	}
}

//BD - Trigger Warning System
void BDFunctions::openPreferences( const LLSD& param)
{
	LLFloaterReg::toggleInstanceOrBringToFront("preferences");
	LLFloaterPreference* preferences = LLFloaterReg::getTypedInstance<LLFloaterPreference>("preferences");
	if (preferences && preferences->getVisible() && !preferences->isMinimized())
	{
		LLTabContainer* prefs_tabs = preferences->getChild<LLTabContainer>("pref core");
		if (prefs_tabs)
		{
			prefs_tabs->selectTabByName(param);
		}
	}
}

//BD - Factory Reset
void BDFunctions::askFactoryReset(const LLSD& param)
{
	LLNotificationsUtil::add(param,	LLSD(),	LLSD(),	&doFactoryReset);
}

void BDFunctions::doFactoryReset(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	//BD - Cancel
	if (option == 1)
	{
		return;
	}

	LL_WARNS("Notifications") << "ABANDON SHIP. We are resetting the Viewer to stock." << LL_ENDL;
	gSavedSettings.doFactoryReset();
	//BD - Do not allow resetting personal settings before we've logged in.
	if (LLStartUp::getStartupState() == STATE_STARTED)
	{
		gSavedPerAccountSettings.doFactoryReset();
	}
}