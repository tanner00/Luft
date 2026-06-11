#pragma once

#include "Base.hpp"
#include "Error.hpp"

inline constexpr float32 Pi = 3.14159265358979323846f;
inline constexpr float32 DegreesToRadians = Pi / 180.0f;
inline constexpr float32 RadiansToDegrees = 180.0f / Pi;

float32 SquareRoot(float32 x);

float32 Sine(float32 x);
float32 Cosine(float32 x);
float32 Tangent(float32 x);

template<typename T>
T Min(T a, T b)
{
	return a > b ? b : a;
}

template<typename T>
T Max(T a, T b)
{
	return a > b ? a : b;
}

template<typename T>
T Absolute(T x)
{
	return x > 0 ? x : -x;
}

template<typename T>
T Clamp(T value, T min, T max)
{
	return value > max ? max : value < min ? min : value;
}

inline bool IsAlmostEqual(float32 a, float32 b, float32 epsilon)
{
	const float32 difference = a - b;
	return difference < epsilon && difference > -epsilon;
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

inline bool IsPointInRectangle(float32 x, float32 y, float32 left, float32 right, float32 top, float32 bottom)
{
	return x >= left && x <= right && y >= top && y <= bottom;
}

class Vector
{
public:
	static const Vector Zero;

	Vector() = default;

	Vector(float32 x, float32 y, float32 z)
		: X(x)
		, Y(y)
		, Z(z)
	{
	}

	Vector operator+(const Vector& rhs) const
	{
		return Vector { X + rhs.X, Y + rhs.Y, Z + rhs.Z };
	}

	Vector operator-(const Vector& rhs) const
	{
		return Vector { X - rhs.X, Y - rhs.Y, Z - rhs.Z };
	}

	Vector operator*(float32 scale) const
	{
		return Vector { scale * X, scale * Y, scale * Z };
	}

	Vector operator-() const
	{
		return Vector { -X, -Y, -Z };
	}

	float32 GetMagnitudeSquared() const
	{
		return X * X + Y * Y + Z * Z;
	}

	float32 GetMagnitude() const
	{
		return SquareRoot(GetMagnitudeSquared());
	}

	Vector GetNormalized() const
	{
		const float32 length = GetMagnitude();
		CHECK(length != 0.0f);
		return Vector { X / length, Y / length, Z / length };
	}

	float32 Dot(const Vector& rhs) const
	{
		return X * rhs.X + Y * rhs.Y + Z * rhs.Z;
	}

	Vector Cross(const Vector& rhs) const
	{
		return Vector
		{
			Y * rhs.Z - Z * rhs.Y,
			Z * rhs.X - X * rhs.Z,
			X * rhs.Y - Y * rhs.X,
		};
	}

	float32 X;
	float32 Y;
	float32 Z;
};

inline const Vector Vector::Zero = { 0.0f, 0.0f, 0.0f };

class Matrix
{
public:
	static const Matrix Identity;

	static constexpr usize Dimension = 4;

	Matrix() = default;

	Matrix(float32 m00, float32 m10, float32 m20, float32 m30,
		   float32 m01, float32 m11, float32 m21, float32 m31,
		   float32 m02, float32 m12, float32 m22, float32 m32,
		   float32 m03, float32 m13, float32 m23, float32 m33)
		: M00(m00), M10(m10), M20(m20), M30(m30)
		, M01(m01), M11(m11), M21(m21), M31(m31)
		, M02(m02), M12(m12), M22(m22), M32(m32)
		, M03(m03), M13(m13), M23(m23), M33(m33)
	{
	}

	static Matrix Scale(float32 scaleX, float32 scaleY, float32 scaleZ)
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

	static Matrix Translation(float32 x, float32 y, float32 z)
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

	static Matrix Orthographic(float32 left, float32 right, float32 top, float32 bottom, float32 near, float32 far)
	{
		CHECK(right - left != 0.0f && top - bottom != 0.0f && far - near != 0.0f);
		return Matrix
		{
			2.0f / (right - left),            0.0f,                             0.0f,                 0.0f,
			0.0f,                             2.0f / (top - bottom),            0.0f,                 0.0f,
			0.0f,                             0.0f,                             -1.0f / (far - near), 0.0f,
			-(right + left) / (right - left), -(top + bottom) / (top - bottom), -near / (far - near), 1.0f,
		};
	}

	static Matrix Perspective(float32 fieldOfViewYRadians, float32 aspectRatio, float32 near)
	{
		const float32 inverseHeight = Tangent(0.5f * fieldOfViewYRadians);
		CHECK(inverseHeight != 0.0f);
		const float32 height = 1.0f / inverseHeight;
		CHECK(aspectRatio != 0.0f);
		const float32 width = height / aspectRatio;
		return Matrix
		{
			width, 0.0f,   0.0f,  0.0f,
			0.0f,  height, 0.0f,  0.0f,
			0.0f,  0.0f,   -1.0f, -1.0f,
			0.0f,  0.0f,   -near, 0.0f,
		};
	}

	static Matrix Perspective(float32 fieldOfViewYRadians, float32 aspectRatio, float32 near, float32 far)
	{
		const float32 inverseHeight = Tangent(0.5f * fieldOfViewYRadians);
		CHECK(inverseHeight != 0.0f);
		const float32 height = 1.0f / inverseHeight;
		CHECK(aspectRatio != 0.0f);
		const float32 width = height / aspectRatio;
		CHECK(near - far != 0.0f);
		const float32 range = far / (near - far);
		return Matrix
		{
			width, 0.0f,   0.0f,         0.0f,
			0.0f,  height, 0.0f,         0.0f,
			0.0f,  0.0f,   range,        -1.0f,
			0.0f,  0.0f,   range * near, 0.0f,
		};
	}

	static Matrix ReverseDepth()
	{
		return Matrix
		{
			1.0f, 0.0f, 0.0f,  0.0f,
			0.0f, 1.0f, 0.0f,  0.0f,
			0.0f, 0.0f, -1.0f, 0.0f,
			0.0f, 0.0f, 1.0f,  1.0f,
		};
	}

	float32& operator()(usize row, usize column)
	{
		CHECK(column * Dimension + row < Dimension * Dimension);
		return (&M00)[column * Dimension + row];
	}

	const float32& operator()(usize row, usize column) const
	{
		CHECK(column * Dimension + row < Dimension * Dimension);
		return (&M00)[column * Dimension + row];
	}

	Matrix operator+(const Matrix& rhs) const
	{
		return Matrix
		{
			M00 + rhs.M00, M10 + rhs.M10, M20 + rhs.M20, M30 + rhs.M30,
			M01 + rhs.M01, M11 + rhs.M11, M21 + rhs.M21, M31 + rhs.M31,
			M02 + rhs.M02, M12 + rhs.M12, M22 + rhs.M22, M32 + rhs.M32,
			M03 + rhs.M03, M13 + rhs.M13, M23 + rhs.M23, M33 + rhs.M33,
		};
	}

	Matrix operator-(const Matrix& rhs) const
	{
		return Matrix
		{
			M00 - rhs.M00, M10 - rhs.M10, M20 - rhs.M20, M30 - rhs.M30,
			M01 - rhs.M01, M11 - rhs.M11, M21 - rhs.M21, M31 - rhs.M31,
			M02 - rhs.M02, M12 - rhs.M12, M22 - rhs.M22, M32 - rhs.M32,
			M03 - rhs.M03, M13 - rhs.M13, M23 - rhs.M23, M33 - rhs.M33,
		};
	}

	Matrix operator*(const Matrix& rhs) const
	{
		return Matrix
		{
			rhs.M00 * M00 + rhs.M10 * M01 + rhs.M20 * M02 + rhs.M30 * M03,
			rhs.M00 * M10 + rhs.M10 * M11 + rhs.M20 * M12 + rhs.M30 * M13,
			rhs.M00 * M20 + rhs.M10 * M21 + rhs.M20 * M22 + rhs.M30 * M23,
			rhs.M00 * M30 + rhs.M10 * M31 + rhs.M20 * M32 + rhs.M30 * M33,

			rhs.M01 * M00 + rhs.M11 * M01 + rhs.M21 * M02 + rhs.M31 * M03,
			rhs.M01 * M10 + rhs.M11 * M11 + rhs.M21 * M12 + rhs.M31 * M13,
			rhs.M01 * M20 + rhs.M11 * M21 + rhs.M21 * M22 + rhs.M31 * M23,
			rhs.M01 * M30 + rhs.M11 * M31 + rhs.M21 * M32 + rhs.M31 * M33,

			rhs.M02 * M00 + rhs.M12 * M01 + rhs.M22 * M02 + rhs.M32 * M03,
			rhs.M02 * M10 + rhs.M12 * M11 + rhs.M22 * M12 + rhs.M32 * M13,
			rhs.M02 * M20 + rhs.M12 * M21 + rhs.M22 * M22 + rhs.M32 * M23,
			rhs.M02 * M30 + rhs.M12 * M31 + rhs.M22 * M32 + rhs.M32 * M33,

			rhs.M03 * M00 + rhs.M13 * M01 + rhs.M23 * M02 + rhs.M33 * M03,
			rhs.M03 * M10 + rhs.M13 * M11 + rhs.M23 * M12 + rhs.M33 * M13,
			rhs.M03 * M20 + rhs.M13 * M21 + rhs.M23 * M22 + rhs.M33 * M23,
			rhs.M03 * M30 + rhs.M13 * M31 + rhs.M23 * M32 + rhs.M33 * M33,
		};
	}

	Matrix operator*(float32 scale) const
	{
		return Matrix
		{
			scale * M00, scale * M10, scale * M20, scale * M30,
			scale * M01, scale * M11, scale * M21, scale * M31,
			scale * M02, scale * M12, scale * M22, scale * M32,
			scale * M03, scale * M13, scale * M23, scale * M33,
		};
	}

	Vector Transform(const Vector& rhs) const
	{
		return Vector
		{
			M00 * rhs.X + M01 * rhs.Y + M02 * rhs.Z + 1.0f * M03,
			M10 * rhs.X + M11 * rhs.Y + M12 * rhs.Z + 1.0f * M13,
			M20 * rhs.X + M21 * rhs.Y + M22 * rhs.Z + 1.0f * M23,
		};
	}

	Vector TransformDirection(const Vector& rhs) const
	{
		return Vector
		{
			M00 * rhs.X + M01 * rhs.Y + M02 * rhs.Z,
			M10 * rhs.X + M11 * rhs.Y + M12 * rhs.Z,
			M20 * rhs.X + M21 * rhs.Y + M22 * rhs.Z,
		};
	}

	Matrix GetInverse() const
	{
		const float32 determinant = M00 * (M11 * (M22 * M33 - M23 * M32) - M21 * (M12 * M33 - M13 * M32) + M31 * (M12 * M23 - M13 * M22)) -
									M10 * (M01 * (M22 * M33 - M23 * M32) - M21 * (M02 * M33 - M03 * M32) + M31 * (M02 * M23 - M03 * M22)) +
									M20 * (M01 * (M12 * M33 - M13 * M32) - M11 * (M02 * M33 - M03 * M32) + M31 * (M02 * M13 - M03 * M12)) -
									M30 * (M01 * (M12 * M23 - M13 * M22) - M11 * (M02 * M23 - M03 * M22) + M21 * (M02 * M13 - M03 * M12));
		CHECK(determinant != 0.0f);

		const float32 i00 =  (M11 * (M22 * M33 - M32 * M23) - M21 * (M12 * M33 - M32 * M13) + M31 * (M12 * M23 - M22 * M13)) / determinant;
		const float32 i01 = -(M01 * (M22 * M33 - M32 * M23) - M21 * (M02 * M33 - M32 * M03) + M31 * (M02 * M23 - M22 * M03)) / determinant;
		const float32 i02 =  (M01 * (M12 * M33 - M32 * M13) - M11 * (M02 * M33 - M32 * M03) + M31 * (M02 * M13 - M12 * M03)) / determinant;
		const float32 i03 = -(M01 * (M12 * M23 - M22 * M13) - M11 * (M02 * M23 - M22 * M03) + M21 * (M02 * M13 - M12 * M03)) / determinant;

		const float32 i10 = -(M10 * (M22 * M33 - M32 * M23) - M20 * (M12 * M33 - M32 * M13) + M30 * (M12 * M23 - M22 * M13)) / determinant;
		const float32 i11 =  (M00 * (M22 * M33 - M32 * M23) - M20 * (M02 * M33 - M32 * M03) + M30 * (M02 * M23 - M22 * M03)) / determinant;
		const float32 i12 = -(M00 * (M12 * M33 - M32 * M13) - M10 * (M02 * M33 - M32 * M03) + M30 * (M02 * M13 - M12 * M03)) / determinant;
		const float32 i13 =  (M00 * (M12 * M23 - M22 * M13) - M10 * (M02 * M23 - M22 * M03) + M20 * (M02 * M13 - M12 * M03)) / determinant;

		const float32 i20 =  (M10 * (M21 * M33 - M31 * M23) - M20 * (M11 * M33 - M31 * M13) + M30 * (M11 * M23 - M21 * M13)) / determinant;
		const float32 i21 = -(M00 * (M21 * M33 - M31 * M23) - M20 * (M01 * M33 - M31 * M03) + M30 * (M01 * M23 - M21 * M03)) / determinant;
		const float32 i22 =  (M00 * (M11 * M33 - M31 * M13) - M10 * (M01 * M33 - M31 * M03) + M30 * (M01 * M13 - M11 * M03)) / determinant;
		const float32 i23 = -(M00 * (M11 * M23 - M21 * M13) - M10 * (M01 * M23 - M21 * M03) + M20 * (M01 * M13 - M11 * M03)) / determinant;

		const float32 i30 = -(M10 * (M21 * M32 - M31 * M22) - M20 * (M11 * M32 - M31 * M12) + M30 * (M11 * M22 - M21 * M12)) / determinant;
		const float32 i31 =  (M00 * (M21 * M32 - M31 * M22) - M20 * (M01 * M32 - M31 * M02) + M30 * (M01 * M22 - M21 * M02)) / determinant;
		const float32 i32 = -(M00 * (M11 * M32 - M31 * M12) - M10 * (M01 * M32 - M31 * M02) + M30 * (M01 * M12 - M11 * M02)) / determinant;
		const float32 i33 =  (M00 * (M11 * M22 - M21 * M12) - M10 * (M01 * M22 - M21 * M02) + M20 * (M01 * M12 - M11 * M02)) / determinant;

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

	float32 M00, M10, M20, M30;
	float32 M01, M11, M21, M31;
	float32 M02, M12, M22, M32;
	float32 M03, M13, M23, M33;
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

	Quaternion(float32 x, float32 y, float32 z, float32 w)
		: X(x), Y(y), Z(z), W(w)
	{
	}

	static Quaternion AxisAngle(const Vector& axis, const float32 angleRadians)
	{
		const float32 sineTheta2 = Sine(angleRadians / 2);
		return Quaternion
		{
			sineTheta2 * axis.X,
			sineTheta2 * axis.Y,
			sineTheta2 * axis.Z,
			Cosine(angleRadians / 2),
		};
	}

	Quaternion operator*(const Quaternion& rhs) const
	{
		return Quaternion
		{
			W * rhs.X + X * rhs.W + Y * rhs.Z - Z * rhs.Y,
			W * rhs.Y - X * rhs.Z + Y * rhs.W + Z * rhs.X,
			W * rhs.Z + X * rhs.Y - Y * rhs.X + Z * rhs.W,
			W * rhs.W - X * rhs.X - Y * rhs.Y - Z * rhs.Z,
		};
	}

	Quaternion GetConjugate() const
	{
		return Quaternion { -X, -Y, -Z, W };
	}

	Quaternion GetNormalized() const
	{
		const float32 magnitude = SquareRoot(X * X + Y * Y + Z * Z + W * W);
		return Quaternion { X / magnitude, Y / magnitude, Z / magnitude, W / magnitude };
	}

	Vector Rotate(const Vector& v) const
	{
		const Quaternion vQuaternion = { v.X, v.Y, v.Z, 0.0f };
		const Quaternion rotated = *this * vQuaternion * GetConjugate();
		return Vector { rotated.X, rotated.Y, rotated.Z };
	}

	Matrix ToMatrix() const
	{
		const float32 ww = W * W;
		const float32 xx = X * X;
		const float32 yy = Y * Y;
		const float32 zz = Z * Z;

		const float32 wx = W * X;
		const float32 wy = W * Y;
		const float32 wz = W * Z;

		const float32 xy = X * Y;
		const float32 xz = X * Z;
		const float32 yz = Y * Z;

		return Matrix
		{
			ww + xx - yy - zz, 2.0f * (xy + wz),  2.0f * (xz - wy),  0.0f,
			2.0f * (xy - wz),  ww - xx + yy - zz, 2.0f * (yz + wx),  0.0f,
			2.0f * (xz + wy),  2.0f * (yz - wx),  ww - xx - yy + zz, 0.0f,
			0.0f,              0.0f,              0.0f,              1.0f,
		};
	}

	float32 X;
	float32 Y;
	float32 Z;
	float32 W;
};

inline const Quaternion Quaternion::Identity = { 0.0f, 0.0f, 0.0f, 1.0f };

inline void DecomposeTransform(const Matrix& transform, Vector* translation, Quaternion* orientation, Vector* scale)
{
	if (translation)
	{
		translation->X = transform(0, 3);
		translation->Y = transform(1, 3);
		translation->Z = transform(2, 3);
	}

	const float32 determinant = transform(0, 0) * (transform(1, 1) * transform(2, 2) - transform(1, 2) * transform(2, 1)) -
								transform(1, 0) * (transform(0, 1) * transform(2, 2) - transform(2, 1) * transform(0, 2)) +
								transform(2, 0) * (transform(0, 1) * transform(1, 2) - transform(1, 1) * transform(0, 2));
	const float32 sign = determinant < 0.0f ? -1.0f : 1.0f;

	const Vector transformScale =
	{
		SquareRoot(transform(0, 0) * transform(0, 0) + transform(1, 0) * transform(1, 0) + transform(2, 0) * transform(2, 0)) * sign,
		SquareRoot(transform(0, 1) * transform(0, 1) + transform(1, 1) * transform(1, 1) + transform(2, 1) * transform(2, 1)) * sign,
		SquareRoot(transform(0, 2) * transform(0, 2) + transform(1, 2) * transform(1, 2) + transform(2, 2) * transform(2, 2)) * sign,
	};
	if (scale)
	{
		*scale = transformScale;
	}

	if (orientation)
	{
		const float32 inverseScaleX = transformScale.X == 0.0f ? 0.0f : 1.0f / transformScale.X;
		const float32 inverseScaleY = transformScale.Y == 0.0f ? 0.0f : 1.0f / transformScale.Y;
		const float32 inverseScaleZ = transformScale.Z == 0.0f ? 0.0f : 1.0f / transformScale.Z;

		const float32 transformOrientation[3][3] =
		{
			{ transform(0, 0) * inverseScaleX, transform(0, 1) * inverseScaleY, transform(0, 2) * inverseScaleZ },
			{ transform(1, 0) * inverseScaleX, transform(1, 1) * inverseScaleY, transform(1, 2) * inverseScaleZ },
			{ transform(2, 0) * inverseScaleX, transform(2, 1) * inverseScaleY, transform(2, 2) * inverseScaleZ },
		};

		const uint32 control = transformOrientation[2][2] < 0 ? transformOrientation[0][0] > transformOrientation[1][1] ? 0 : 1
															  : transformOrientation[0][0] < -transformOrientation[1][1] ? 2 : 3;
		const float32 sign1 = control & 2 ? -1.0f : 1.0f;
		const float32 sign2 = control & 1 ? -1.0f : 1.0f;
		const float32 sign3 = control - 1 & 2 ? -1.0f : 1.0f;

		const float32 t = 1.0f - (sign3 * transformOrientation[0][0]) - (sign2 * transformOrientation[1][1]) - (sign1 * transformOrientation[2][2]);
		const float32 s = 0.5f / SquareRoot(t);

		float32* orientationComponents = reinterpret_cast<float32*>(orientation);
		orientationComponents[control ^ 0] = s * t;
		orientationComponents[control ^ 1] = s * (transformOrientation[1][0] + sign1 * transformOrientation[0][1]);
		orientationComponents[control ^ 2] = s * (transformOrientation[0][2] + sign2 * transformOrientation[2][0]);
		orientationComponents[control ^ 3] = s * (transformOrientation[2][1] + sign3 * transformOrientation[1][2]);
	}
}
