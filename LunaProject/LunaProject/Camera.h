#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include "MathHelper.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

class Camera {
public:
	Camera();
	~Camera();

	XMVECTOR GetPosition() const;
	XMFLOAT3 GetPosition3f() const;
	void SetPosition(float x, float y, float z);
	void SetPosition(const XMFLOAT3& v);

	XMVECTOR GetRight() const;
	XMFLOAT3 GetRight3f() const;
	XMVECTOR GetUp() const;
	XMFLOAT3 GetUp3f() const;
	XMVECTOR GetLook() const;
	XMFLOAT3 GetLook3f() const;

	XMMATRIX View() const;
	XMMATRIX Proj() const;

	float GetNearZ() const;
	float GetFarZ() const;
	float GetAspect() const;
	float GetFovY() const;
	float GetFovX() const;

	float GetNearWindowHeight() const;
	float GetNearWindowWidth() const;
	float GetFarWindowHeight() const;
	float GetFarWindowWidth() const;

	void Walk(float d);
	void Strafe(float d);
	void Pitch(float angle);
	void Rotate(float angle);
	void UpdateViewMatrix();
	void SetLens(float fovY, float aspect, float zn, float zf);

private:
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	XMFLOAT3 mPos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };

	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	bool mViewDirty = true;

};

#endif