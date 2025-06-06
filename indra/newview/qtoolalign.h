/** 
 * @file qtoolalign.h
 * @brief A tool to align objects
 */

#ifndef Q_QTOOLALIGN_H
#define Q_QTOOLALIGN_H

#include "lltool.h"
#include "llthread.h"
#include "llbbox.h"

class LLViewerObject;
class LLPickInfo;
class LLToolSelectRect;

class AlignThread : public LLThread
{
public:
	AlignThread(void);
	~AlignThread();	
	/*virtual*/ void run(void);
	/*virtual*/ void shutdown(void);
	static AlignThread* sInstance;
};

class QToolAlign
:	public LLTool, public LLSingleton<QToolAlign>
{
	LLSINGLETON(QToolAlign);
	virtual ~QToolAlign();
public:

	virtual void	handleSelect();
	virtual void	handleDeselect();
	virtual bool	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual bool    handleHover(S32 x, S32 y, MASK mask);
	virtual void	render();
	virtual bool	canAffectSelection();

	static void pickCallback(const LLPickInfo& pick_info);
	static void aligndone();

	LLBBox          mBBox;
	F32             mManipulatorSize;
	S32             mHighlightedAxis;
	F32             mHighlightedDirection;
	bool            mForce;

private:
	void            align();
	void            computeManipulatorSize();
	void            renderManipulators();
	bool            findSelectedManipulator(S32 x, S32 y);
};

#endif // Q_QTOOLALIGN_H
