#pragma once

#include "Base.hpp"
#include "Error.hpp"

inline constexpr float Pi = 3.14159265358979323846f;
inline constexpr float DegreesToRadians = Pi / 180.0f;
inline constexpr float RadiansToDegrees = 180.0f / Pi;

float SquareRoot(float x);

float Sine(float x);
float Cosine(float x);
float Tangent(float x);

template<typename T>
T Min(T a, T b)
{
	return (a > b) ? b : a;
}

template<typename T>
T Max(T a, T b)
{
	return (a > b) ? a : b;
}

template<typename T>
T Clamp(T value, T min, T max)
{
	return (value > max) ? max : ((value < min) ? min : value);
}

inline uint64 NextMultipleOf(uint64 value, uint64 multiple)
{
	CHECK(multiple > 0);
	const uint64 remainder = value % multiple;
	return remainder ? (value + multiple - remainder) : value;
}

class Vector
{
public:
	static const Vector Zero;

	Vector() = default;

	Vector(float x, float y, float z)
		: X(x)
		, Y(y)
		, Z(z)
	{
	}

	Vector operator+(const Vector& b) const
	{
		return Vector { X + b.X, Y + b.Y, Z + b.Z };
	}

	Vector operator-(const Vector& b) const
	{
		return Vector { X - b.X, Y - b.Y, Z - b.Z };
	}

	Vector operator*(float scale) const
	{
		return Vector { scale * X, scale * Y, scale * Z };
	}

	float GetLength() const
	{
		return SquareRoot(X * X + Y * Y + Z * Z);
	}

	float GetLengthSquared() const
	{
		return X * X + Y * Y + Z * Z;
	}

	Vector Normalize() const
	{
		const float length = GetLength();
		return Vector { X / length, Y / length, Z / length };
	}

	float Dot(const Vector& b) const
	{
		return X * b.X + Y * b.Y + Z * b.Z;
	}

	Vector Cross(const Vector& b) const
	{
		return Vector
		{
			Y * b.Z - Z * b.Y,
			Z * b.X - X * b.Z,
			X * b.Y - Y * b.X,
		};
	}

	float X;
	float Y;
	float Z;
};

inline const Vector Vector::Zero = { 0, 0, 0 };

class Matrix
{
public:
	static const Matrix Identity;

	static constexpr usize Dimension = 4;

	Matrix() = default;

	Matrix(float m00, float m10, float m20, float m30,
		   float m01, float m11, float m21, float m31,
		   float m02, float m12, float m22, float m32,
		   float m03, float m13, float m23, float m33)
		: M00(m00), M10(m10), M20(m20), M30(m30)
		, M01(m01), M11(m11), M21(m21), M31(m31)
		, M02(m02), M12(m12), M22(m22), M32(m32)
		, M03(m03), M13(m13), M23(m23), M33(m33)
	{
	}

	static Matrix Scale(float scaleX, float scaleY, float scaleZ)
	{
		return Matrix
		{
			scaleX, 0.0f,   0.0f,   0.0f,
			0.0f,   scaleY, 0.0f,   0.0f,
			0.0f,   0.0f,   scaleZ, 0.0f,
			0.0f,   0.0f,   0.0f,   1.0f,
		};
	}

	static Matrix Translation(float x, float y, float z)
	{
		return Matrix
		{
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			x,    y,    z,    1.0f,
		};
	}

	static Matrix Perspective(float fieldOfViewYDegrees, float aspectRatio, float nearZ, float farZ)
	{
		const float height = 1.0f / Tangent(0.5f * fieldOfViewYDegrees);
		const float width = height / aspectRatio;
		const float range = farZ / (nearZ - farZ);
		return Matrix
		{
			width, 0.0f,   0.0f,          0.0f,
			0.0f,  height, 0.0f,          0.0f,
			0.0f,  0.0f,   range,         -1.0f,
			0.0f,  0.0f,   range * nearZ, 0.0f,
		};
	}

	static Matrix LookAt(Vector position, Vector at, Vector up)
	{
		const Vector back = (at - position).Normalize();
		const Vector right = back.Cross(up).Normalize();
		up = right.Cross(back);
		return Matrix
		{
			right.X,              up.X,              -back.X,            0.0f,
			right.Y,              up.Y,              -back.Y,            0.0f,
			right.Z,              up.Z,              -back.Z,            0.0f,
			-position.Dot(right), -position.Dot(up), position.Dot(back), 1.0,
		};
	}

	float& operator()(usize row, usize column)
	{
		CHECK(column * Dimension + row < Dimension * Dimension);
		return (&M00)[column * Dimension + row];
	}

	const float& operator()(usize row, usize column) const
	{
		CHECK(column * Dimension + row < Dimension * Dimension);
		return (&M00)[column * Dimension + row];
	}

	Matrix operator+(const Matrix& b) const
	{
		return Matrix
		{
			M00 + b.M00, M10 + b.M10, M20 + b.M20, M30 + b.M30,
			M01 + b.M01, M11 + b.M11, M21 + b.M21, M31 + b.M31,
			M02 + b.M02, M12 + b.M12, M22 + b.M22, M32 + b.M32,
			M03 + b.M03, M13 + b.M13, M23 + b.M23, M33 + b.M33,
		};
	}

	Matrix operator-(const Matrix& b) const
	{
		return Matrix
		{
			M00 - b.M00, M10 - b.M10, M20 - b.M20, M30 - b.M30,
			M01 - b.M01, M11 - b.M11, M21 - b.M21, M31 - b.M31,
			M02 - b.M02, M12 - b.M12, M22 - b.M22, M32 - b.M32,
			M03 - b.M03, M13 - b.M13, M23 - b.M23, M33 - b.M33,
		};
	}

	Matrix operator*(const Matrix& b) const
	{
		return Matrix
		{
			b.M00 * M00 + b.M10 * M01 + b.M20 * M02 + b.M30 * M03,
			b.M00 * M10 + b.M10 * M11 + b.M20 * M12 + b.M30 * M13,
			b.M00 * M20 + b.M10 * M21 + b.M20 * M22 + b.M30 * M23,
			b.M00 * M30 + b.M10 * M31 + b.M20 * M32 + b.M30 * M33,

			b.M01 * M00 + b.M11 * M01 + b.M21 * M02 + b.M31 * M03,
			b.M01 * M10 + b.M11 * M11 + b.M21 * M12 + b.M31 * M13,
			b.M01 * M20 + b.M11 * M21 + b.M21 * M22 + b.M31 * M23,
			b.M01 * M30 + b.M11 * M31 + b.M21 * M32 + b.M31 * M33,

			b.M02 * M00 + b.M12 * M01 + b.M22 * M02 + b.M32 * M03,
			b.M02 * M10 + b.M12 * M11 + b.M22 * M12 + b.M32 * M13,
			b.M02 * M20 + b.M12 * M21 + b.M22 * M22 + b.M32 * M23,
			b.M02 * M30 + b.M12 * M31 + b.M22 * M32 + b.M32 * M33,

			b.M03 * M00 + b.M13 * M01 + b.M23 * M02 + b.M33 * M03,
			b.M03 * M10 + b.M13 * M11 + b.M23 * M12 + b.M33 * M13,
			b.M03 * M20 + b.M13 * M21 + b.M23 * M22 + b.M33 * M23,
			b.M03 * M30 + b.M13 * M31 + b.M23 * M32 + b.M33 * M33,
		};
	}

	Matrix operator*(float scale) const
	{
		return Matrix
		{
			scale * M00, scale * M10, scale * M20, scale * M30,
			scale * M01, scale * M11, scale * M21, scale * M31,
			scale * M02, scale * M12, scale * M22, scale * M32,
			scale * M03, scale * M13, scale * M23, scale * M33,
		};
	}

	Vector Transform(Vector b) const
	{
		return Vector
		{
			M00 * b.X + M01 * b.Y + M02 * b.Z + 1.0f * M03,
			M10 * b.X + M11 * b.Y + M12 * b.Z + 1.0f * M13,
			M20 * b.X + M21 * b.Y + M22 * b.Z + 1.0f * M23,
		};
	}

	Vector TransformDirection(Vector b) const
	{
		return Vector
		{
			M00 * b.X + M01 * b.Y + M02 * b.Z,
			M10 * b.X + M11 * b.Y + M12 * b.Z,
			M20 * b.X + M21 * b.Y + M22 * b.Z,
		};
	}

	Matrix Inverse() const
	{
		const Vector a = { M00, M10, M20 };
		const Vector b = { M01, M11, M21 };
		const Vector c = { M02, M12, M22 };
		const Vector d = { M03, M13, M23 };

		const float x = M30;
		const float y = M31;
		const float z = M32;
		const float w = M33;

		Vector s = a.Cross(b);
		Vector t = c.Cross(d);
		Vector u = a * y - b * x;
		Vector v = c * w - d * z;

		const float determinant = s.Dot(v) + t.Dot(u);
		CHECK(determinant != 0.0f);
		const float inverseDeterminant = 1.0f / determinant;
		s = s * inverseDeterminant;
		t = t * inverseDeterminant;
		u = u * inverseDeterminant;
		v = v * inverseDeterminant;

		const Vector v0 = b.Cross(v) + t * y;
		const Vector v1 = v.Cross(a) - t * x;
		const Vector v2 = d.Cross(u) + s * w;
		const Vector v3 = u.Cross(c) - s * z;

		return Matrix
		{
			v0.X, v0.Y, v0.Z, -b.Dot(t),
			v1.X, v1.Y, v1.Z,  a.Dot(t),
			v2.X, v2.Y, v2.Z, -d.Dot(s),
			v3.X, v3.Y, v3.Z,  c.Dot(s),
		};
	}

	Matrix Transpose() const
	{
		return Matrix
		{
			M00, M01, M02, M03,
			M10, M11, M12, M13,
			M20, M21, M22, M23,
			M30, M31, M32, M33,
		};
	}

	float M00, M10, M20, M30;
	float M01, M11, M21, M31;
	float M02, M12, M22, M32;
	float M03, M13, M23, M33;
};

inline const Matrix Matrix::Identity =
{
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
};
