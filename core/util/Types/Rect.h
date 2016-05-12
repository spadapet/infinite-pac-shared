#pragma once

namespace ff
{
	template<typename T>
	class RectType
	{
	public:
		RectType();
		RectType(T tLeft, T tTop, T tRight, T tBottom);
		RectType(const RectType<T> &rhs);
		RectType(const PointType<T> &rhs1, const PointType<T> &rhs2);
		RectType(const PointType<T> &rhs);
		RectType(const RECT &rhs);

		static RectType<T> Zeros();

		T Width() const;
		T Height() const;
		T Area() const;
		PointType<T> Size() const;
		PointType<T> Center() const;
		PointType<T> TopLeft() const;
		PointType<T> TopRight() const;
		PointType<T> BottomLeft() const;
		PointType<T> BottomRight() const;

		bool IsEmpty() const;
		bool IsNull() const;
		bool PointInRect(const PointType<T> &point) const;

		void SetRect(T tLeft, T tTop, T tRight, T tBottom);
		void SetRect(const PointType<T> &topLeft, const PointType<T> &bottomRight);
		void SetRect(T tRight, T tBottom);
		void SetRect(const PointType<T> &bottomRight);

		void Normalize();
		void EnsurePositiveSize();
		void EnsureMinimumSize(const PointType<T> &point);
		void EnsureMinimumSize(T width, T height);

		bool Intersect(const RectType<T> &rhs); // intersect self with rhs
		bool Intersect(const RectType<T> &rhs1, const RectType<T> &rhs2);
		bool DoesTouch(const RectType<T> &rhs) const; // returns true if (*this) touches rhs at all
		bool IsInside(const RectType<T> &rhs) const; // returns true if (*this) is totally inside of rhs
		bool IsOutside(const RectType<T> &rhs) const; // returns true if (*this) is totally outside of rhs
		void Bound(const RectType<T> &rhs); // make myself larger to also cover rhs
		void Bound(const RectType<T> &rhs1, const RectType<T> &rhs2); // make myself cover rhs1 and rhs2
		void Bound(const PointType<T> &point);
		void Bound(const PointType<T> &pt1, const PointType<T> &pt2);

		void CenterWithin(const RectType<T> &rhs); // center (*this) within rhs, if bigger than rhs, center on top of rhs
		void CenterOn(const PointType<T> &point);// center (*this) on top of point
		void ScaleToFit(const RectType<T> &rhs); // moves bottom-right so that (*this) would fit within rhs, but keeps aspect ratio
		void MoveInside(const RectType<T> &rhs); // move this rect so that it is inside of rhs (impossible to do if rhs is smaller than *this)
		void MoveTopLeft(const PointType<T> &point); // keeps the same size, but moves the rect to point
		void MoveTopLeft(T tx, T ty); // same as above
		void Crop(const RectType<T> &rhs); // cut off my edges so that I'm inside of rhs
		void Interpolate(const RectType<T> &rhs1, const RectType<T> &rhs2, double value); // 0 is rhs1, 1 is rhs2
		void FloorToIntegers();

		void Offset(T tx, T ty);
		void Offset(const PointType<T> &point);
		void OffsetSize(T tx, T ty);
		void OffsetSize(const PointType<T> &point);
		void Deflate(T tx, T ty);
		void Deflate(T tx, T ty, T tx2, T ty2);
		void Deflate(const PointType<T> &point);
		void Deflate(const RectType<T> &rhs);

		// operators

		RectType<T> &operator=(const RectType<T> &rhs);
		bool operator==(const RectType<T> &rhs) const;
		bool operator!=(const RectType<T> &rhs) const;

		void operator+=(const PointType<T> &point);
		void operator-=(const PointType<T> &point);

		const RectType<T> operator+(const PointType<T> &point) const;
		const RectType<T> operator-(const PointType<T> &point) const;

		const RectType<T> operator*(const RectType<T> &rhs) const;
		const RectType<T> operator*(const PointType<T> &rhs) const;
		const RectType<T> operator/(const RectType<T> &rhs) const;
		const RectType<T> operator/(const PointType<T> &rhs) const;

		RectType<T> &operator=(const RECT &rhs);
		RECT ToRECT() const;
		RectType<int> ToInt() const;
		RectType<short> ToShort() const;
		RectType<float> ToFloat() const;
		RectType<double> ToDouble() const;
		RectType<size_t> ToSize() const;

		union
		{
			struct { T left, top, right, bottom; };
			T arr[4];
		};
	};

	typedef RectType<int> RectInt;
	typedef RectType<short> RectShort;
	typedef RectType<float> RectFloat;
	typedef RectType<double> RectDouble;
	typedef RectType<size_t> RectSize;
}

template<typename T>
ff::RectType<T>::RectType()
{
}

template<typename T>
ff::RectType<T>::RectType(T tLeft, T tTop, T tRight, T tBottom)
	: left(tLeft)
	, top(tTop)
	, right(tRight)
	, bottom(tBottom)
{
}

template<typename T>
ff::RectType<T>::RectType(const RectType<T> &rhs)
{
	*this = rhs;
}

template<typename T>
ff::RectType<T>::RectType(const PointType<T> &rhs1, const PointType<T> &rhs2)
	: left(rhs1.x)
	, top(rhs1.y)
	, right(rhs2.x)
	, bottom(rhs2.y)
{
}

template<typename T>
ff::RectType<T>::RectType(const PointType<T> &rhs)
	: left(0)
	, top(0)
	, right(rhs.x)
	, bottom(rhs.y)
{
}

template<typename T>
ff::RectType<T>::RectType(const RECT &rhs)
{
	*this = rhs;
}

// static
template<typename T>
ff::RectType<T> ff::RectType<T>::Zeros()
{
	return RectType<T>(0, 0, 0, 0);
}

template<typename T>
T ff::RectType<T>::Width() const
{
	return right - left;
}

template<typename T>
T ff::RectType<T>::Height() const
{
	return bottom - top;
}

template<typename T>
T ff::RectType<T>::Area() const
{
	return Width() * Height();
}

template<typename T>
ff::PointType<T> ff::RectType<T>::Size() const
{
	return PointType<T>(right - left, bottom - top);
}

template<typename T>
ff::PointType<T> ff::RectType<T>::Center() const
{
	return PointType<T>((left + right) / 2, (top + bottom) / 2);
}

template<typename T>
bool ff::RectType<T>::IsEmpty() const
{
	return bottom == top && right == left;
}

template<typename T>
bool ff::RectType<T>::IsNull() const
{
	return left == 0 && top == 0 && bottom - top == 0 && right - left == 0;
}

template<typename T>
bool ff::RectType<T>::PointInRect(const PointType<T> &point) const
{
	return point.x >= left && point.x < right && point.y >= top && point.y < bottom;
}

template<typename T>
ff::PointType<T> ff::RectType<T>::TopLeft() const
{
	return PointType<T>(left, top);
}

template<typename T>
ff::PointType<T> ff::RectType<T>::TopRight() const
{
	return PointType<T>(right, top);
}

template<typename T>
ff::PointType<T> ff::RectType<T>::BottomLeft() const
{
	return PointType<T>(left, bottom);
}

template<typename T>
ff::PointType<T> ff::RectType<T>::BottomRight() const
{
	return PointType<T>(right, bottom);
}

template<typename T>
void ff::RectType<T>::SetRect(T tLeft, T tTop, T tRight, T tBottom)
{
	left = tLeft;
	top = tTop;
	right = tRight;
	bottom = tBottom;
}

template<typename T>
void ff::RectType<T>::SetRect(const PointType<T> &topLeft, const PointType<T> &bottomRight)
{
	left = topLeft.x;
	top = topLeft.y;
	right = bottomRight.x;
	bottom = bottomRight.y;
}

template<typename T>
void ff::RectType<T>::SetRect(T tRight, T tBottom)
{
	left = 0;
	top = 0;
	right = tRight;
	bottom = tBottom;
}

template<typename T>
void ff::RectType<T>::SetRect(const PointType<T> &bottomRight)
{
	left = 0;
	top = 0;
	right = bottomRight.x;
	bottom = bottomRight.y;
}

template<typename T>
void ff::RectType<T>::Offset(T tx, T ty)
{
	left += tx;
	right += tx;
	top += ty;
	bottom += ty;
}

template<typename T>
void ff::RectType<T>::Offset(const PointType<T> &point)
{
	left += point.x;
	right += point.x;
	top += point.y;
	bottom += point.y;
}

template<typename T>
void ff::RectType<T>::OffsetSize(T tx, T ty)
{
	right += tx;
	bottom += ty;
}

template<typename T>
void ff::RectType<T>::OffsetSize(const PointType<T> &point)
{
	right += point.x;
	bottom += point.y;
}

template<typename T>
void ff::RectType<T>::Normalize()
{
	if (left > right)
	{
		std::swap(left, right);
	}

	if (top > bottom)
	{
		std::swap(top, bottom);
	}
}

template<typename T>
void ff::RectType<T>::EnsurePositiveSize()
{
	if (bottom < top)
	{
		bottom = top;
	}

	if (right < left)
	{
		right = left;
	}
}

template<typename T>
void ff::RectType<T>::EnsureMinimumSize(const PointType<T> &point)
{
	right = std::max(left + point.x, right);
	bottom = std::max(top + point.y, bottom);
}

template<typename T>
void ff::RectType<T>::EnsureMinimumSize(T width, T height)
{
	right = std::max(left + width, right);
	bottom = std::max(top + height, bottom);
}

template<typename T>
void ff::RectType<T>::Deflate(T tx, T ty)
{
	left += tx;
	right -= tx;
	top += ty;
	bottom -= ty;
	Normalize();
}

template<typename T>
void ff::RectType<T>::Deflate(T tx, T ty, T tx2, T ty2)
{
	left += tx;
	right -= tx2;
	top += ty;
	bottom -= ty2;
	Normalize();
}

template<typename T>
void ff::RectType<T>::Deflate(const PointType<T> &point)
{
	left += point.x;
	right -= point.x;
	top += point.y;
	bottom -= point.y;
	Normalize();
}

template<typename T>
void ff::RectType<T>::Deflate(const RectType<T> &rhs)
{
	left += rhs.left;
	right -= rhs.right;
	top += rhs.top;
	bottom -= rhs.bottom;
	Normalize();
}

template<typename T>
bool ff::RectType<T>::Intersect(const RectType<T> &rhs)
{
	T nLeft = std::max(left, rhs.left);
	T nRight = std::min(right, rhs.right);
	T nTop = std::max(top, rhs.top);
	T nBottom = std::min(bottom, rhs.bottom);

	if (nLeft <= nRight && nTop <= nBottom)
	{
		SetRect(nLeft, nTop, nRight, nBottom);
		return true;
	}
	else
	{
		SetRect(0, 0, 0, 0);
		return false;
	}
}

template<typename T>
bool ff::RectType<T>::Intersect(const RectType<T> &rhs1, const RectType<T> &rhs2)
{
	*this = rhs1;
	return Intersect(rhs2);
}

template<typename T>
bool ff::RectType<T>::DoesTouch(const RectType<T> &rhs) const
{
	return right > rhs.left && left < rhs.right && bottom > rhs.top && top < rhs.bottom;
}

template<typename T>
bool ff::RectType<T>::IsInside(const RectType<T> &rhs) const
{
	return left >= rhs.left && right <= rhs.right && top >= rhs.top && bottom <= rhs.bottom;
}

template<typename T>
bool ff::RectType<T>::IsOutside(const RectType<T> &rhs) const
{
	return left >= rhs.right || right <= rhs.left || top >= rhs.bottom || bottom <= rhs.top;
}

template<typename T>
void ff::RectType<T>::Bound(const RectType<T> &rhs)
{
	SetRect(
		std::min(left, rhs.left),
		std::min(top, rhs.top),
		std::max(right, rhs.right),
		std::max(bottom, rhs.bottom));
}

template<typename T>
void ff::RectType<T>::Bound(const RectType<T> &rhs1, const RectType<T> &rhs2)
{
	SetRect(
		std::min(rhs1.left, rhs2.left),
		std::min(rhs1.top, rhs2.top),
		std::max(rhs1.right, rhs2.right),
		std::max(rhs1.bottom, rhs2.bottom));
}

template<typename T>
void ff::RectType<T>::Bound(const PointType<T> &point)
{
	SetRect(
		std::min(left, point.x),
		std::min(top, point.y),
		std::max(right, point.x),
		std::max(bottom, point.y));
}

template<typename T>
void ff::RectType<T>::Bound(const PointType<T> &pt1, const PointType<T> &pt2)
{
	SetRect(
		std::min(pt1.x, pt2.x),
		std::min(pt1.y, pt2.y),
		std::max(pt1.x, pt2.x),
		std::max(pt1.y, pt2.y));
}

template<typename T>
void ff::RectType<T>::CenterWithin(const RectType<T> &rhs)
{
	T width = right - left;
	T height = bottom - top;

	left = rhs.left + (rhs.Width() - width) / 2;
	top = rhs.top + (rhs.Height() - height) / 2;
	right = left + width;
	bottom = top + height;
}

template<typename T>
void ff::RectType<T>::CenterOn(const PointType<T> &point)
{
	T width = right - left;
	T height = bottom - top;

	left = point.x - width / 2;
	top = point.y - height / 2;
	right = left + width;
	bottom = top + height;
}

template<typename T>
void ff::RectType<T>::MoveInside(const RectType<T> &rhs)
{
	if (left < rhs.left)
	{
		T offset = rhs.left - left;

		left += offset;
		right += offset;
	}
	else if (right > rhs.right)
	{
		T offset = right - rhs.right;

		left -= offset;
		right -= offset;
	}

	if (top < rhs.top)
	{
		T offset = rhs.top - top;

		top += offset;
		bottom += offset;
	}
	else if (bottom > rhs.bottom)
	{
		T offset = bottom - rhs.bottom;

		top -= offset;
		bottom -= offset;
	}
}

template<typename T>
void ff::RectType<T>::MoveTopLeft(const PointType<T> &point)
{
	Offset(point.x - left, point.y - top);
}

template<typename T>
void ff::RectType<T>::MoveTopLeft(T tx, T ty)
{
	Offset(tx - left, ty - top);
}

template<typename T>
void ff::RectType<T>::Crop(const RectType<T> &rhs)
{
	if (left < rhs.left)
	{
		left = rhs.left;
	}

	if (right > rhs.right)
	{
		right = rhs.right;
	}

	if (top < rhs.top)
	{
		top = rhs.top;
	}

	if (bottom > rhs.bottom)
	{
		bottom = rhs.bottom;
	}
}

template<typename T>
void ff::RectType<T>::ScaleToFit(const RectType<T> &rhs)
{
	if (left == right)
	{
		if (top != bottom)
		{
			bottom = top + rhs.Height();
		}
	}
	else if (top == bottom)
	{
		right = left + rhs.Width();
	}
	else
	{
		double ratio = (double)(right - left) / (double)(bottom - top);

		if ((double)rhs.Width() / ratio > (double)rhs.Height())
		{
			right = left + (T)((double)rhs.Height() * ratio);
			bottom = top + rhs.Height();
		}
		else
		{
			right = left + rhs.Width();
			bottom = top + (T)((double)rhs.Width() / ratio);
		}
	}
}

template<typename T>
void ff::RectType<T>::Interpolate(const RectType<T> &rhs1, const RectType<T> &rhs2, double value)
{
	RectDouble dr1((double)rhs1.left, (double)rhs1.top, (double)rhs1.right, (double)rhs1.bottom);
	RectDouble dr2((double)rhs2.left, (double)rhs2.top, (double)rhs2.right, (double)rhs2.bottom);

	SetRect(
		(T)((dr2.left - dr1.left) * value + dr1.left ),
		(T)((dr2.top - dr1.top) * value + dr1.top ),
		(T)((dr2.right - dr1.right) * value + dr1.right ),
		(T)((dr2.bottom - dr1.bottom) * value + dr1.bottom));
}

template<typename T>
void ff::RectType<T>::FloorToIntegers()
{
}

template<>
inline void ff::RectType<float>::FloorToIntegers()
{
	left = std::floor(left);
	top = std::floor(top);
	right = std::floor(right);
	bottom = std::floor(bottom);
}

template<>
inline void ff::RectType<double>::FloorToIntegers()
{
	left = std::floor(left);
	top = std::floor(top);
	right = std::floor(right);
	bottom = std::floor(bottom);
}

template<typename T>
ff::RectType<T> &ff::RectType<T>::operator=(const RectType<T> &rhs)
{
	CopyObject(*this, rhs);
	return *this;
}

template<typename T>
bool ff::RectType<T>::operator==(const RectType<T> &rhs) const
{
	return !CompareObjects(*this, rhs);
}

template<typename T>
bool ff::RectType<T>::operator!=(const RectType<T> &rhs) const
{
	return !(*this == rhs);
}

template<typename T>
void ff::RectType<T>::operator+=(const PointType<T> &point)
{
	left += point.x;
	top += point.y;
	right += point.x;
	bottom += point.y;
}

template<typename T>
void ff::RectType<T>::operator-=(const PointType<T> &point)
{
	left -= point.x;
	top -= point.y;
	right -= point.x;
	bottom -= point.y;
}

template<typename T>
const ff::RectType<T> ff::RectType<T>::operator+(const PointType<T> &point) const
{
	return RectType<T>(left + point.x, top + point.y, right + point.x, bottom + point.y);
}

template<typename T>
const ff::RectType<T> ff::RectType<T>::operator-(const PointType<T> &point) const
{
	return RectType<T>(left - point.x, top - point.y, right - point.x, bottom - point.y);
}

template<typename T>
const ff::RectType<T> ff::RectType<T>::operator*(const RectType<T> &rhs) const
{
	return RectType<T>(left * rhs.left, top * rhs.top, right * rhs.right, bottom * rhs.bottom);
}

template<typename T>
const ff::RectType<T> ff::RectType<T>::operator*(const PointType<T> &rhs) const
{
	return RectType<T>(left * rhs.x, top * rhs.y, right * rhs.x, bottom * rhs.y);
}

template<typename T>
const ff::RectType<T> ff::RectType<T>::operator/(const RectType<T> &rhs) const
{
	return RectType<T>(left / rhs.left, top / rhs.top, right / rhs.right, bottom / rhs.bottom);
}

template<typename T>
const ff::RectType<T> ff::RectType<T>::operator/(const PointType<T> &rhs) const
{
	return RectType<T>(left / rhs.x, top / rhs.y, right / rhs.x, bottom / rhs.y);
}

template<typename T>
ff::RectType<T> &ff::RectType<T>::operator=(const RECT &rhs)
{
	SetRect((T)rhs.left, (T)rhs.top, (T)rhs.right, (T)rhs.bottom);
	return *this;
}

template<typename T>
RECT ff::RectType<T>::ToRECT() const
{
	RECT rect = { (int)left, (int)top, (int)right, (int)bottom };
	return rect;
}

template<typename T>
ff::RectType<int> ff::RectType<T>::ToInt() const
{
	return ff::RectType<int>((int)left, (int)top, (int)right, (int)bottom);
}

template<typename T>
ff::RectType<short> ff::RectType<T>::ToShort() const
{
	return ff::RectType<short>((short)left, (short)top, (short)right, (short)bottom);
}

template<typename T>
ff::RectType<float> ff::RectType<T>::ToFloat() const
{
	return ff::RectType<float>((float)left, (float)top, (float)right, (float)bottom);
}

template<typename T>
ff::RectType<double> ff::RectType<T>::ToDouble() const
{
	return ff::RectType<double>((double)left, (double)top, (double)right, (double)bottom);
}

template<typename T>
ff::RectType<size_t> ff::RectType<T>::ToSize() const
{
	return ff::RectType<size_t>((size_t)left, (size_t)top, (size_t)right, (size_t)bottom);
}

MAKE_POD(ff::RectInt);
MAKE_POD(ff::RectShort);
MAKE_POD(ff::RectFloat);
MAKE_POD(ff::RectDouble);
MAKE_POD(ff::RectSize);
