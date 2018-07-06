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


#ifndef BD_FLOATER_POSER_H
#define BD_FLOATER_POSER_H

#include "llfloater.h"
#include "llscrolllistctrl.h"
#include "llsliderctrl.h"
//#include "llmultisliderctrl.h"
//#include "lltimectrl.h"
#include "llkeyframemotion.h"
#include "llframetimer.h"

/*struct BDPoseKey
{
public:
	// source of a pose set
	std::string name;

	// for conversion from LLSD
	static const int NAME_IDX = 0;
	static const int SCOPE_IDX = 1;

	inline BDPoseKey(const std::string& n)
		: name(n)
	{
	}

	inline BDPoseKey(LLSD llsd)
		: name(llsd[NAME_IDX].asString())
	{
	}

	inline BDPoseKey() // NOT really valid, just so std::maps can return a default of some sort
		: name("")
	{
	}

	inline BDPoseKey(std::string& stringVal)
	{
		size_t len = stringVal.length();
		if (len > 0)
		{
			name = stringVal.substr(0, len - 1);
		}
	}

	inline std::string toStringVal() const
	{
		std::stringstream str;
		str << name;
		return str.str();
	}

	inline LLSD toLLSD() const
	{
		LLSD llsd = LLSD::emptyArray();
		llsd.append(LLSD(name));
		return llsd;
	}

	inline void fromLLSD(const LLSD& llsd)
	{
		name = llsd[NAME_IDX].asString();
	}

	inline bool operator <(const BDPoseKey other) const
	{
		if (name < other.name)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	inline bool operator ==(const BDPoseKey other) const
	{
		return (name == other.name);
	}

	std::string toString() const;
};*/



typedef enum E_Columns
{
	COL_ICON = 0,
	COL_NAME = 1,
	COL_ROT_X = 2,
	COL_ROT_Y = 3,
	COL_ROT_Z = 4,
	COL_POS_X = 5,
	COL_POS_Y = 6,
	COL_POS_Z = 7
} E_Columns;

class BDFloaterPoser :
	public LLFloater
{
	friend class LLFloaterReg;
private:
	BDFloaterPoser(const LLSD& key);
	/*virtual*/	~BDFloaterPoser();
	/*virtual*/	BOOL postBuild();
	/*virtual*/ void draw();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/	void onClose(bool app_quitting);

	//BD - Posing
	void onClickPoseSave();
	void onPoseStart();
	void onPoseDelete();
	void onPoseRefresh();
	void onPoseSet(LLUICtrl* ctrl, const LLSD& param);
	void onPoseControlsRefresh();
	BOOL onPoseSave(S32 type, F32 time, bool editing);
	BOOL onPoseLoad(const LLSD& name);

	//BD - Joints
	void onJointRefresh();
	void onJointSet(LLUICtrl* ctrl, const LLSD& param);
	void onJointPosSet(LLUICtrl* ctrl, const LLSD& param);
	void onJointScaleSet(LLUICtrl* ctrl, const LLSD& param);
	void onJointChangeState();
	void onJointControlsRefresh();
	void onJointRotPosReset();
	void onJointRotationReset();
	void onJointPositionReset();
	void afterJointPositionReset();

	//BD - Animating
	void onAnimAdd(const LLSD& param);
	void onAnimListWrite();
	void onAnimMove(const LLSD& param);
	void onAnimDelete();
	void onAnimSave();
	void onAnimSet();
	void onAnimPlay();
	void onAnimStop();
	void onAnimControlsRefresh();

	//BD - Misc
	void onUpdateLayout();


	//BD - Experimental
	/*void onAddKey();
	void onDeleteKey();
	void addSliderKey(F32 time, BDPoseKey keyframe);
	void onTimeSliderMoved();
	void onKeyTimeMoved();
	void onKeyTimeChanged();
	void onAnimSetValue(LLUICtrl* ctrl, const LLSD& param);

	/// convenience class for holding keyframes mapped to sliders
	struct SliderKey
	{
	public:
		SliderKey(BDPoseKey kf, F32 t) : keyframe(kf), time(t) {}
		SliderKey() : keyframe(), time(0.f) {} // Don't use this default constructor

		BDPoseKey keyframe;
		F32 time;
	};*/



	//BD - Posing
	LLScrollListCtrl*				mPoseScroll;
	LLScrollListCtrl*				mJointsScroll;

	std::array<LLUICtrl*, 3>		mRotationSliders;
	std::array<LLSliderCtrl*, 3>	mPositionSliders;

	//BD - Animations
	LLScrollListCtrl*				mAnimEditorScroll;

	//BD - Misc


	//BD - Exerpimental
	/*std::map<std::string, SliderKey> mSliderToKey;
	LLMultiSliderCtrl*				mTimeSlider;
	LLMultiSliderCtrl*				mKeySlider;*/
};

#endif
