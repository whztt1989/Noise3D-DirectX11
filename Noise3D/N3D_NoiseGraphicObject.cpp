
/***********************************************************************

						�ࣺNOISE Graphic Object

							������������Ⱦ

************************************************************************/
#include "Noise3D.h"


NoiseGraphicObject::NoiseGraphicObject()
{
	//there are several variables needed for one graphic object type
	//and for the time being ,there are 5 graphic object types
	//see enumerations in ".h" file
	for (UINT i = 0;i < 5;i++)
	{
		mVB_ByteSize_GPU[i]		=0;

		m_pVB_GPU[i]					=nullptr;

		mCanUpdateToGpu[i]		= FALSE;

		m_pVB_Mem[i]					= new std::vector<N_SimpleVertex>;
	}

	m_pRegionList_TriangleID_Rect		=new std::vector<N_RegionInfo>;	
	m_pRegionList_TriangleID_Ellipse	=new std::vector<N_RegionInfo>;
	m_pRegionList_LineID_Rect			=new std::vector<N_RegionInfo>;
	m_pRegionList_LineID_Ellipse			=new std::vector<N_RegionInfo>;
}


void NoiseGraphicObject::SelfDestruction()
{
	ReleaseCOM(m_pVB_GPU[0]);
	ReleaseCOM(m_pVB_GPU[1]);
	ReleaseCOM(m_pVB_GPU[2]);
	ReleaseCOM(m_pVB_GPU[3]);
	ReleaseCOM(m_pVB_GPU[4]);
}

UINT NoiseGraphicObject::AddLine3D(NVECTOR3 v1, NVECTOR3 v2, NVECTOR4 color1, NVECTOR4 color2)
{
	//.....................
	N_SimpleVertex tmpVertex;
	tmpVertex.Pos = v1;
	tmpVertex.Color = color1;
	m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_LINE_3D]->push_back(tmpVertex);

	tmpVertex.Pos = v2;
	tmpVertex.Color = color2;
	m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_LINE_3D]->push_back(tmpVertex);

	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_LINE_3D] = TRUE;
	return GetLine3DCount() - 1;
}

UINT NoiseGraphicObject::AddLine2D(NVECTOR2 v1, NVECTOR2 v2, NVECTOR4 color1, NVECTOR4 color2)
{
	//.....................
	N_SimpleVertex tmpVertex;
	tmpVertex.Pos = NVECTOR3(v1.x,v1.y,0);
	tmpVertex.Color = color1;
	m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_LINE_2D]->push_back(tmpVertex);

	tmpVertex.Pos = NVECTOR3(v2.x, v2.y, 0);
	tmpVertex.Color = color2;
	m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_LINE_2D]->push_back(tmpVertex);

	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_LINE_2D] = TRUE;
	return GetLine2DCount() - 1;
}

UINT NoiseGraphicObject::AddPoint3D(NVECTOR3 v, NVECTOR4 color)
{
	//.....................
	N_SimpleVertex tmpVertex;
	tmpVertex.Pos = v;
	tmpVertex.Color = color;
	m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_POINT_3D]->push_back(tmpVertex);

	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_POINT_3D] = TRUE;
	return GetPoint3DCount() - 1;
}

UINT NoiseGraphicObject::AddPoint2D(NVECTOR2 v, NVECTOR4 color)
{
	//.....................
	N_SimpleVertex tmpVertex;
	tmpVertex.Pos = NVECTOR3(v.x,v.y,0);
	tmpVertex.Color = color;
	m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_POINT_2D]->push_back(tmpVertex);

	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_POINT_2D] = TRUE;
	return GetPoint2DCount() - 1;
}

UINT NoiseGraphicObject::AddTriangle2D(NVECTOR2 v1, NVECTOR2 v2, NVECTOR2 v3, NVECTOR4 color1, NVECTOR4 color2, NVECTOR4 color3)
{
	//.....................
	N_SimpleVertex tmpVertex;
	tmpVertex.Pos = NVECTOR3(v1.x, v1.y, 0);
	tmpVertex.Color = color1;
	m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_TRIANGLE_2D]->push_back(tmpVertex);

	tmpVertex.Pos = NVECTOR3(v2.x, v2.y, 0);
	tmpVertex.Color = color2;
	m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_TRIANGLE_2D]->push_back(tmpVertex);

	tmpVertex.Pos = NVECTOR3(v3.x, v3.y, 0);
	tmpVertex.Color = color3;
	m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_TRIANGLE_2D]->push_back(tmpVertex);

	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_TRIANGLE_2D] = TRUE;
	return GetTriangle2DCount() - 1;
}

UINT NoiseGraphicObject::AddRectangle(NVECTOR2 vTopLeft, NVECTOR2 vBottomRight, NVECTOR4 color, UINT texID)
{
	//information to be pushed into region info list
	UINT startTriangleID = GetTriangle2DCount();

		//we will use triangle to make up complex shapes
		AddTriangle2D(
			vTopLeft,
			NVECTOR2(vBottomRight.x, vTopLeft.y),
			NVECTOR2(vTopLeft.x, vBottomRight.y),
			color,
			color,
			color);

		AddTriangle2D(
			NVECTOR2(vBottomRight.x, vTopLeft.y),
			vBottomRight,
			NVECTOR2(vTopLeft.x, vBottomRight.y),
			color,
			color,
			color);

	//information to be pushed into region info list
	UINT endTriangleID = GetTriangle2DCount() - 1;

	//write down region info (to show this block of triangles compose of a rectangle)
	N_RegionInfo regionInfo;
	regionInfo.texID = texID;
	regionInfo.startID = startTriangleID;
	regionInfo.elememtCount = endTriangleID - startTriangleID+1;
	m_pRegionList_TriangleID_Rect->push_back(regionInfo);

	//return ID of Rectangle
	return GetRectCount()-1;
}

UINT NoiseGraphicObject::AddEllipse(float a, float b, NVECTOR2 vCenter, NVECTOR4 color, UINT stepCount , UINT texID)
{
	//well, step counts cannot too small
	//stepCount =3 means 
	if (stepCount < 3) stepCount = 3;

	//check "a" (semi-major axis) ,"b" (semi-minor axis)
	if (a < 0) a = 1;
	if (b < 0) b = 1;

	//information to be pushed into region info list
	UINT startTriangleID = GetTriangle2DCount();

	//use X coord to derive Y coord
	float angleStep = MATH_PI / (stepCount-1);
	float currAngle = 0.0f;
	float nextAngle = currAngle + angleStep;

	//define 2 temporary vertices which are right on the Ellipse (interpolation)
	NVECTOR2 tmpV;
	NVECTOR2 tmpNextV;

	//loop to generate vertices
	for (UINT i = 0;i < stepCount;i++)
	{

		//this euqation can be derived from ellipse standard equation
		tmpV.x = a * cosf(currAngle);
		tmpV.y = sqrtf((a*a*b*b - b*b *tmpV.x*tmpV.x) / (a*a));

		tmpNextV.x = a * cosf(nextAngle);
		tmpNextV.y = sqrtf((a*a*b*b - b*b *tmpNextV.x*tmpNextV.x) / (a*a));

		//so we can generate triangles using "radial triangulation" (well , a word created by myself hhhhhh)
		AddTriangle2D(vCenter, tmpV+ vCenter, tmpNextV+ vCenter, color, color, color);

		//symmetric triangle about X AXIS
		AddTriangle2D(vCenter, -tmpV+ vCenter, -tmpNextV+ vCenter, color, color, color);

		//update slopes
		currAngle += angleStep;
		nextAngle += angleStep;

	}

	//information to be pushed into region info list
	UINT endTriangleID = GetTriangle2DCount() - 1;

	//write down region info (to show this block of vertices compose of a rectangle)
	N_RegionInfo regionInfo;
	regionInfo.startID = startTriangleID;
	regionInfo.elememtCount = endTriangleID - startTriangleID+1;
	m_pRegionList_TriangleID_Ellipse->push_back(regionInfo);

	return GetEllipseCount()-1;

	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_TRIANGLE_2D] = TRUE;
}

void NoiseGraphicObject::SetLine3D(UINT index, NVECTOR3 v1, NVECTOR3 v2, NVECTOR4 color1, NVECTOR4 color2)
{
	//source vertex array
	N_SimpleVertex simpleV[2];
	simpleV[0].Pos = v1;
	simpleV[0].Color = color1;
	simpleV[0].TexCoord = NVECTOR2(0, 0);
	simpleV[1].Pos = v2;
	simpleV[1].Color = color2;
	simpleV[1].TexCoord = NVECTOR2(0, 0);

	//a clearer index expression
	UINT vertexStartID = index * 2;
	UINT	 vertexCount = 2;

	//transmit an array ,the function do the error check
	mFunction_SetVertices(m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_LINE_3D], simpleV, vertexStartID, vertexCount);

	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_LINE_3D] = TRUE;
}

void NoiseGraphicObject::SetLine2D(UINT index, NVECTOR2 v1, NVECTOR2 v2, NVECTOR4 color1, NVECTOR4 color2)
{
	//source vertex array
	N_SimpleVertex simpleV[2];
	simpleV[0].Pos = NVECTOR3(v1.x,v1.y,0);
	simpleV[0].Color = color1;
	simpleV[0].TexCoord = NVECTOR2(0, 0);
	simpleV[1].Pos = NVECTOR3(v2.x, v2.y, 0);
	simpleV[1].Color = color2;
	simpleV[1].TexCoord = NVECTOR2(0, 0);

	//a clearer index expression
	UINT vertexStartID = index * 2;
	UINT	 vertexCount = 2;

	//transmit an array ,the function do the error check
	mFunction_SetVertices(m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_LINE_2D], simpleV, vertexStartID, vertexCount);

	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_LINE_2D] = TRUE;
}

void NoiseGraphicObject::SetPoint3D(UINT index, NVECTOR3 v, NVECTOR4 color)
{
	//source vertex array
	N_SimpleVertex simpleV[1];
	simpleV[0].Pos = v;
	simpleV[0].Color = color;
	simpleV[0].TexCoord = NVECTOR2(0, 0);


	//a clearer index expression
	UINT vertexStartID = index;
	UINT	 vertexCount = 1;

	//transmit an array ,the function do the error check
	mFunction_SetVertices(m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_POINT_3D], simpleV, vertexStartID, vertexCount);

	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_POINT_3D] = TRUE;
}

void NoiseGraphicObject::SetPoint2D(UINT index, NVECTOR2 v, NVECTOR4 color)
{
	//source vertex array
	N_SimpleVertex simpleV[1];
	simpleV[0].Pos = NVECTOR3(v.x, v.y, 0);
	simpleV[0].Color = color;
	simpleV[0].TexCoord = NVECTOR2(0, 0);


	//a clearer index expression
	UINT vertexStartID = index;
	UINT	 vertexCount = 1;

	//transmit an array ,the function do the error check
	mFunction_SetVertices(m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_POINT_2D], simpleV, vertexStartID, vertexCount);
	
	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_POINT_2D] = TRUE;
}

void NoiseGraphicObject::SetTriangle2D(UINT index, NVECTOR2 v1, NVECTOR2 v2, NVECTOR2 v3, NVECTOR4 color1, NVECTOR4 color2, NVECTOR4 color3)
{
	//source vertex array
	N_SimpleVertex simpleV[3];
	simpleV[0].Pos = NVECTOR3(v1.x, v1.y, 0);
	simpleV[0].Color = color1;
	simpleV[0].TexCoord = NVECTOR2(0, 0);
	simpleV[1].Pos = NVECTOR3(v2.x, v2.y, 0);
	simpleV[1].Color = color2;
	simpleV[1].TexCoord = NVECTOR2(0, 0);
	simpleV[2].Pos = NVECTOR3(v3.x, v3.y, 0);
	simpleV[2].Color = color3;
	simpleV[2].TexCoord = NVECTOR2(0, 0);


	//a clearer index expression
	UINT vertexStartID = index*3;
	UINT	 vertexCount = 3;

	//transmit an array ,the function do the error check
	mFunction_SetVertices(m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_TRIANGLE_2D], simpleV, vertexStartID, vertexCount);

	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_TRIANGLE_2D] = TRUE;
}

void NoiseGraphicObject::SetRectangle(UINT index, NVECTOR2 vTopLeft, NVECTOR2 vBottomRight, NVECTOR4 color, UINT texID)
{
	UINT triangleID1 = 0;
	UINT triangleID2 = 0;

	if (index < m_pRegionList_TriangleID_Rect->size())
	{
		//the index is used to identify triangles , so it's TRIANGLE INDEX
		triangleID1 = m_pRegionList_TriangleID_Rect->at(index).startID;
		triangleID2 = triangleID1 + 1;
	}
	else
	{
		DEBUG_MSG1("Rectangle Index Invalid !!");
		return;
	}

	//2 triangles for 1 rect
	SetTriangle2D(
		triangleID1,
		vTopLeft,
		NVECTOR2(vBottomRight.x, vTopLeft.y),
		NVECTOR2(vTopLeft.x, vBottomRight.y),
		color,
		color,
		color);

	SetTriangle2D(
		triangleID2,
		NVECTOR2(vBottomRight.x, vTopLeft.y),
		vBottomRight,
		NVECTOR2(vTopLeft.x, vBottomRight.y),
		color,
		color,
		color);

	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_TRIANGLE_2D] = TRUE;
}

void NoiseGraphicObject::SetEllipse(UINT index, float a, float b,NVECTOR2 vCenter, NVECTOR4 color, UINT texID)
{
	if (index >= m_pRegionList_TriangleID_Ellipse->size())
	{
		DEBUG_MSG1("SetEllipse: index invalid");
		return;
	}


	//define an array to use mFunction_SetVertices
	UINT triangleCount = m_pRegionList_TriangleID_Ellipse->at(index).elememtCount;

	//about why stepCount is divided by 2 ,see AddEllipse
	UINT	 stepCount = triangleCount / 2;

	UINT startTriangleID = m_pRegionList_TriangleID_Ellipse->at(index).startID;

	//check "a" (semi-major axis) ,"b" (semi-minor axis)
	if (a < 0) a = 1;
	if (b < 0) b = 1;

	//use X coord to derive Y coord
	float angleStep = MATH_PI / (stepCount-1);
	float currAngle = 0.0f;
	float nextAngle = currAngle + angleStep;

	//define 2 temporary vertices which are right on the Ellipse (interpolation)
	NVECTOR2 tmpV;
	NVECTOR2 tmpNextV;

	//loop to generate vertices
	for (UINT i = 0;i < stepCount;i++)
	{

		//this euqation can be derived from ellipse standard equation
		tmpV.x = a * cosf(currAngle);
		tmpV.y = sqrtf((a*a*b*b - b*b *tmpV.x*tmpV.x) / (a*a));

		tmpNextV.x = a * cosf(nextAngle);
		tmpNextV.y = sqrtf((a*a*b*b - b*b *tmpNextV.x*tmpNextV.x) / (a*a));

		//so we can generate triangles using "radial triangulation" (well , a word created by myself hhhhhh)
		SetTriangle2D(startTriangleID+i*2,vCenter, tmpV + vCenter, tmpNextV + vCenter, color, color, color);

		//symmetric triangle about X AXIS
		SetTriangle2D(startTriangleID+i*2+1,vCenter, -tmpV + vCenter, -tmpNextV + vCenter, color, color, color);

		//update slopes
		currAngle += angleStep;
		nextAngle += angleStep;

	}

	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_TRIANGLE_2D] = TRUE;
};

void NoiseGraphicObject::DeleteLine3D(UINT index)
{
	//to explain codes in a  clearer way
	UINT vertexStartIndex = index * 2;
	UINT vertexCount = 2;	//1 line consist of 2 vertices

	//erase function
	mFunction_EraseVertices(m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_LINE_3D], vertexStartIndex, vertexCount);
	
	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_LINE_3D] = TRUE;
}

void NoiseGraphicObject::DeleteLine2D(UINT index)
{
	//to explain codes in a  clearer way
	UINT vertexStartIndex = index * 2;
	UINT vertexCount = 2;	//1 line consist of 2 vertices

							//erase function
	mFunction_EraseVertices(m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_LINE_2D], vertexStartIndex, vertexCount);

	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_LINE_2D] = TRUE;
}

void NoiseGraphicObject::DeletePoint3D(UINT index)
{
	//to explain codes in a  clearer way
	UINT vertexStartIndex = index;
	UINT vertexCount = 1;	//1 line consist of 2 vertices

							//erase function
	mFunction_EraseVertices(m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_POINT_3D], vertexStartIndex, vertexCount);

	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_POINT_3D] = TRUE;
}

void NoiseGraphicObject::DeletePoint2D(UINT index)
{
	//to explain codes in a  clearer way
	UINT vertexStartIndex = index;
	UINT vertexCount = 1;	//1 line consist of 2 vertices

	//erase function
	mFunction_EraseVertices(m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_POINT_2D], vertexStartIndex, vertexCount);

	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_POINT_2D] = TRUE;
}

void NoiseGraphicObject::DeleteTriangle2D(UINT index)
{
	UINT vertexStartIndex = index *3;
	UINT	 vertexCount = 3;

	//erase function
	mFunction_EraseVertices(m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_TRIANGLE_2D], vertexStartIndex, vertexCount);

	//now it is allowed to update because of modification
	mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_TRIANGLE_2D] = TRUE;

}

void NoiseGraphicObject::DeleteRetangle(UINT index)
{
	//delete the index_th Rectangle

	UINT vertexStartIndex = 0;
	UINT	 vertexCount = 0;

	if (index < m_pRegionList_TriangleID_Rect->size())
	{
		vertexStartIndex = m_pRegionList_TriangleID_Rect->at(index).startID * 3;
		vertexCount = m_pRegionList_TriangleID_Rect->at(index).elememtCount*3;
		//erase from vector
		mFunction_EraseVertices(m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_TRIANGLE_2D], vertexStartIndex, vertexCount);
		
		//now it is allowed to update because of modification
		mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_TRIANGLE_2D] = TRUE;

		//also remember to delete corresponding region info
		auto iter_thisRegionInfo = m_pRegionList_TriangleID_Rect->begin();
		iter_thisRegionInfo += index;
		m_pRegionList_TriangleID_Rect->erase(iter_thisRegionInfo);
	
	}
	else
	{
		DEBUG_MSG1("Rectangle Index Invalid!!");
		return;
	}

}

void NoiseGraphicObject::DeleteEllipse(UINT index)
{
	//delete the index_th Ellipse

	UINT vertexStartIndex = 0;
	UINT	 vertexCount = 0;

	if (index < m_pRegionList_TriangleID_Ellipse->size())
	{
		//sum up triangle counts , because the step count of Ellipse might not be the same
		vertexStartIndex = m_pRegionList_TriangleID_Ellipse->at(index).startID * 3;
		vertexCount = m_pRegionList_TriangleID_Ellipse->at(index).elememtCount * 3;

		//erase from vector
		mFunction_EraseVertices(m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_TRIANGLE_2D], vertexStartIndex, vertexCount);

		//now it is allowed to update because of modification
		mCanUpdateToGpu[NOISE_GRAPHIC_OBJECT_TYPE_TRIANGLE_2D] = TRUE;

		//also remember to delete corresponding region info
		auto iter_thisRegionInfo = m_pRegionList_TriangleID_Ellipse->begin();
		iter_thisRegionInfo += index;
		m_pRegionList_TriangleID_Rect->erase(iter_thisRegionInfo);

	}
	else
	{
		DEBUG_MSG1("Ellipse Index Invalid!!");
		return;
	}
};


UINT NoiseGraphicObject::GetLine3DCount()
{
	return m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_LINE_3D]->size()/2;
};

UINT NoiseGraphicObject::GetLine2DCount()
{
	return m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_LINE_2D]->size() / 2;
};

UINT NoiseGraphicObject::GetPoint3DCount()
{
	return m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_POINT_3D]->size();
};

UINT NoiseGraphicObject::GetPoint2DCount()
{
	return m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_POINT_2D]->size();
};

UINT NoiseGraphicObject::GetTriangle2DCount()
{
	return m_pVB_Mem[NOISE_GRAPHIC_OBJECT_TYPE_TRIANGLE_2D]->size()/3;
}

UINT NoiseGraphicObject::GetRectCount()
{
	return m_pRegionList_TriangleID_Rect->size();
}

UINT NoiseGraphicObject::GetEllipseCount()
{
	return m_pRegionList_TriangleID_Ellipse->size();
};

BOOL NoiseGraphicObject::AddToRenderList()
{
	if (m_pFatherScene == NULL)
	{
		DEBUG_MSG3("NoiseLineBuffer: NoiseScene Has Not been created!", "", "");
		return FALSE;
	}
	m_pFatherScene->m_pChildRenderer->m_pRenderList_GraphicObject->push_back(this);

	//Update Data to GPU if data is not up to date , 5 object types for now
	for (UINT i = 0;i < 5;i++)
	{
		if (mCanUpdateToGpu[i])
		{
			mFunction_UpdateToGpu(i);
			mCanUpdateToGpu[i] = FALSE;
		}
	}
	return TRUE;
}


/***********************************************************************
						PRIVATE
***********************************************************************/

BOOL	NoiseGraphicObject::mFunction_InitVB(UINT objType_ID)
{
	//get VB (in memory) byte size of corresponding graphic object type
	UINT vertexCount = m_pVB_Mem[objType_ID]->size();

	//Create VERTEX BUFFER
	D3D11_BUFFER_DESC vbd;
	vbd.ByteWidth = vertexCount * sizeof(N_SimpleVertex);
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0; 
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;


	D3D11_SUBRESOURCE_DATA tmpInitData;
	tmpInitData.pSysMem = &m_pVB_Mem[objType_ID]->at(0);
	tmpInitData.SysMemPitch = 0;
	tmpInitData.SysMemSlicePitch = 0;

	//update the byte size of GPU vertex buffer
	mVB_ByteSize_GPU[objType_ID] = vbd.ByteWidth;

	//Create Buffers
	HRESULT hr = 0;
	hr = g_pd3dDevice->CreateBuffer(&vbd, &tmpInitData, &m_pVB_GPU[objType_ID]);
	HR_DEBUG(hr, "VERTEX BUFFER����ʧ��");

	return TRUE;
}

void		NoiseGraphicObject::mFunction_UpdateToGpu(UINT objType_ID)
{
	//if a GPU buffer has not been created then create a new one
	if (m_pVB_GPU[objType_ID] == NULL)
	{
		mFunction_InitVB(objType_ID);
		return;
	}

	//calculate byte size of ALL VERTEX BUFFER
	UINT mVB_Byte_Size_Memory =	m_pVB_Mem[objType_ID]->size()*	sizeof(N_SimpleVertex);

	//decide whether we should expand the capacity of GPU Vertex Buffer
	if (mVB_Byte_Size_Memory  > mVB_ByteSize_GPU[objType_ID])
	{
		//If VB was once created , destroy it to remake a new one
		if (m_pVB_GPU[objType_ID])
		{
			m_pVB_GPU[objType_ID]->Release();
			
			//create a new VB
			mFunction_InitVB(objType_ID);
		}
		else
		{
			mFunction_InitVB(objType_ID);
		}
	}

	//update new data to GPU
	g_pImmediateContext->UpdateSubresource(m_pVB_GPU[objType_ID], 0, 0, &m_pVB_Mem[objType_ID]->at(0), 0, 0);
}

void		NoiseGraphicObject::mFunction_EraseVertices(std::vector<N_SimpleVertex>* pList, UINT iVertexStartID,UINT iVertexCount)
{
	//define iterators
	std::vector<N_SimpleVertex>::iterator iter_Start;
	std::vector<N_SimpleVertex>::iterator iter_End;

	if (pList)
	{
		//initialize iterators
		iter_Start = pList->begin();
		iter_End = pList->begin();
	}
	else
	{
		DEBUG_MSG1("Noise Graphic Object : vector ptr 'pList' invalid ");
		return;
	}


	//check boundary (check the tail ,if the tail is within boundary , then it's valid
	if ((iVertexStartID + iVertexCount) >= pList->size())
	{
		DEBUG_MSG1("Noise Graphic Object : Vertex ID Out of boundary!");
		return;
	}


	//vector :: erase  ---------  delete elements in [a,b) 
	//(note that if I apply vector.erase(begin()+1,begin()+3) , then the 2nd & 3rd elements will be erased
	iter_Start += iVertexStartID;
	iter_End = iter_Start + iVertexCount;

	//delte elements
	pList->erase(iter_Start, iter_End);

}

void		NoiseGraphicObject::mFunction_SetVertices(std::vector<N_SimpleVertex>* pList, N_SimpleVertex* pSourceArray, UINT iVertexStartID, UINT iVertexCount)
{
	//validate pList
	if (!pList)
	{
		DEBUG_MSG1("Noise Graphic Object : vector ptr 'pList' invalid ");
		return;
	}

	//check boundary (check the tail ,if the tail is within boundary , then it's valid
	if (iVertexStartID + iVertexCount > pList->size())
	{
		DEBUG_MSG1("Noise Graphic Object : Vertex ID Out of boundary!");
		return;
	}
	
	//value assignment
	for (UINT i = 0;i < iVertexCount;i++)
	{
		pList->at(iVertexStartID + i) = pSourceArray[i];
	}

}

