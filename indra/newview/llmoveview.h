/** 
 * @file llmoveview.h
 * @brief Container for buttons for walking, turning, flying
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLMOVEVIEW_H
#define LL_LLMOVEVIEW_H

// Library includes
#include "llfloater.h"

class LLButton;
class LLJoystickAgentTurn;
class LLJoystickAgentSlide;

//
// Classes
//
class LLFloaterMove
:	public LLFloater
{
	LOG_CLASS(LLFloaterMove);
	friend class LLFloaterReg;

private:
	LLFloaterMove(const LLSD& key);
	~LLFloaterMove();
public:
	//BD
	typedef enum stand_stop_flying_mode_t
	{
		SSFM_STAND,
		SSFM_STOP_FLYING
	} EStandStopFlyingMode;

	/*virtual*/	BOOL	postBuild();
	/*virtual*/ void	setVisible(BOOL visible);
	static F32	getYawRate(F32 time);
	static void setFlyingMode(BOOL fly);
	void setFlyingModeImpl(BOOL fly);
	static void setAlwaysRunMode(bool run);
	void setAlwaysRunModeImpl(bool run);
	static void setSittingMode(BOOL bSitting);
	static void enableInstance();
	/*virtual*/ void onOpen(const LLSD& key);

	static void sUpdateFlyingStatus();

	//BD
	void setStandStopFlyingMode(EStandStopFlyingMode mode);
	void clearStandStopFlyingMode(EStandStopFlyingMode mode);

protected:
	void turnLeft();
	void turnRight();

	void moveUp();
	void moveDown();

private:
	typedef enum movement_mode_t
	{
		MM_WALK,
		MM_RUN,
		MM_FLY
	} EMovementMode;

	void onWalkButtonClick();
	void onRunButtonClick();
	void onFlyButtonClick();
	void initMovementMode();
	void setMovementMode(const EMovementMode mode);
	void initModeTooltips();
	void setModeTooltip(const EMovementMode mode);
	void setModeTitle(const EMovementMode mode);
	void initModeButtonMap();
	void setModeButtonToggleState(const EMovementMode mode);
	void updateButtonsWithMovementMode(const EMovementMode newMode);
	void showModeButtons(BOOL bShow);

	//BD
	void onStandButtonClick();
	void onStopFlyingButtonClick();

public:

	LLJoystickAgentTurn*	mForwardButton;
	LLJoystickAgentTurn*	mBackwardButton;
	LLJoystickAgentSlide*	mSlideLeftButton;
	LLJoystickAgentSlide*	mSlideRightButton;
	LLButton*				mTurnLeftButton;
	LLButton*				mTurnRightButton;
	LLButton*				mMoveUpButton;
	LLButton*				mMoveDownButton;

	//BD
	LLButton* mStandButton;
	LLButton* mStopFlyingButton;
private:
	LLPanel*				mModeActionsPanel;
	
	typedef std::map<LLView*, std::string> control_tooltip_map_t;
	typedef std::map<EMovementMode, control_tooltip_map_t> mode_control_tooltip_map_t;
	mode_control_tooltip_map_t mModeControlTooltipsMap;

	typedef std::map<EMovementMode, LLButton*> mode_control_button_map_t;
	mode_control_button_map_t mModeControlButtonMap;
	EMovementMode mCurrentMode;
};


#endif
