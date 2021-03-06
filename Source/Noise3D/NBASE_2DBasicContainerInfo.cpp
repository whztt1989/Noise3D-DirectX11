
/***********************************************************************

							class��NOISE GUI basic contain Info

					Description : encapsule basic container information like
					position, size ..... this is a base class to be inherited.

************************************************************************/

#include "Noise3D.h"

using namespace Noise3D;

IBasicContainerInfo::IBasicContainerInfo()
{
	mPosZ = 1.0f;//the deepest
	mIsEnabled = TRUE;
	m_pPositionCenter = new NVECTOR2(50,30);
	mWidth = 100;
	mHeight = 60;
	m_pBasicColor = new NVECTOR4(0.3f,0.3f,1.0f,1.0f);
}

void IBasicContainerInfo::SetDiagonal(float x_topLeft, float y_topLeft,float x_bottomRight,float y_bottomRight)
{
	//should I check the sign of (posX-x) and (posY - y) ????
	if (x_bottomRight > x_topLeft && y_bottomRight > y_topLeft)
	{
		mWidth = x_bottomRight - x_topLeft;
		mHeight = y_bottomRight - y_topLeft;
		m_pPositionCenter->x = (x_bottomRight + x_topLeft) / 2.0f;
		m_pPositionCenter->y = (y_bottomRight + y_topLeft) / 2.0f;
	}
};

void IBasicContainerInfo::SetDiagonal(NVECTOR2 v_topLeft, NVECTOR2 v_bottomRight)
{
	//use another overloaded function
	SetDiagonal(v_topLeft.x, v_topLeft.y, v_bottomRight.x, v_bottomRight.y);
}

NVECTOR2 IBasicContainerInfo::GetTopLeft()
{
	return (*m_pPositionCenter)-NVECTOR2(mWidth/2,mHeight/2);
}

NVECTOR2 IBasicContainerInfo::GetBottomRight()
{
	return  (*m_pPositionCenter) + NVECTOR2(mWidth / 2, mHeight / 2);
};

void IBasicContainerInfo::SetCenterPos(float x, float y)
{
	m_pPositionCenter->x = x;
	m_pPositionCenter->y = y;
};

void IBasicContainerInfo::SetCenterPos(NVECTOR2 v)
{
	*(m_pPositionCenter) = v;
}

void IBasicContainerInfo::Move(float relativeX, float relativeY)
{
	SetCenterPos(m_pPositionCenter->x + relativeX, m_pPositionCenter->y + relativeY);
}

void IBasicContainerInfo::Move(NVECTOR2 relativePos)
{
	Move(relativePos.x, relativePos.y);
}

NVECTOR2 IBasicContainerInfo::GetCenterPos()
{
	return *m_pPositionCenter;
}

void IBasicContainerInfo::SetWidth(float w)
{
	//well , a button can't be that big
	if (w > 1 && w < 4096) mWidth = w;
};

void IBasicContainerInfo::SetHeight(float h)
{
	//well , a button can't be that big
	if (h > 1 && h < 4096)	mHeight = h;
}

float IBasicContainerInfo::GetWidth()
{
	return mWidth;
}

float IBasicContainerInfo::GetHeight()
{
	return mHeight;
}


BOOL IBasicContainerInfo::IsPointInContainer(NVECTOR2 v)
{
	return gFunction_IsPointInRect2D(
		v,
		*m_pPositionCenter - NVECTOR2(mWidth / 2, mHeight / 2),
		*m_pPositionCenter + NVECTOR2(mWidth / 2, mHeight / 2)
		);
};

void IBasicContainerInfo::SetBasicColor(NVECTOR4 c)
{
	*m_pBasicColor = c;
}

NVECTOR4 IBasicContainerInfo::GetBasicColor()
{
	return *m_pBasicColor;
}

void IBasicContainerInfo::SetEnabled(BOOL isEnabled)
{
	mIsEnabled = isEnabled;
}

BOOL IBasicContainerInfo::IsEnabled()
{
	return mIsEnabled;
}

void IBasicContainerInfo::SetPosZ(float posZ)
{
	//clamp to 0~1
	mPosZ = posZ >= 0.0 ? (posZ <= 1.0f ? posZ : 1.0f) : 0.0f;
}

float IBasicContainerInfo::GetPosZ()
{
	return mPosZ;
}





