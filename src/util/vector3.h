#pragma once

template <typename T>
class Vector3
{
public:
	T x, y, z;

	Vector3()
		: x(0)
		, y(0)
		, z(0)
	{
	}
	Vector3(T a_x, T a_y, T a_z)
		: x(a_x)
		, y(a_y)
		, z(a_z)
	{
	}

	bool Equals(const Vector3<T>& v) const
	{
		return !((x != v.x) || (y != v.y) || (z != v.z));
	}

	bool operator!=(const Vector3<T>& v) const { return !Equals(v); }

	bool operator==(const Vector3<T>& v) const { return Equals(v); }

	void operator+=(const Vector3<T>& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
	}

	void operator-=(const Vector3<T>& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
	}

	void operator*=(const Vector3<T>& v)
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
	}

	void operator*=(T v)
	{
		x *= v;
		y *= v;
		z *= v;
	}
};
