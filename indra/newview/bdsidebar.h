/** 
 * @file bdsidebar.h
 * @brief LLSideBar class definition
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

#ifndef LL_BDSIDEBAR_H
#define LL_BDSIDEBAR_H

#include "llpanel.h"
#include "lluictrl.h"
#include "llsliderctrl.h"
#include "lllayoutstack.h"

class LLUICtrl;
class LLSliderCtrl;
class LLLayoutPanel;

class LLSideBar
:	public LLPanel
{
public:
	LLSideBar(const LLRect& rect);
	/*virtual*/ ~LLSideBar();
	
	/*virtual*/ void draw();

	/*virtual*/ BOOL postBuild();

	void refreshGraphicControls();
	void setVisibleForMouselook(bool visible);

	class BDSidebarItem
	{
	public:
		BDSidebarItem()
			: mPanelName("")
			, mOrder(0)
			, mMaxValue(1.f)
			, mMinValue(0.f)
			, mIncrement(0.1f)
			, mDecimals(1)
			, mIsToggle(true)
			, mDebugSetting("")
			//, mParameter()
			//, mFunction()
			//, mPanel(NULL)
			, mType(SLIDER)

			, mRadioCount(4)

			, mLabel("Label")
			, mRadioLabel1("Radio 1")
			, mRadioLabel2("Radio 2")
			, mRadioLabel3("Radio 3")
			, mRadioLabel4("Radio 4")

			, mRadioValues(LLVector4(0,0,0,0))
		{}

		enum SidebarType
		{
			CHECKBOX = 0,
			RADIO = 1,
			SLIDER = 2
		};

		std::string		mPanelName;

		S32				mOrder;

		F32				mMaxValue;
		F32				mMinValue;
		F32				mIncrement;
		S32				mDecimals;

		S32				mRadioCount;

		std::string		mLabel;
		std::string		mRadioLabel1;
		std::string		mRadioLabel2;
		std::string		mRadioLabel3;
		std::string		mRadioLabel4;

		LLVector4		mRadioValues;

		//BD - 0 = X
		//	   1 = Y
		//     2 = Z
		//     3 = W
		S32				mArray;

		bool			mIsToggle;

		std::string		mDebugSetting;
		//const LLSD&		mParameter;
		//const LLSD&		mFunction;

		//LLPanel*		mPanel;

		SidebarType		mType;
	};

	S32							mWidgetCount;

	std::vector<BDSidebarItem*> mSidebarItems;
private:

	//void onClickAddWidget();
	void loadWidgetList();
	void saveWidgetList();
	void createWidget();
	void deleteWidget(LLUICtrl* ctrl);
	void toggleEditMode();

	void onClickNext();
	void onClickPrevious();

	void onTypeSelection(LLUICtrl* ctrl, const LLSD& param);

	void onMouseLeave(S32 x, S32 y, MASK mask);
	void onMouseEnter(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL 	handleMouseDown(S32 x, S32 y, MASK mask);

	LLUICtrl*			mShadowResX;
	LLUICtrl*			mShadowResY;
	LLUICtrl*			mShadowResZ;
	LLUICtrl*			mShadowResW;

	LLUICtrl*			mShadowDistX;
	LLUICtrl*			mShadowDistY;
	LLUICtrl*			mShadowDistZ;
	LLUICtrl*			mShadowDistW;

	LLUICtrl*			mProjectorResX;
	LLUICtrl*			mProjectorResY;

	LLUICtrl*			mVignetteX;
	LLUICtrl*			mVignetteY;
	LLUICtrl*			mVignetteZ;

	LLSliderCtrl*		mCameraAngle;

	bool				mEditMode;

	//LLLayoutPanel*		mLayoutPanel;

	std::string			mCurrentPage;
};

extern LLSideBar *gSideBar;

#endif
