/** 
* @file nvtopbarholder.cpp
* @brief LLSidebar class implementation
*
* $LicenseInfo:firstyear=2002&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2010, Linden Research, Inc.
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
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
* 
* Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
* $/LicenseInfo$
*/

#include "llviewerprecompiledheaders.h"

#include "bdsidebar.h"

// viewer includes
#include "llviewercontrol.h"
#include "llfloaterpreference.h"
#include "llsdserialize.h"

// system includes
#include <iomanip>

LLSidebar *gSidebar = NULL;

LLSidebar::LLSidebar()
:	LLPanel()
{
	buildFromFile("panel_machinima_sidebar.xml");

//	//BD - Vector4
	mCommitCallbackRegistrar.add("Pref.ArrayVec4X",			boost::bind(&LLFloaterPreference::onCommitVec4X, this, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayVec4Y",			boost::bind(&LLFloaterPreference::onCommitVec4Y, this, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayVec4Z",			boost::bind(&LLFloaterPreference::onCommitVec4Z, this, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayVec4W",			boost::bind(&LLFloaterPreference::onCommitVec4W, this, _1, _2));

//	//BD - Array Debugs
	mCommitCallbackRegistrar.add("Pref.ArrayX",				boost::bind(&LLFloaterPreference::onCommitX, this, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayY",				boost::bind(&LLFloaterPreference::onCommitY, this, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayZ",				boost::bind(&LLFloaterPreference::onCommitZ, this, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayXD",			boost::bind(&LLFloaterPreference::onCommitXd, this, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayYD",			boost::bind(&LLFloaterPreference::onCommitYd, this, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayZD",			boost::bind(&LLFloaterPreference::onCommitZd, this, _1, _2));

	mCommitCallbackRegistrar.add("Sidebar.CreateWidget",	boost::bind(&LLSidebar::onCreateWidget, this));
	mCommitCallbackRegistrar.add("Sidebar.ModifyWidget",	boost::bind(&LLSidebar::onModifyWidget, this));
	mCommitCallbackRegistrar.add("Sidebar.DeleteWidget",	boost::bind(&LLSidebar::onDeleteWidget, this));

	//BD - Creation Types
	mCommitCallbackRegistrar.add("Sidebar.SetCheckbox",		boost::bind(&LLSidebar::onClickCheckbox, this));
	mCommitCallbackRegistrar.add("Sidebar.SetRadio",		boost::bind(&LLSidebar::onClickRadio, this));
	mCommitCallbackRegistrar.add("Sidebar.SetDropdown",		boost::bind(&LLSidebar::onClickDropdown, this));
	mCommitCallbackRegistrar.add("Sidebar.SetTextbox",		boost::bind(&LLSidebar::onClickTextbox, this));
	mCommitCallbackRegistrar.add("Sidebar.SetSlider",		boost::bind(&LLSidebar::onClickSlider, this));
	mCommitCallbackRegistrar.add("Sidebar.SetTitle",		boost::bind(&LLSidebar::onClickTitle, this));
	mCommitCallbackRegistrar.add("Sidebar.SetButton",		boost::bind(&LLSidebar::onClickButton, this));
}

LLSidebar::~LLSidebar()
{
}

// virtual
void LLSidebar::draw()
{
	LLPanel::draw();
}

BOOL LLSidebar::postBuild()
{


	return TRUE;
}

//void LLFloaterPreference::onBindKey(KEY key, MASK mask, LLUICtrl* ctrl, const LLSD& param)
void LLSidebar::onCreateWidget()
{
	//BD - 0 = Checkbox
	//     1 = Textbox
	//     2 = Slider
	//     3 = Radio Buttons
	//     4 = Dropdown
	//     5 = Button
	//     6 = Title
	S32 ctrl_type;
	std::string ctrl_debug;
	std::string ctrl_label;
	F32 ctrl_min_val;
	F32 ctrl_max_val;
	S32 ctrl_decimals;

	LLSD record;
	llifstream infile;
	llofstream file;
	std::string temp_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "temp_sidebar.xml");
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "sidebar.xml");

	file.open(temp_filename.c_str());
	infile.open(filename);
	if (!infile.is_open())
	{
		LL_WARNS("Settings") << "Cannot find file " << filename << " to load." << LL_ENDL;
		return;
	}
	while (LLSDParser::PARSE_FAILURE != LLSDSerialize::fromXML(record, infile))
	{
		if (infile.eof())
		{
			record["type"] = ctrl_type;
			record["debug"] = ctrl_debug;
			record["label"] = ctrl_label;
			record["min"] = ctrl_min_val;
			record["max"] = ctrl_max_val;
			record["decimals"] = ctrl_decimals;
			LLSDSerialize::toXML(record, file);
			LL_INFOS() << "Created: " << ctrl_type + " "
				<< ctrl_debug << " (" 
				<< ctrl_min_val << + " , " << ctrl_max_val << " : "
				<< ctrl_decimals + ") " 
				<< ctrl_label << LL_ENDL;
			break;
		}
		else
		{
			LLSDSerialize::toXML(record, file);
			LL_INFOS() << "Kept: " << record["type"].asInteger() + " "
				<< record["debug"].asString() << " ("
				<< record["min"].asReal() << + " , " << record["max"].asReal() << " : "
				<< record["decimals"].asInteger() + ") "
				<< record["label"].asString() << LL_ENDL;
		}
	}
	infile.close();
	file.close();
	//BD - We can now safely rewrite the entire file now that we filled all bindings in.
	//onExportControls();
}

void LLSidebar::onModifyWidget()
{
}

void LLSidebar::onDeleteWidget()
{
	//onBindKey(NULL, MASK_NONE, ctrl, param);
}

//BD - Custom Keyboard Layout
/*BOOL LLViewerKeyboard::exportBindingsXML(const std::string& filename)
{
	S32 slot = 0;
	llofstream file;
	//LL_INFOS("Settings") << "Control settings path: " << filename << "" << LL_ENDL;

	//BD - Open the file and go through all modes, while in all modes go through all
	//     bindings and write them into the file.
	//     We need to rewrite the entire file due to toXML()'s limitations and to prevent
	//     bad things from happening.
	file.open(filename.c_str());
	for (S32 i = 0; i < 5; i++)
	{
		for (S32 it = 0, end_it = mBindingCount[i]; it < end_it; it++)
		{
			KEY key = mBindings[i][it].mKey;
			MASK mask = mBindings[i][it].mMask;
			LLSD record;
			record["function"] = mBindings[i][it].mFunctionName;
			record["key"] = gKeyboard->stringFromKey(key, false);
			record["mode"] = i;
			record["mask"] = gKeyboard->stringFromMask(mask);
			record["slot"] = slot;

			LLSDSerialize::toXML(record, file);
			slot++;
			//LL_INFOS() << "Exported: " << key << +"(" + record["key"].asString() << ") + " 
			//			<< mask << +"(" << record["mask"].asString() << ") to " 
			//			<< record["function"] << " in mode " 
			//			<< i << LL_ENDL;
		}
	}
	return true;
}

//BD - Custom Keyboard Layout
S32 LLViewerKeyboard::loadBindingsSettings(const std::string& filename)
{
	LLSD settings;
	llifstream infile;

	infile.open(filename);
	if (!infile.is_open())
	{
		LL_WARNS("Settings") << "Cannot find file " << filename << " to load." << LL_ENDL;
		return FALSE;
	}

	//BD - This is used for loading the default bindings from the local Viewer foldler.
	while (!infile.eof() && LLSDParser::PARSE_FAILURE != LLSDSerialize::fromXML(settings, infile))
	{
		KEY key = NULL;
		MASK mask = MASK_NONE;
		S32 mode = settings["mode"].asInteger();
		std::string function = settings["function"].asString();

		LLKeyboard::keyFromString(settings["key"], &key);
		LLKeyboard::maskFromString(settings["mask"], &mask);
		bindKey(mode, key, mask, function);
		//LL_INFOS() << "Setting: " << key << +"(" + settings["key"].asString() << ") + " 
		//			<< mask << +"(" << settings["mask"].asString() << ") to " 
		//			<< function << " in mode " 
		//			<< mode << LL_ENDL;
	}
	return TRUE;
}*/

/*BOOL LLViewerKeyboard::bindKey(const S32 mode, const KEY key, const MASK mask, const std::string& function_name)
{
	typedef boost::function<void(EKeystate)> function_t;
	function_t function = NULL;
	LLSD binds;

	// Allow remapping of F2-F12
	if (function_name[0] == 'F')
	{
		int c1 = function_name[1] - '0';
		int c2 = function_name[2] ? function_name[2] - '0' : -1;
		if (c1 >= 0 && c1 <= 9 && c2 >= -1 && c2 <= 9)
		{
			int idx = c1;
			if (c2 >= 0)
				idx = idx * 10 + c2;
			if (idx >= 2 && idx <= 12)
			{
				U32 keyidx = ((mask << 16) | key);
				(mRemapKeys[mode])[keyidx] = ((0 << 16) | (KEY_F1 + (idx - 1)));
				return TRUE;
			}
		}
	}

	// Not remapped, look for a function
	function_t* result = LLKeyboardActionRegistry::getValue(function_name);
	if (result)
	{
		function = *result;
	}

	if (!function)
	{
		LL_WARNS() << "Can't bind key to function " << function_name << ", no function with this name found" << LL_ENDL;
		return FALSE;
	}

	if (mBindingCount[mode] >= MAX_KEY_BINDINGS)
	{
		LL_WARNS() << "LLKeyboard::bindKey() - too many keys for mode " << mode << LL_ENDL;
		return FALSE;
	}

	if (mode >= MODE_COUNT)
	{
		LL_WARNS() << "LLKeyboard::bindKey() - unknown mode passed" << mode << LL_ENDL;
		return FALSE;
	}

	mBindings[mode][mBindingCount[mode]].mKey = key;
	mBindings[mode][mBindingCount[mode]].mMask = mask;
	mBindings[mode][mBindingCount[mode]].mFunction = function;
	mBindings[mode][mBindingCount[mode]].mFunctionName = function_name;
	mBindingCount[mode]++;

	return TRUE;
}

//BD - Custom Keyboard Layout
BOOL LLViewerKeyboard::unbindAllKeys(bool reset)
{
	for (S32 i = 0; i < 5; i++)
	{
		for (S32 it = 0, end_it = mBindingCount[i]; it < end_it; it++)
		{
			mBindings[i][it].mKey = NULL;
			mBindings[i][it].mMask = NULL;
		}

		//BD -  We need to seperate this to prevent evil things from happening.
		if (reset)
		{
			mBindingCount[i] = 0;
		}
	}

	return TRUE;
}*/

void LLSidebar::onMouseEnter(S32 x, S32 y, MASK mask)
{
	LLPanel::onMouseEnter(x, y, mask);
}

void LLSidebar::onMouseLeave(S32 x, S32 y, MASK mask)
{
	LLPanel::onMouseLeave(x, y, mask);
} 

BOOL LLSidebar::handleMouseDown(S32 x, S32 y, MASK mask)
{
	return TRUE;
}