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

#ifndef LL_BDSIDEBAR_H
#define LL_BDSIDEBAR_H

#include "llpanel.h"
#include "lluictrl.h"
//#include "llsliderctrl.h"
//#include "lllayoutstack.h"
//#include "llbutton.h"
//#include "lllineeditor.h"
//#include "llcombobox.h"

class LLUICtrl;
class LLSliderCtrl;
//class LLLayoutPanel;
//class LLButton;
//class LLComboBox;
//class LLLineEditor;
//class LLPanel;
//class LLLayoutStack;

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

	/*class BDSidebarItem
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
			, mPanel(NULL)
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
			SLIDER = 2,
			TITLE = 3,
			TAB = 4
		};

		std::string		mPanelName;

		LLLayoutPanel*	mPanel;

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

		LLButton*		mRemoveBtn;

		SidebarType		mType;
	};*/

private:
	//BD - Legacy stuff.
	void onMouseLeave(S32 x, S32 y, MASK mask);
	void onMouseEnter(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL 	handleMouseDown(S32 x, S32 y, MASK mask);

	LLUICtrl*					mShadowResX;
	LLUICtrl*					mShadowResY;
	LLUICtrl*					mShadowResZ;
	LLUICtrl*					mShadowResW;

	LLUICtrl*					mShadowDistX;
	LLUICtrl*					mShadowDistY;
	LLUICtrl*					mShadowDistZ;
	LLUICtrl*					mShadowDistW;

	LLUICtrl*					mProjectorResX;
	LLUICtrl*					mProjectorResY;

	LLUICtrl*					mVignetteX;
	LLUICtrl*					mVignetteY;
	LLUICtrl*					mVignetteZ;

	LLSliderCtrl*				mCameraAngle;


	//BD - New stuff.
	/*bool loadWidgetList();
	void saveWidgetList();
	void createWidget();
	void deleteWidget(LLUICtrl* ctrl);
	void toggleEditMode();

	void onTypeSelection();

	void refreshCreationControls();

	void onAddTest();
	void onDeleteTest();

	S32							mOffset;
	S32							mWidgetCount;*/

	LLFrameTimer				mUpdateTimer;

	/*std::vector<BDSidebarItem*> mSidebarItems;
	std::vector<LLLayoutPanel*>	mSidebarPanels;
	LLLayoutStack*				mTestStack;

	//BD - General stuff.
	LLButton*					mIsCheckbox;
	LLButton*					mIsSlider;
	LLButton*					mIsRadio;
	LLButton*					mIsTitle;
	LLButton*					mIsTab;

	LLPanel*					mScrollPanel;
	LLLayoutStack*				mWidgetsStack;
	LLLayoutPanel*				mMainLayout;

	LLComboBox*					mDebugCombo;
	LLLineEditor*				mLabelEditor;
	LLButton*					mNextBtn;

	//BD - Slider stuff.
	LLLineEditor*				mDecimalsEditor;
	LLLineEditor*				mIncrementEditor;
	LLLineEditor*				mMinEditor;
	LLLineEditor*				mMaxEditor;
	LLButton*					mIsX;
	LLButton*					mIsY;
	LLButton*					mIsZ;
	LLButton*					mIsW;

	//BD - Radio stuff.
	LLButton* mRadio2;
	LLButton* mRadio3;
	LLButton* mRadio4;
	LLUICtrl* mRadioCount;
	LLLineEditor* mRadioVal1;
	LLLineEditor* mRadioVal2;
	LLLineEditor* mRadioVal3;
	LLLineEditor* mRadioVal4;
	LLLineEditor* mRadioLabel1;
	LLLineEditor* mRadioLabel2;
	LLLineEditor* mRadioLabel3;
	LLLineEditor* mRadioLabel4;

	bool						mEditMode;
	bool						mFirstTime;*/
};

extern LLSideBar *gSideBar;

#endif
