#include <StdCfg.h>
#include "FormattedListboxTextItem.h"

#include <CEGUI/LeftAlignedRenderedString.h>
#include <CEGUI/RightAlignedRenderedString.h>
#include <CEGUI/CentredRenderedString.h>
#include <CEGUI/RenderedStringWordWrapper.h>
#include <CEGUI/CoordConverter.h>
#include <CEGUI/Font.h>
#include <CEGUI/Image.h>
#include <CEGUI/widgets/MultiColumnList.h>

namespace CEGUI
{

//----------------------------------------------------------------------------//
FormattedListboxTextItem::FormattedListboxTextItem(const String& text,
	const HorizontalTextFormatting format,
	const unsigned int item_id,
	void* const item_data,
	const bool disabled,
	const bool auto_delete) :
// initialise base class
ListboxTextItem(text, item_id, item_data, disabled, auto_delete),
	// initialise subclass fields
	d_formatting(format),
	d_formattedRenderedString(NULL),
	d_formattingAreaSize(0, 0)
{
}

//----------------------------------------------------------------------------//
FormattedListboxTextItem::~FormattedListboxTextItem()
{
	delete d_formattedRenderedString;
}

//----------------------------------------------------------------------------//
void FormattedListboxTextItem::setFormatting(const HorizontalTextFormatting fmt)
{
	if (fmt == d_formatting)
		return;

	d_formatting = fmt;
	delete d_formattedRenderedString;
	d_formattedRenderedString = NULL;
	d_formattingAreaSize = Sizef(0, 0);
}

//----------------------------------------------------------------------------//
void FormattedListboxTextItem::updateString() const
{
	bool StringChanged = !d_renderedStringValid;
	if (StringChanged) parseTextString();

	if (!d_formattedRenderedString) setupStringFormatter();
	else if (StringChanged) d_formattedRenderedString->setRenderedString(d_renderedString);

	// get size of render area from target window, to see if we need to reformat
	// NB: We do not use targetRect, since it may not represent the same area.
	const Sizef area_sz(static_cast<const MultiColumnList*>(d_owner)->getListRenderArea().getSize());
	if (StringChanged || area_sz != d_formattingAreaSize)
	{
		d_formattedRenderedString->format(d_owner, area_sz);
		d_formattingAreaSize = area_sz;
	}
}

//----------------------------------------------------------------------------//
Sizef FormattedListboxTextItem::getPixelSize(void) const
{
	if (!d_owner) return Sizef(0, 0);

	updateString();

	return Sizef(d_formattedRenderedString->getHorizontalExtent(d_owner),
		d_formattedRenderedString->getVerticalExtent(d_owner));
}

//----------------------------------------------------------------------------//
std::vector<GeometryBuffer*> FormattedListboxTextItem::createRenderGeometry(
	const Rectf& targetRect,
	float alpha, const Rectf* clipper) const
{
	std::vector<GeometryBuffer*> geomBuffers;

	if (d_selected && d_selectBrush != nullptr)
	{
		ImageRenderSettings imgRenderSettings(
			targetRect, clipper, true,
			d_selectCols, alpha);

		std::vector<GeometryBuffer*> brushGeomBuffers =
			d_selectBrush->createRenderGeometry(imgRenderSettings);

		geomBuffers.insert(geomBuffers.end(), brushGeomBuffers.begin(),
			brushGeomBuffers.end());
	}

	const Font* font = getFont();

	if (!font)
		return geomBuffers;

	glm::vec2 draw_pos(targetRect.getPosition());

	draw_pos.y += CoordConverter::alignToPixels(
		(font->getLineSpacing() - font->getFontHeight()) * 0.5f);

	updateString();

	const ColourRect final_colours(ColourRect(0xFFFFFFFF));

	std::vector<GeometryBuffer*> stringGeomBuffers =
		d_formattedRenderedString->createRenderGeometry(d_owner, draw_pos, &final_colours, clipper);

	geomBuffers.insert(geomBuffers.end(), stringGeomBuffers.begin(), stringGeomBuffers.end());

	return geomBuffers;
}

//----------------------------------------------------------------------------//
void FormattedListboxTextItem::setupStringFormatter() const
{
	// delete any existing formatter
	delete d_formattedRenderedString;

	// create new formatter of whichever type...
	switch(d_formatting)
	{
		case HorizontalTextFormatting::LeftAligned:
			d_formattedRenderedString =
				new LeftAlignedRenderedString(d_renderedString);
			break;

		case HorizontalTextFormatting::RightAligned:
			d_formattedRenderedString =
				new RightAlignedRenderedString(d_renderedString);
			break;

		case HorizontalTextFormatting::CentreAligned:
			d_formattedRenderedString =
				new CentredRenderedString(d_renderedString);
			break;

		case HorizontalTextFormatting::Justified:
			d_formattedRenderedString =
				new JustifiedRenderedString(d_renderedString);
			break;

		case HorizontalTextFormatting::WordWrapLeftAligned:
			d_formattedRenderedString =
				new RenderedStringWordWrapper
				<LeftAlignedRenderedString>(d_renderedString);
			break;

		case HorizontalTextFormatting::WordWrapRightAligned:
			d_formattedRenderedString =
				new RenderedStringWordWrapper
				<RightAlignedRenderedString>(d_renderedString);
			break;

		case HorizontalTextFormatting::WordWrapCentreAligned:
			d_formattedRenderedString =
				new RenderedStringWordWrapper
				<CentredRenderedString>(d_renderedString);
			break;

		case HorizontalTextFormatting::WordWraperJustified:
			d_formattedRenderedString =
				new RenderedStringWordWrapper
				<JustifiedRenderedString>(d_renderedString);
			break;

		default: d_formattedRenderedString = NULL; break;
	}
}

//----------------------------------------------------------------------------//

}