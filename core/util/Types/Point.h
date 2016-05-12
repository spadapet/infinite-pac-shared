#pragma once

namespace ff
{
	template<typename T>
	class PointType
	{
	public:
		PointType();
		PointType(T tx, T ty);
		PointType(const PointType<T> &rhs);

		static PointType<T> Zeros();

		// functions

		void SetPoint(T tx, T ty);
		void Offset(T tx, T ty);
		void Offset(const PointType<T> &rhs);
		void FloorToIntegers();
		bool IsNull() const;
		T Length() const;
		T Length2() const;

		// operators

		PointType<T> &operator=(const PointType<T> &rhs);
		PointType<T> &operator=(const SIZE &rhs);
		PointType<T> &operator=(const POINT &rhs);

		bool operator==(const PointType<T> &rhs) const;
		bool operator!=(const PointType<T> &rhs) const;

		PointType<T> operator+(const PointType<T> &rhs) const;
		PointType<T> operator-(const PointType<T> &rhs) const;
		PointType<T> operator-();

		void operator+=(const PointType<T> &rhs);
		void operator-=(const PointType<T> &rhs);

		void operator*=(T scale);
		void operator*=(const PointType<T> &rhs);
		void operator/=(T scale);
		void operator/=(const PointType<T> &rhs);

		PointType<T> operator*(T scale) const;
		PointType<T> operator*(const PointType<T> &rhs) const;
		PointType<T> operator/(T scale) const;
		PointType<T> operator/(const PointType<T> &rhs) const;

		POINT ToPOINT() const;
		PointType<int> ToInt() const;
		PointType<short> ToShort() const;
		PointType<float> ToFloat() const;
		PointType<double> ToDouble() const;
		PointType<size_t> ToSize() const;

		// vars

		union
		{
			struct {T x, y;};
			T arr[2];
		};
	};

	typedef PointType<int> PointInt;
	typedef PointType<short> PointShort;
	typedef PointType<float> PointFloat;
	typedef PointType<double> PointDouble;
	typedef PointType<size_t> PointSize;
}

template<typename T>
ff::PointType<T>::PointType()
{
}

template<typename T>
ff::PointType<T>::PointType(T tx, T ty)
	: x(tx)
	, y(ty)
{
}

template<typename T>
ff::PointType<T>::PointType(const PointType<T> &rhs)
	: x(rhs.x)
	, y(rhs.y)
{
}

template<typename T>
ff::PointType<T> ff::PointType<T>::Zeros()
{
	return PointType<T>(0, 0);
}

template<typename T>
ff::PointType<T> &ff::PointType<T>::operator=(const PointType<T> &rhs)
{
	x = rhs.x;
	y = rhs.y;
	return *this;
}

template<typename T>
void ff::PointType<T>::SetPoint(T tx, T ty)
{
	x = tx;
	y = ty;
}

template<typename T>
void ff::PointType<T>::Offset(T tx, T ty)
{
	x += tx;
	y += ty;
}

template<typename T>
void ff::PointType<T>::Offset(const PointType<T> &zPt)
{
	x += zPt.x;
	y += zPt.y;
}

template<typename T>
void ff::PointType<T>::FloorToIntegers()
{
	x = std::floor(x);
	y = std::floor(y);
}

template<typename T>
bool ff::PointType<T>::IsNull() const
{
	return x == 0 && y == 0;
}

template<typename T>
T ff::PointType<T>::Length() const
{
	return std::sqrt(Length2());
}

template<typename T>
T ff::PointType<T>::Length2() const
{
	return x * x + y * y;
}

template<typename T>
bool ff::PointType<T>::operator==(const PointType<T> &rhs) const
{
	return x == rhs.x && y == rhs.y;
}

template<typename T>
bool ff::PointType<T>::operator!=(const PointType<T> &rhs) const
{
	return x != rhs.x || y != rhs.y;
}

template<typename T>
ff::PointType<T> ff::PointType<T>::operator+(const PointType<T> &rhs) const
{
	return PointType<T>(x + rhs.x, y + rhs.y);
}

template<typename T>
ff::PointType<T> ff::PointType<T>::operator-(const PointType<T> &rhs) const
{
	return PointType<T>(x - rhs.x, y - rhs.y);
}

template<typename T>
ff::PointType<T> ff::PointType<T>::operator-()
{
	return PointType<T>(-x, -y);
}

template<typename T>
void ff::PointType<T>::operator+=(const PointType<T> &rhs)
{
	x += rhs.x;
	y += rhs.y;
}

template<typename T>
void ff::PointType<T>::operator-=(const PointType<T> &rhs)
{
	x -= rhs.x;
	y -= rhs.y;
}

template<typename T>
void ff::PointType<T>::operator*=(T scale)
{
	x *= scale;
	y *= scale;
}

template<typename T>
void ff::PointType<T>::operator*=(const PointType<T> &rhs)
{
	x *= rhs.x;
	y *= rhs.y;
}

template<typename T>
void ff::PointType<T>::operator/=(T scale)
{
	x /= scale;
	y /= scale;
}

template<typename T>
void ff::PointType<T>::operator/=(const PointType<T> &rhs)
{
	x /= rhs.x;
	y /= rhs.y;
}

template<typename T>
ff::PointType<T> ff::PointType<T>::operator*(T scale) const
{
	return PointType<T>(x * scale, y * scale);
}

template<typename T>
ff::PointType<T> ff::PointType<T>::operator/(T scale) const
{
	return PointType<T>(x / scale, y / scale);
}

template<typename T>
ff::PointType<T> ff::PointType<T>::operator*(const PointType<T> &rhs) const
{
	return PointType<T>(x * rhs.x, y * rhs.y);
}

template<typename T>
ff::PointType<T> ff::PointType<T>::operator/(const PointType<T> &rhs) const
{
	return PointType<T>(x / rhs.x, y / rhs.y);
}

template<typename T>
ff::PointType<T> &ff::PointType<T>::operator=(const SIZE &rhs)
{
	x = (T)rhs.cx;
	y = (T)rhs.cy;
	return *this;
}

template<typename T>
ff::PointType<T> &ff::PointType<T>::operator=(const POINT &rhs)
{
	x = (T)rhs.x;
	y = (T)rhs.y;
	return *this;
}

template<typename T>
POINT ff::PointType<T>::ToPOINT() const
{
	POINT pt = { (int)x, (int)y };
	return pt;
}

template<typename T>
ff::PointType<int> ff::PointType<T>::ToInt() const
{
	return ff::PointType<int>((int)x, (int)y);
}

template<typename T>
ff::PointType<short> ff::PointType<T>::ToShort() const
{
	return ff::PointType<short>((short)x, (short)y);
}

template<typename T>
ff::PointType<float> ff::PointType<T>::ToFloat() const
{
	return ff::PointType<float>((float)x, (float)y);
}

template<typename T>
ff::PointType<double> ff::PointType<T>::ToDouble() const
{
	return ff::PointType<double>((double)x, (double)y);
}

template<typename T>
ff::PointType<size_t> ff::PointType<T>::ToSize() const
{
	return ff::PointType<size_t>((size_t)x, (size_t)y);
}

MAKE_POD(ff::PointInt);
MAKE_POD(ff::PointShort);
MAKE_POD(ff::PointFloat);
MAKE_POD(ff::PointDouble);
MAKE_POD(ff::PointSize);
