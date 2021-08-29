#pragma once

#include "DirectX-std.h"

class Renderer;

class Camera
{
public:
	double theta, phi, radius;
	XMVECTOR origin, up;
	double aspectRatio;

	static constexpr double transformCommon = 30;
	static constexpr double transformX = -0.2;
	static constexpr double transformY = 0.2;
	static constexpr double rotateX    = 1.7;
	static constexpr double rotateY    = 0.7;
	static constexpr double scale      = -0.5;


public:
	Camera(double aspectRatio = 16.0 / 9.0): aspectRatio(aspectRatio)
	{
		theta = 2.290308;
		phi = -0.235183;;
		radius = 1880;
		origin = XMVectorSet(0, 400, 0, 1);
		up = XMVectorSet(0, 1, 0, 0);
	}

	XMMATRIX getViewMatrix()
	{
		XMVECTOR eyePos{radius * cos(phi) * cos(theta), radius * sin(phi), radius * cos(phi) * sin(theta)};
		eyePos = origin - eyePos;
		return XMMatrixLookAtLH(eyePos, origin, up);
	}

	XMMATRIX getProjectMatrix()
	{
		return XMMatrixPerspectiveFovLH(0.25 * PI, aspectRatio, 1.0f, 1000000.0f);
	}

	void Scale(double offset)
	{
		radius += offset * scale;
		radius = MathHelper::Clamp(radius, 1.0, 10000.0);
	}

	void Rotate(double xoffset, double yoffset)
	{
		float dx = -XMConvertToRadians(0.25 * static_cast<float>(xoffset));
		float dy = -XMConvertToRadians(0.25 * static_cast<float>(yoffset));

		const double eps = 1e-8;

		theta += dx * rotateX; phi += dy * rotateY;
		phi = MathHelper::Clamp(phi, eps - PI, PI - eps);
	}

	void Transform(double xoffset, double yoffset)
	{
		XMVECTOR up{0, 1, 0, 1};
		XMVECTOR right{sin(theta), 0, -cos(theta), 1};

		origin += up * yoffset * transformY * transformCommon;
		origin += right * xoffset * transformX * transformCommon;
	}

	XMVECTOR getCameraPos()
	{
		XMVECTOR eyePos{radius * cos(phi) * cos(theta), radius * sin(phi), radius * cos(phi) * sin(theta)};
		eyePos = origin - eyePos;

		return eyePos;
	}

	XMVECTOR getOrigin()
	{
		return origin;
	}
};
