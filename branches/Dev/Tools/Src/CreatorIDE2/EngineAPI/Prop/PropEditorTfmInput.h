#pragma once
#ifndef __CIDE_PROP_EDITOR_TFM_INPUT_H__
#define __CIDE_PROP_EDITOR_TFM_INPUT_H__

//#include <Input/PropInput.h>

// Input processor that allows to transform current entity in the editor

namespace Properties
{

class CPropEditorTfmInput: public CPropInput
{
	DeclareRTTI;
	DeclareFactory(CPropEditorTfmInput);

private:

	virtual void ActivateInput();
    virtual void DeactivateInput();

	DECLARE_EVENT_HANDLER(OnBeginFrame, OnBeginFrame);

public:

};

RegisterFactory(CPropEditorTfmInput);

}

#endif
