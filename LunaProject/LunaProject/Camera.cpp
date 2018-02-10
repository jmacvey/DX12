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

void Camera::LookAt(const XMFLOAT3& center, const XMFLOAT3& target, const XMFLOAT3& up)
{
	XMVECTOR T = XMLoadFloat3(&target);
	XMVECTOR Q = XMLoadFloat3(&center);
	XMVECTOR J = XMLoadFloat3(&up);
	XMVECTOR w = XMVector3Normalize(T - Q); // look
	XMVECTOR u = XMVector3Normalize(XMVector3Cross(J, w)); // right

	XMStoreFloat3(&mLook, w);
	XMStoreFloat3(&mRight, u);
	XMStoreFloat3(&mUp, XMVector3Cross(w, u)); // up
	XMStoreFloat3(&mPos, Q);
	mViewDirty = true;
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

BoundingFrustum& Camera::GetBoundingFrustum()
{
	return mCameraFrustum;
}

XMFLOAT4X4 Camera::GetProj4x4f() const
{
	return mProj;
	return XMFLOAT4X4();
}

XMMATRIX Camera::GetProj4x4() const
{
	return XMLoadFloat4x4(&mProj);
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

bool Camera::Roll(float rollTime, float dt)
{
	static float currRollTime = 0.0f;
	bool isRolling = true;

	if (currRollTime + dt >= rollTime) {
		dt = rollTime - currRollTime;
		currRollTime = 0.0f;
		isRolling = false;
	} else {
		currRollTime += dt;
	}

	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mLook), XM_2PI * (dt / rollTime));
	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mRight, XMVector3TransformNormal(XMLoadFloat3(&mRight), R));

	mViewDirty = true;
	return isRolling;
}

bool Camera::UpdateViewMatrix()
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
		
		// returns true to signify the view was updated
		return true;
	}
	
	return mViewDirty;
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

	BoundingFrustum::CreateFromMatrix(mCameraFrustum, P);
}
