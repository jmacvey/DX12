#include "stdafx.h"
#include "Camera.h"

Camera::Camera()
{
}

Camera::~Camera()
{
}

XMVECTOR Camera::GetPosition() const
{
	return XMLoadFloat3(&mPos);
}

XMFLOAT3 Camera::GetPosition3f() const
{
	return mPos;
}

void Camera::SetPosition(float x, float y, float z)
{
	mPos.x = x;
	mPos.y = y;
	mPos.z = z;
}

void Camera::SetPosition(const XMFLOAT3& v)
{
	XMStoreFloat3(&mPos, XMLoadFloat3(&v));
}

XMVECTOR Camera::GetRight() const
{
	return XMLoadFloat3(&mRight);
}

XMFLOAT3 Camera::GetRight3f() const
{
	return mRight;
}

XMVECTOR Camera::GetUp() const
{
	return XMLoadFloat3(&mUp);
}

XMFLOAT3 Camera::GetUp3f() const
{
	return mUp;
}

XMVECTOR Camera::GetLook() const
{
	return XMLoadFloat3(&mLook);
}

XMFLOAT3 Camera::GetLook3f() const
{
	return mLook;
}

XMMATRIX Camera::View() const
{
	return XMLoadFloat4x4(&mView);
}

XMMATRIX Camera::Proj() const
{
	return XMLoadFloat4x4(&mProj);
}

float Camera::GetNearZ() const
{
	return mNearZ;
}

float Camera::GetFarZ() const
{
	return mFarZ;
}

float Camera::GetAspect() const
{
	return mAspect;
}

float Camera::GetFovY() const
{
	return mFovY;
}

float Camera::GetFovX() const
{
	// formula for FovX given aspect ratio and fovY:
	// return 2.0f * atanf(mAspect * tanf(mFovY / 2.0f));
	// Note: inefficient calculation here for arctan and tan
	// Better to use nearZ distance and width:
	float halfWidth = 0.5f*GetNearWindowWidth();
	return 2.0f * atanf(halfWidth / mNearZ);
}

float Camera::GetNearWindowHeight() const
{
	return mNearWindowHeight;
}

float Camera::GetNearWindowWidth() const
{
	return mNearWindowHeight * mAspect;
}

float Camera::GetFarWindowHeight() const
{
	return mFarWindowHeight;
}

float Camera::GetFarWindowWidth() const
{
	return mFarWindowHeight * mAspect;
}

void Camera::Walk(float d)
{
	// position += d*mLook -> note that mLook is normalized
	XMVECTOR s = XMVectorReplicate(d); // <- scale
	XMVECTOR l = XMLoadFloat3(&mLook);
	XMVECTOR p = XMLoadFloat3(&mPos);
	XMStoreFloat3(&mPos, XMVectorMultiplyAdd(s, l, p));
	mViewDirty = true;
}

void Camera::Strafe(float d)
{
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR r = XMLoadFloat3(&mRight);
	XMVECTOR p = XMLoadFloat3(&mPos);
	XMStoreFloat3(&mPos, XMVectorMultiplyAdd(s, r, p));
	mViewDirty = true;
}

void Camera::Pitch(float angle)
{
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mRight), angle);
	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));
	mViewDirty = true;
}

void Camera::Rotate(float angle)
{
	XMMATRIX R = XMMatrixRotationY(angle);
	XMStoreFloat3(&mRight, XMVector3TransformNormal(XMLoadFloat3(&mRight), R));
	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));
	mViewDirty = true;
}

void Camera::UpdateViewMatrix()
{
	if (mViewDirty) {
		XMVECTOR R = XMLoadFloat3(&mRight);
		XMVECTOR U = XMLoadFloat3(&mUp);
		XMVECTOR L = XMLoadFloat3(&mLook);
		XMVECTOR P = XMLoadFloat3(&mPos);

		L = XMVector3Normalize(L);
		U = XMVector3Normalize(XMVector3Cross(L, R));
		R = XMVector3Cross(U, L);

		float x = -XMVectorGetX(XMVector3Dot(P, R));
		float y = -XMVectorGetX(XMVector3Dot(P, U));
		float z = -XMVectorGetX(XMVector3Dot(P, L));

		XMStoreFloat3(&mRight, R);
		XMStoreFloat3(&mUp, U);
		XMStoreFloat3(&mLook, L);

		mView(0, 0) = mRight.x;
		mView(1, 0) = mRight.y;
		mView(2, 0) = mRight.z;
		mView(3, 0) = x;

		mView(0, 1) = mUp.x;
		mView(1, 1) = mUp.y;
		mView(2, 1) = mUp.z;
		mView(3, 1) = y;

		mView(0, 2) = mLook.x;
		mView(1, 2) = mLook.y;
		mView(2, 2) = mLook.z;
		mView(3, 2) = z;

		mView(0, 3) = 0.0f;
		mView(1, 3) = 0.0f;
		mView(2, 3) = 0.0f;
		mView(3, 3) = 1.0f;

		mViewDirty = false;
	}
}

void Camera::SetLens(float fovY, float aspect, float zn, float zf)
{
	mFovY = fovY;
	mAspect = aspect;
	mNearZ = zn;
	mFarZ = zf;

	float twoHalfTanFovY = 2.0f*tanf(0.5f*fovY);
	mNearWindowHeight = twoHalfTanFovY*mNearZ;
	mFarWindowHeight = twoHalfTanFovY*mFarZ;

	XMMATRIX P = XMMatrixPerspectiveFovLH(mFovY, mAspect, mNearZ, mFarZ);
	XMStoreFloat4x4(&mProj, P);
}
