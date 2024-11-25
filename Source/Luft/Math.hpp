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
T Absolute(T x)
{
	return (x > 0) ? x : -x;
}

template<typename T>
T Clamp(T value, T min, T max)
{
	return (value > max) ? max : ((value < min) ? min : value);
}

inline uint64 Power10(uint64 x)
{
	uint64 result = 1;
	while (x)
	{
		result *= 10;
		--x;
	}
	return result;
}

inline uint64 NextMultipleOf(uint64 value, uint64 multiple)
{
	CHECK(multiple != 0);
	return (((value - 1) / multiple) + 1) * multiple;
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

	float GetMagnitudeSquared() const
	{
		return X * X + Y * Y + Z * Z;
	}

	float GetMagnitude() const
	{
		return SquareRoot(GetMagnitudeSquared());
	}

	Vector GetNormalized() const
	{
		const float length = GetMagnitude();
		CHECK(length != 0.0f);
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

inline const Vector Vector::Zero = { 0.0f, 0.0f, 0.0f };

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

	static Matrix Scale(const Vector& scale)
	{
		return Scale(scale.X, scale.Y, scale.Z);
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

	static Matrix Translation(const Vector& location)
	{
		return Translation(location.X, location.Y, location.Z);
	}

	static Matrix LookAt(Vector position, Vector direction, Vector up)
	{
		const Vector right = direction.Cross(up).GetNormalized();
		up = right.Cross(direction);
		return Matrix
		{
			right.X,              up.X,              -direction.X,            0.0f,
			right.Y,              up.Y,              -direction.Y,            0.0f,
			right.Z,              up.Z,              -direction.Z,            0.0f,
			-position.Dot(right), -position.Dot(up), position.Dot(direction), 1.0f,
		};
	}

	static Matrix Orthographic(float leftX, float rightX, float topY, float bottomY, float nearZ, float farZ)
	{
		CHECK(rightX - leftX != 0.0f && topY - bottomY != 0.0f && farZ - nearZ != 0.0f);
		return Matrix
		{
			2.0f / (rightX - leftX),              0.0f,                                 0.0f,                    0.0f,
			0.0f,                                 2.0f / (topY - bottomY),              0.0f,                    0.0f,
			0.0f,                                 0.0f,                                 -1.0f / (farZ - nearZ),  0.0f,
			-(rightX + leftX) / (rightX - leftX), -(topY + bottomY) / (topY - bottomY), -nearZ / (farZ - nearZ), 1.0f,
		};
	}

	static Matrix Perspective(float fieldOfViewYDegrees, float aspectRatio, float nearZ, float farZ)
	{
		const float inverseHeight = Tangent(0.5f * fieldOfViewYDegrees * DegreesToRadians);
		CHECK(inverseHeight != 0.0f);
		const float height = 1.0f / inverseHeight;
		CHECK(aspectRatio != 0.0f);
		const float width = height / aspectRatio;
		CHECK(nearZ - farZ != 0.0f);
		const float range = farZ / (nearZ - farZ);
		return Matrix
		{
			width, 0.0f,   0.0f,          0.0f,
			0.0f,  height, 0.0f,          0.0f,
			0.0f,  0.0f,   range,         -1.0f,
			0.0f,  0.0f,   range * nearZ, 0.0f,
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

	Vector Transform(const Vector& b) const
	{
		return Vector
		{
			M00 * b.X + M01 * b.Y + M02 * b.Z + 1.0f * M03,
			M10 * b.X + M11 * b.Y + M12 * b.Z + 1.0f * M13,
			M20 * b.X + M21 * b.Y + M22 * b.Z + 1.0f * M23,
		};
	}

	Vector TransformDirection(const Vector& b) const
	{
		return Vector
		{
			M00 * b.X + M01 * b.Y + M02 * b.Z,
			M10 * b.X + M11 * b.Y + M12 * b.Z,
			M20 * b.X + M21 * b.Y + M22 * b.Z,
		};
	}

	Matrix GetInverse() const
	{
		const float determinant = M00 * (M11 * (M22 * M33 - M23 * M32) - M21 * (M12 * M33 - M13 * M32) + M31 * (M12 * M23 - M13 * M22)) -
								  M10 * (M01 * (M22 * M33 - M23 * M32) - M21 * (M02 * M33 - M03 * M32) + M31 * (M02 * M23 - M03 * M22)) +
								  M20 * (M01 * (M12 * M33 - M13 * M32) - M11 * (M02 * M33 - M03 * M32) + M31 * (M02 * M13 - M03 * M12)) -
								  M30 * (M01 * (M12 * M23 - M13 * M22) - M11 * (M02 * M23 - M03 * M22) + M21 * (M02 * M13 - M03 * M12));
		CHECK(determinant != 0.0f);
		const float inverseDeterminant = 1.0f / determinant;

		const float i00 =  (M11 * (M22 * M33 - M32 * M23) - M21 * (M12 * M33 - M32 * M13) + M31 * (M12 * M23 - M22 * M13)) * inverseDeterminant;
		const float i01 = -(M01 * (M22 * M33 - M32 * M23) - M21 * (M02 * M33 - M32 * M03) + M31 * (M02 * M23 - M22 * M03)) * inverseDeterminant;
		const float i02 =  (M01 * (M12 * M33 - M32 * M13) - M11 * (M02 * M33 - M32 * M03) + M31 * (M02 * M13 - M12 * M03)) * inverseDeterminant;
		const float i03 = -(M01 * (M12 * M23 - M22 * M13) - M11 * (M02 * M23 - M22 * M03) + M21 * (M02 * M13 - M12 * M03)) * inverseDeterminant;

		const float i10 = -(M10 * (M22 * M33 - M32 * M23) - M20 * (M12 * M33 - M32 * M13) + M30 * (M12 * M23 - M22 * M13)) * inverseDeterminant;
		const float i11 =  (M00 * (M22 * M33 - M32 * M23) - M20 * (M02 * M33 - M32 * M03) + M30 * (M02 * M23 - M22 * M03)) * inverseDeterminant;
		const float i12 = -(M00 * (M12 * M33 - M32 * M13) - M10 * (M02 * M33 - M32 * M03) + M30 * (M02 * M13 - M12 * M03)) * inverseDeterminant;
		const float i13 =  (M00 * (M12 * M23 - M22 * M13) - M10 * (M02 * M23 - M22 * M03) + M20 * (M02 * M13 - M12 * M03)) * inverseDeterminant;

		const float i20 =  (M10 * (M21 * M33 - M31 * M23) - M20 * (M11 * M33 - M31 * M13) + M30 * (M11 * M23 - M21 * M13)) * inverseDeterminant;
		const float i21 = -(M00 * (M21 * M33 - M31 * M23) - M20 * (M01 * M33 - M31 * M03) + M30 * (M01 * M23 - M21 * M03)) * inverseDeterminant;
		const float i22 =  (M00 * (M11 * M33 - M31 * M13) - M10 * (M01 * M33 - M31 * M03) + M30 * (M01 * M13 - M11 * M03)) * inverseDeterminant;
		const float i23 = -(M00 * (M11 * M23 - M21 * M13) - M10 * (M01 * M23 - M21 * M03) + M20 * (M01 * M13 - M11 * M03)) * inverseDeterminant;

		const float i30 = -(M10 * (M21 * M32 - M31 * M22) - M20 * (M11 * M32 - M31 * M12) + M30 * (M11 * M22 - M21 * M12)) * inverseDeterminant;
		const float i31 =  (M00 * (M21 * M32 - M31 * M22) - M20 * (M01 * M32 - M31 * M02) + M30 * (M01 * M22 - M21 * M02)) * inverseDeterminant;
		const float i32 = -(M00 * (M11 * M32 - M31 * M12) - M10 * (M01 * M32 - M31 * M02) + M30 * (M01 * M12 - M11 * M02)) * inverseDeterminant;
		const float i33 =  (M00 * (M11 * M22 - M21 * M12) - M10 * (M01 * M22 - M21 * M02) + M20 * (M01 * M12 - M11 * M02)) * inverseDeterminant;

		return Matrix
		{
			i00, i10, i20, i30,
			i01, i11, i21, i31,
			i02, i12, i22, i32,
			i03, i13, i23, i33,
		};
	}

	Matrix GetTranspose() const
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

class Quaternion
{
public:
	static const Quaternion Identity;

	Quaternion() = default;

	Quaternion(float x, float y, float z, float w)
		: X(x), Y(y), Z(z), W(w)
	{
	}

	Quaternion operator*(const Quaternion& b) const
	{
		return Quaternion
		{
			W * b.X + X * b.W + Y * b.Z - Z * b.Y,
			W * b.Y - X * b.Z + Y * b.W + Z * b.X,
			W * b.Z + X * b.Y - Y * b.X + Z * b.W,
			W * b.W - X * b.X - Y * b.Y - Z * b.Z,
		};
	}

	Quaternion GetConjugate() const
	{
		return Quaternion { -X, -Y, -Z, W };
	}

	Vector Rotate(const Vector& v) const
	{
		const Quaternion vQuaternion = { v.X, v.Y, v.Z, 0.0f };
		const Quaternion rotated = *this * vQuaternion * GetConjugate();
		return Vector { rotated.X, rotated.Y, rotated.Z };
	}

	Matrix GetMatrix() const
	{
		const float ww = W * W;
		const float xx = X * X;
		const float yy = Y * Y;
		const float zz = Z * Z;

		const float wx = W * X;
		const float wy = W * Y;
		const float wz = W * Z;

		const float xy = X * Y;
		const float xz = X * Z;
		const float yz = Y * Z;

		return Matrix
		{
			ww + xx - yy - zz, 2.0f * (xy + wz),  2.0f * (xz - wy),  0.0f,
			2.0f * (xy - wz),  ww - xx + yy - zz, 2.0f * (yz + wx),  0.0f,
			2.0f * (xz + wy),  2.0f * (yz - wx),  ww - xx - yy + zz, 0.0f,
			0.0f,              0.0f,              0.0f,              1.0f,
		};
	}

	float X;
	float Y;
	float Z;
	float W;
};

inline const Quaternion Quaternion::Identity = { 0.0f, 0.0f, 0.0f, 1.0f };
