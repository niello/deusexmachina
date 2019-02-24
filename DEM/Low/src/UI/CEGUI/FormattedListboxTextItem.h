#pragma once
#include <CEGUI/widgets/ListboxTextItem.h>

namespace CEGUI
{

//! A ListboxItem based class that can do horizontal text formatiing.
class FormattedListboxTextItem : public ListboxTextItem
{
public:
	//! Constructor
	FormattedListboxTextItem(const String& text,
		const HorizontalTextFormatting format = HorizontalTextFormatting::LeftAligned,
		const unsigned int item_id = 0,
		void* const item_data = 0,
		const bool disabled = false,
		const bool auto_delete = true);

	//! Destructor.
	virtual ~FormattedListboxTextItem() override;

	//! Return the current formatting set.
	HorizontalTextFormatting getFormatting() const { return d_formatting; }
	/*!
	Set the formatting.  You should call MultiColumnList::handleUpdatedItemData
	after setting the formatting in order to update the listbox.  We do not
	do it automatically since you may wish to batch changes to multiple
	items and multiple calls to handleUpdatedItemData is wasteful.
	*/
	void setFormatting(const HorizontalTextFormatting fmt);

	// overridden functions.
	virtual Sizef getPixelSize(void) const override;
	virtual std::vector<GeometryBuffer*> createRenderGeometry(
		const Rectf& targetRect, float alpha, const Rectf* clipper) const override;

protected:
	//! Helper to create a FormattedRenderedString of an appropriate type.
	void updateString() const;
	void setupStringFormatter() const;
	//! Current formatting set
	HorizontalTextFormatting d_formatting;
	//! Class that renders RenderedString with some formatting.
	mutable FormattedRenderedString* d_formattedRenderedString;
	//! Tracks target area for rendering so we can reformat when needed
	mutable Sizef d_formattingAreaSize;
};

}
