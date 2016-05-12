#include "pch.h"
#include "Graph/RenderTarget/Viewport.h"
#include "Graph/RenderTarget/RenderTarget.h"

ff::Viewport::Viewport()
	: _aspect(1920, 1080)
	, _padding(0, 0, 0, 0)
	, _viewport(0, 0, 0, 0)
	, _targetSize(0, 0)
{
}

ff::Viewport::~Viewport()
{
}

void ff::Viewport::SetAutoSize(PointFloat aspect, RectFloat padding)
{
	_aspect = aspect;
	_padding = padding;
}

ff::RectFloat ff::Viewport::GetView(IRenderTarget *target)
{
	_target = target;
	assertRetVal(_target, RectFloat::Zeros());

	PointFloat targetSize = _target->GetRotatedSize().ToFloat();
	if (targetSize != _targetSize)
	{
		RectFloat safeArea(targetSize);
		if (safeArea.Width() > _padding.left + _padding.right &&
			safeArea.Height() > _padding.top + _padding.bottom)
		{
			// Adjust for padding
			safeArea.Deflate(_padding);
		}

		_viewport = (_aspect.x && _aspect.y) ? RectFloat(_aspect) : safeArea;
		_viewport.ScaleToFit(safeArea);
		_viewport.CenterWithin(safeArea);
		_viewport.FloorToIntegers();

		_targetSize = targetSize;
	}

	return _viewport;
}
