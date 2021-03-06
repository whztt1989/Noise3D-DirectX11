
/***********************************************************************

							class��NOISE texture Manger

							texture management/modification

************************************************************************/

#include "Noise3D.h"

using namespace Noise3D;

ITextureManager::ITextureManager()
{
	m_pFatherScene = NULL;
	m_pTextureObjectList = new std::vector<N_TextureObject>;
	m_pTextureObjectHashTable = new std::unordered_map<std::string, UINT>;
};

void	 ITextureManager::Destroy()
{
	for (auto texObj : *m_pTextureObjectList)
	{
		texObj.mPixelBuffer.clear();
		ReleaseCOM(texObj.m_pSRV);
	}
}

BOOL ITextureManager::SetPixel_SysMem(UINT texID, UINT x, UINT y,const  NVECTOR4& color)
{
	UINT validatedTexID = ValidateIndex(texID, NOISE_TEXTURE_TYPE_COMMON);
	
	if (validatedTexID!=NOISE_MACRO_INVALID_TEXTURE_ID)
	{
		//this texture is not created with a SysMem Copy
		if (!m_pTextureObjectList->at(validatedTexID).mIsPixelBufferInMemValid)
		{
			DEBUG_MSG1("Set Pixel : Texture didn't have a copy in memory!!");
			return FALSE;
		}

		//locate pixel in memory according to (x,y) coord
		UINT textureWidth = GetTextureWidth(validatedTexID);
		UINT textureHeight = GetTextureHeight(validatedTexID);

		//(x,y)must be in the rect of (width x height)
		if(x>=0 && x<textureWidth && y>=0 && y<textureHeight)
		{
			UINT pixelIndex = mFunction_GetPixelIndexFromXY(x, y, GetTextureWidth(validatedTexID));
			m_pTextureObjectList->at(validatedTexID).mPixelBuffer.at(pixelIndex) = color;
			return TRUE;
		}
		else//X,Y Invalid or Out Of Range
		{
			DEBUG_MSG1("SetPixel : Point out of range!");
			return FALSE;
		}
	}
	else
	{
		DEBUG_MSG1("SetPixel : Texture ID or Texture Type invalid !!");
		return FALSE;
	}
}

NVECTOR4 ITextureManager::GetPixel_SysMem(UINT texID, UINT x, UINT y)
{
	NVECTOR4 outColor(0, 0, 0, 1.0f);

	UINT validatedTexID = ValidateIndex(texID, NOISE_TEXTURE_TYPE_COMMON);

	if (validatedTexID != NOISE_MACRO_INVALID_TEXTURE_ID)
	{
		//this texture is not created with a SysMem Copy
		if (!m_pTextureObjectList->at(validatedTexID).mIsPixelBufferInMemValid)
		{
			DEBUG_MSG1("GetPixel : Texture didn't have a copy in memory!!");
			return outColor;
		}

		//locate pixel in memory according to (x,y) coord
		UINT textureWidth = GetTextureWidth(validatedTexID);
		UINT textureHeight = GetTextureHeight(validatedTexID);

		//(x,y)must be in the rect of (width x height)
		if (x >= 0 && x < textureWidth && y >= 0 && y < textureHeight)
		{
			UINT pixelIndex = mFunction_GetPixelIndexFromXY(x, y, GetTextureWidth(validatedTexID));
			outColor = m_pTextureObjectList->at(validatedTexID).mPixelBuffer.at(pixelIndex);
		}
		else//X,Y Invalid or Out Of Range
		{
			DEBUG_MSG1("GetPixel : Point out of range!");
			return outColor;
		}
	}
	else
	{
		DEBUG_MSG1("GetPixel : Texture ID or Texture Type invalid !!");
		return outColor;
	}

	return outColor;
}

BOOL ITextureManager::UpdateTextureDataToGraphicMemory(UINT texID)
{
	UINT validatedTexID = ValidateIndex(texID, NOISE_TEXTURE_TYPE_COMMON);

	if (validatedTexID != NOISE_MACRO_INVALID_TEXTURE_ID)
	{
		if (m_pTextureObjectList->at(validatedTexID).mIsPixelBufferInMemValid)
		{
			UINT picWidth = GetTextureWidth(validatedTexID);

			//after modifying buffer in memory, update to GPU
			ID3D11Resource* pTmpRes;
			//resource reference count will increase ,so remember to release
			//so getResource() actually gets a interface to the resource BOUND to the view.
			m_pTextureObjectList->at(validatedTexID).m_pSRV->GetResource(&pTmpRes);

			//update resource which is bound to the corresponding SRV
			g_pImmediateContext->UpdateSubresource(
				pTmpRes,
				0,
				NULL,
				&m_pTextureObjectList->at(validatedTexID).mPixelBuffer.at(0),
				picWidth * NOISE_MACRO_DEFAULT_COLOR_BYTESIZE,
				NULL);

			ReleaseCOM(pTmpRes);
		}
		else
		{
			//mIsPixelBufferInMemValid==FALSE
			DEBUG_MSG1("UpdateTextureToGraphicMemory : Texture didn't have a copy in System Memory!");
			return FALSE;
		}
	}
	else
	{
		//texID = ==Noise_macro_invalid_Texture_ID
		DEBUG_MSG1("UpdateTextureToGraphicMemory : Texture ID invalid!!");
		return FALSE;
	}

	return TRUE;
}

inline BOOL ITextureManager::UpdateTextureDataToGraphicMemory(std::string texName)
{
	return UpdateTextureDataToGraphicMemory(GetTextureID(texName));
}

//--------------------------------TEXTURE CREATION-----------------------------
UINT ITextureManager::CreatePureColorTexture(std::string texName, UINT pixelWidth, UINT pixelHeight, NVECTOR4 color, BOOL keepCopyInMemory)
{
	//create New Texture Object
	HRESULT hr = S_OK;
	UINT newTexIndex = NOISE_MACRO_INVALID_TEXTURE_ID;
	std::string tmpStringTextureName(texName);

	//we must check if new name has been used
	if(GetTextureID(texName)!=NOISE_MACRO_INVALID_TEXTURE_ID)
	{
			DEBUG_MSG1("CreateTextureFromFile : Texture name has been used!!");
			return NOISE_MACRO_INVALID_TEXTURE_ID;//invalid
	}

#pragma region CreateTex2D&SRV

	//a temporary Texture Object
	N_TextureObject tmpTexObj;
	tmpTexObj.mIsPixelBufferInMemValid = keepCopyInMemory;
	tmpTexObj.mTexName = tmpStringTextureName;
	tmpTexObj.mTextureType = NOISE_TEXTURE_TYPE_COMMON;

	//assign a the same NVECTOR4 to vector
	tmpTexObj.mPixelBuffer.resize(pixelWidth*pixelHeight,color);
	//tmpTexObj.mPixelBuffer.assign(pixelHeight*pixelWidth, color);????

	//texture2D desc (create a default usage texture)
	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width = pixelWidth;//determined by the loaded picture
	texDesc.Height = pixelHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;//allow update subresource
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = NULL;
	texDesc.MiscFlags = NULL;

	//SRV desc
	D3D11_SHADER_RESOURCE_VIEW_DESC SRViewDesc;
	SRViewDesc.Format = texDesc.Format;
	SRViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRViewDesc.TextureCube.MipLevels = texDesc.MipLevels;
	SRViewDesc.TextureCube.MostDetailedMip = 0;

	//init data
	D3D11_SUBRESOURCE_DATA texInitDataDesc;
	texInitDataDesc.pSysMem = &tmpTexObj.mPixelBuffer.at(0);
	texInitDataDesc.SysMemPitch = (texDesc.Width) * NOISE_MACRO_DEFAULT_COLOR_BYTESIZE;//bytesize for 1 row
	texInitDataDesc.SysMemSlicePitch = 0;//available for tex3D

	 //create texture2D first 
	ID3D11Texture2D* pTmpTexture2D;
	hr = g_pd3dDevice11->CreateTexture2D(&texDesc, &texInitDataDesc, &pTmpTexture2D);
	//ReleaseCOM(g_pd3dDevice11);
	if (FAILED(hr))
	{
		ReleaseCOM(pTmpTexture2D);
		DEBUG_MSG1("CreateTextureFromFile : Create ID3D11Texture2D failed!");
		return NOISE_MACRO_INVALID_TEXTURE_ID;
	}

	//Create SRV from texture 2D (to a tmp textureObject)
	hr = g_pd3dDevice11->CreateShaderResourceView(pTmpTexture2D, &SRViewDesc, &tmpTexObj.m_pSRV);
	//ReleaseCOM(g_pd3dDevice11);
	if (FAILED(hr))
	{
		ReleaseCOM(pTmpTexture2D);
		DEBUG_MSG1("CreateTextureFromFile : Create ID3D11SRV failed!");
		return NOISE_MACRO_INVALID_TEXTURE_ID;
	}


	//clear tmp interfaces
	ReleaseCOM(pTmpTexture2D);

	//clear Memory of the picture
	if (!keepCopyInMemory)tmpTexObj.mPixelBuffer.clear();

#pragma endregion CreateTex2D&SRV

	//at last push back a new texture object
	m_pTextureObjectList->push_back(tmpTexObj);
	newTexIndex = m_pTextureObjectList->size() - 1;

	//don't forget to add "name-index" map to hash table to accelerate element searching
	m_pTextureObjectHashTable->insert(std::make_pair(tmpTexObj.mTexName, newTexIndex));

	return newTexIndex;
};

UINT ITextureManager::CreateTextureFromFile(NFilePath filePath, std::string texName, BOOL useDefaultSize, UINT pixelWidth, UINT pixelHeight,BOOL keepCopyInMemory)
{
	if (keepCopyInMemory)
	{
		return mFunction_CreateTextureFromFile_KeepACopyInMemory(filePath, texName, useDefaultSize, pixelWidth, pixelHeight);
	}
	else
	{
		return mFunction_CreateTextureFromFile_DirectlyLoadToGpu(filePath, texName, useDefaultSize, pixelWidth, pixelHeight);
	}
	return NOISE_MACRO_INVALID_TEXTURE_ID;
}

UINT ITextureManager::CreateCubeMapFromFiles(NFilePath fileName[6], std::string cubeTextureName, NOISE_CUBEMAP_SIZE faceSize)
{
	HRESULT hr = S_OK;
	UINT newTexIndex = NOISE_MACRO_INVALID_TEXTURE_ID;

#pragma region LoadDataToBufferArray
	//create temporary textures , so we should mark the ID to delete it later
	UINT tmpTexID[6];
	UINT cubeMapWidth = 0;

	//...what size to use
	switch (faceSize)
	{
	case NOISE_CUBEMAP_SIZE_64x64:
		cubeMapWidth = 64;
		break;
	case NOISE_CUBEMAP_SIZE_128x128:
		cubeMapWidth = 128;
		break;
	case NOISE_CUBEMAP_SIZE_256x256:
		cubeMapWidth = 256;
		break;
	case NOISE_CUBEMAP_SIZE_512x512:
		cubeMapWidth = 512;
		break;
	case NOISE_CUBEMAP_SIZE_1024x1024:
		cubeMapWidth = 1024;
		break;
	};

	//create temporary textures
	for (UINT i = 0; i < 6;i++)
	{
		std::stringstream texName;
		texName << "cubeMapTmpTexure" << i;
		tmpTexID[i] = CreateTextureFromFile(
			fileName[i], 
			texName.str().c_str(), 
			FALSE,
			cubeMapWidth, 
			cubeMapWidth, 
			TRUE);

		//one of the texture failed loading
		if (tmpTexID[i] == NOISE_MACRO_INVALID_TEXTURE_ID)
		{
			for (UINT j = 0;j <= i;j++)
			{
				//delete previously created temporary textures
				DeleteTexture(tmpTexID[j]);
			}

			DEBUG_MSG1("NoiseTexMgr :CreateCubeMapFromFiles:create face from file failed ! face ID : ");
			return NOISE_MACRO_INVALID_TEXTURE_ID;
		}
	}


	//combine 4 buffers
	std::vector<NVECTOR4> pixelBuffer[6];
	for (UINT i = 0;i < 6;i++)
	{
		auto srcPartialBuffer = m_pTextureObjectList->at(tmpTexID[i]).mPixelBuffer;
		//assign values for each buffer
		pixelBuffer[i].assign(srcPartialBuffer.begin(), srcPartialBuffer.end());
	}

	//delete temporary textures after copying data to pixelBuffer(s)
	DeleteTexture(tmpTexID[5]);
	DeleteTexture(tmpTexID[4]);
	DeleteTexture(tmpTexID[3]);
	DeleteTexture(tmpTexID[2]);
	DeleteTexture(tmpTexID[1]);
	DeleteTexture(tmpTexID[0]);
#pragma endregion LoadDataToBufferArray


#pragma region CreateCubeMap
	//we must check if new texture name has been used
	std::string cubemapNameString(cubeTextureName);
	for (auto t : *m_pTextureObjectList)
	{
		if (t.mTexName == cubemapNameString)
		{
			DEBUG_MSG1("NoiseTexMgr:CreateCubeMapFromFiles : Texture name has been used!!");
			return NOISE_MACRO_INVALID_TEXTURE_ID;//invalid
		}
	}


	//cube map is similar to texture arrays
	D3D11_TEXTURE2D_DESC texDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC SRViewDesc;
	D3D11_SUBRESOURCE_DATA texInitDataDesc[6];
	for (UINT i = 0;i < 6;i++)
	{
		texInitDataDesc[i].pSysMem = &pixelBuffer[i].at(0);
		texInitDataDesc[i].SysMemPitch = cubeMapWidth * NOISE_MACRO_DEFAULT_COLOR_BYTESIZE;
		texInitDataDesc[i].SysMemSlicePitch = 0;
	}

	//2 description need to be updated
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width = cubeMapWidth;
	texDesc.Height = cubeMapWidth;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 6;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.CPUAccessFlags = 0;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	ZeroMemory(&SRViewDesc, sizeof(SRViewDesc));
	SRViewDesc.Format = texDesc.Format;
	SRViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	SRViewDesc.TextureCube.MipLevels = texDesc.MipLevels;


	//create id3d11texture2D
	ID3D11Texture2D* pCubeMapTexture2D;
	hr = g_pd3dDevice11->CreateTexture2D(&texDesc, &texInitDataDesc[0], &pCubeMapTexture2D);
	if (FAILED(hr))
	{
		DEBUG_MSG1("NoiseTexMgr:CreateCubeMapFromFiles : Create new cube map failed!!");
		return NOISE_MACRO_INVALID_TEXTURE_ID;
	}

	
	//we disable texture modification of Cube Map for the time being...
	N_TextureObject tmpTexObj;
	tmpTexObj.mTexName = cubemapNameString;
	tmpTexObj.mIsPixelBufferInMemValid = FALSE;
	tmpTexObj.mTextureType = NOISE_TEXTURE_TYPE_CUBEMAP;

	//create SRV from texture2D
	hr = g_pd3dDevice11->CreateShaderResourceView(
		pCubeMapTexture2D,
		&SRViewDesc,
		&tmpTexObj.m_pSRV);

	if (FAILED(hr))
	{
		//new temporary tex obj should be dumped
		DEBUG_MSG1("NoiseTexMgr:CreateCubeMapFromFiles : Create SRV failed!!");
		return NOISE_MACRO_INVALID_TEXTURE_ID;
	}

#pragma endregion CreateCubeMap


	//by now ,we have successfully created a cube map Texture Object
	m_pTextureObjectList->push_back(tmpTexObj);
	newTexIndex = m_pTextureObjectList->size() - 1;

	//don't forget to add "name-index" map to hash table to accelerate element searching
	m_pTextureObjectHashTable->insert(std::make_pair(tmpTexObj.mTexName, newTexIndex));

	return newTexIndex;
}

UINT ITextureManager::CreateCubeMapFromDDS(NFilePath dds_FileName, std::string cubeTextureName, NOISE_CUBEMAP_SIZE faceSize)
{
	BOOL isFileNameValid;
	for (UINT i = 0; i < 6;i++)
	{
		//try opening a file
		isFileNameValid = std::fstream(dds_FileName).good();

		//check if  one of the texture id is illegal
		if (!isFileNameValid)
		{
			DEBUG_MSG1("Noise Tex Mgr :CreateCubeTextureFromDDS : file not exist!! ; Index : ");
			return NOISE_MACRO_INVALID_TEXTURE_ID;
		}
	}

#pragma region CreateSRVFromDDS

	//some settings about loading image
	D3DX11_IMAGE_LOAD_INFO loadInfo;

	//we must check if user want to use default image size
	switch(faceSize)
	{
	case NOISE_CUBEMAP_SIZE_64x64:
		loadInfo.Width = 64;
		loadInfo.Height = 64;
		break;
	case NOISE_CUBEMAP_SIZE_128x128:
		loadInfo.Width = 128;
		loadInfo.Height = 128;
		break;
	case NOISE_CUBEMAP_SIZE_256x256:
		loadInfo.Width = 256;
		loadInfo.Height = 256;
		break;
	case NOISE_CUBEMAP_SIZE_512x512:
		loadInfo.Width = 512;
		loadInfo.Height = 512;
		break;
	case NOISE_CUBEMAP_SIZE_1024x1024:
		loadInfo.Width = 1024;
		loadInfo.Height = 1024;
		break;
	};
	

	//continue filling the settings
	loadInfo.Filter = D3DX11_FILTER_LINEAR;
	loadInfo.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	loadInfo.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	//create New Texture Object
	HRESULT hr = S_OK;
	N_TextureObject tmpTexObj;
	std::string tmpString(cubeTextureName);

	//we must check if new name has been used
	for (auto t : *m_pTextureObjectList)
	{
		if (t.mTexName == tmpString)
		{
			DEBUG_MSG1("CreateCubeMapFromDDS : Texture name has been used!!");
			return NOISE_MACRO_INVALID_TEXTURE_ID;//invalid
		}
	}

	//then endow a pointer to new SRV
	D3DX11CreateShaderResourceViewFromFileA(
		g_pd3dDevice11,
		dds_FileName.c_str(),
		&loadInfo,
		nullptr,
		&tmpTexObj.m_pSRV,
		&hr
		);
	//ReleaseCOM(g_pd3dDevice11);

	//................
	if (FAILED(hr))
	{
		DEBUG_MSG1("CreateCubeMapFromDDS : Create SRV failed!");
		return NOISE_MACRO_INVALID_TEXTURE_ID;
	}

	//set texture object attribute
	tmpTexObj.mTexName = tmpString;
	tmpTexObj.mIsPixelBufferInMemValid = FALSE;
	tmpTexObj.mTexName = tmpString;
	tmpTexObj.mTextureType = NOISE_TEXTURE_TYPE_CUBEMAP;

#pragma endregion CreateSRVFromDDS


	//by now ,we have successfully created a cube map Texture Object
	m_pTextureObjectList->push_back(tmpTexObj);
	UINT newTexIndex = m_pTextureObjectList->size()-1;

	//don't forget to add "name-index" map to hash table to accelerate element searching
	m_pTextureObjectHashTable->insert(std::make_pair(tmpTexObj.mTexName, newTexIndex));

	return newTexIndex;
}


BOOL ITextureManager::ConvertTextureToGreyMap(UINT texID)
{
	//these factor combination (0.3,0.59,0.11) is based on characteristic of human eye  and 
	//electromagnetic waves (of visible light)
	return ConvertTextureToGreyMapEx(texID,0.3f,0.59f,0.11f);
}

BOOL ITextureManager::ConvertTextureToGreyMap(std::string texName)
{
	return ConvertTextureToGreyMap(GetTextureID(texName));
}

BOOL ITextureManager::ConvertTextureToGreyMapEx(UINT texID, float factorR, float factorG, float factorB)
{
	UINT validatedTexID = ValidateIndex(texID, NOISE_TEXTURE_TYPE_COMMON);
	if (validatedTexID == NOISE_MACRO_INVALID_TEXTURE_ID)
	{
		DEBUG_MSG1("ConvertTextureToGreyMap:texID Out of Range or Type Invalid!");
		return FALSE;
	}

	//only the texture created both in gpu & memory can be modified 
	//( actually the copy in mem will be modified)
	if (!m_pTextureObjectList->at(validatedTexID).mIsPixelBufferInMemValid)
	{
		DEBUG_MSG1("ConvertTextureToGreyMap:Only Textures that keep a copy in memory can be converted ! ");
		return FALSE;
	}

	//Get Dimension info
	D3D11_TEXTURE2D_DESC textureDesc;
	ID3D11Texture2D* pTex2D;
	m_pTextureObjectList->at(validatedTexID).m_pSRV->GetResource((ID3D11Resource**)&pTex2D);
	pTex2D->GetDesc(&textureDesc);

	UINT picWidth = textureDesc.Width;
	UINT picHeight = textureDesc.Height;

	ReleaseCOM(pTex2D);

	//for every pixel of the copy in memory ,
	//convert their RGBs to same value under some rules
	for (auto& c : m_pTextureObjectList->at(validatedTexID).mPixelBuffer)
	{
		float greyScale = factorR *c.x + factorG*c.y + factorB*c.z;
		c = NVECTOR4(greyScale, greyScale, greyScale, c.w);
	}

	//after modifying buffer in memory, update to GPU
	ID3D11Resource* pTmpRes;
	//resource reference count will increase ,so remember to release
	//so getResource() actually gets a interface to the resource BOUND to the view.
	m_pTextureObjectList->at(validatedTexID).m_pSRV->GetResource(&pTmpRes);

	//update resource which is bound to the corresponding SRV
	g_pImmediateContext->UpdateSubresource(
		pTmpRes,
		0,
		NULL,
		&m_pTextureObjectList->at(validatedTexID).mPixelBuffer.at(0),
		picWidth * NOISE_MACRO_DEFAULT_COLOR_BYTESIZE,
		NULL);

	ReleaseCOM(pTmpRes);

	return TRUE;
}

BOOL ITextureManager::ConvertTextureToGreyMapEx(std::string texName, float factorR, float factorG, float factorB)
{
	return ConvertTextureToGreyMapEx(GetTextureID(texName),factorR,factorG,factorB);
}

BOOL ITextureManager::ConvertHeightMapToNormalMap(UINT texID, float bumpScaleFactor)
{
	UINT validatedTexID = ValidateIndex(texID, NOISE_TEXTURE_TYPE_COMMON);
	if (validatedTexID == NOISE_MACRO_INVALID_TEXTURE_ID)
	{
		DEBUG_MSG1("ConvertTextureToNormalMap:texID Out of Range or Type Invalid!");
		return FALSE;
	}

	//only the texture created both in gpu & memory can be modified 
	//( actually the copy in mem will be modified)
	if (!m_pTextureObjectList->at(validatedTexID).mIsPixelBufferInMemValid)
	{
		DEBUG_MSG1("ConvertTextureToNormalMap:Only Textures that keep a copy in memory can be converted ! ");
		return FALSE;
	}

	//Get Dimension info
	D3D11_TEXTURE2D_DESC textureDesc;
	ID3D11Texture2D* pTex2D;
	m_pTextureObjectList->at(validatedTexID).m_pSRV->GetResource((ID3D11Resource**)&pTex2D);
	pTex2D->GetDesc(&textureDesc);

	UINT picWidth = textureDesc.Width;
	UINT picHeight = textureDesc.Height;

	ReleaseCOM(pTex2D);

	//every pixel of the height field stands for a 3D point
	//and we can use cross product to generate a pixel matrix of (n-1)x(n-1) dimension
	//the last 1 row & column need to be dealt with specially

	for (UINT j = 0;j < picHeight-1;j++)
	{
		for (UINT i = 0;i < picWidth-1;i++)
		{
			NVECTOR4  color1(0, 0,0, 0), color2(0, 0, 0,0), color3(0, 0, 0,0);
			NVECTOR3	v1(0, 0, 0), v2(0, 0, 0), currentNormal(0, 0, 0);
			UINT vertexID1 = 0, vertexID2 = 0, vertexID3 = 0;

			vertexID1 = mFunction_GetPixelIndexFromXY(i, j, picWidth);
			vertexID2 = mFunction_GetPixelIndexFromXY(i + 1, j, picWidth);
			vertexID3 = mFunction_GetPixelIndexFromXY(i, j+1, picWidth);
			color1 = m_pTextureObjectList->at(validatedTexID).mPixelBuffer.at(vertexID1);
			color2 = m_pTextureObjectList->at(validatedTexID).mPixelBuffer.at(vertexID2);
			color3 = m_pTextureObjectList->at(validatedTexID).mPixelBuffer.at(vertexID3);
			//because it's grey map , so we can only use one color channel
			v1 = NVECTOR3(1.0f, 0,bumpScaleFactor* (color2.x - color1.x));
			v2 = NVECTOR3(0, 1.0f, bumpScaleFactor* (color3.x - color1.x));
			D3DXVec3Cross(&currentNormal, &v1, &v2);

			//convert normal to Normal Map Color
			m_pTextureObjectList->at(validatedTexID).mPixelBuffer.at(vertexID1) =
				NVECTOR4((currentNormal.x + 1) / 2, (currentNormal.y + 1) / 2, (currentNormal.z + 1) / 2, 1.0f);
		}
	}

	//last row (except right bottom pixel)
	for (UINT i = 0;i < picWidth - 1;i++)
	{
		UINT currentVertexID = 0,vertexID1 = 0, vertexID2 = 0;
		currentVertexID = mFunction_GetPixelIndexFromXY(i, picHeight - 1, picWidth);
		vertexID1 = mFunction_GetPixelIndexFromXY(i, picHeight-2, picWidth);
		vertexID2 = mFunction_GetPixelIndexFromXY(i,0, picWidth);
		NVECTOR4 color1, color2;
		//the row (n-1) and row 0
		color1 = m_pTextureObjectList->at(validatedTexID).mPixelBuffer.at(vertexID1);
		color2 = m_pTextureObjectList->at(validatedTexID).mPixelBuffer.at(vertexID2);

		//compute average color
		m_pTextureObjectList->at(validatedTexID).mPixelBuffer.at(currentVertexID) =(color1 + color2) / 2;
	}

	//last colomn
	for (UINT i = 0;i < picHeight;i++)
	{
		UINT currentVertexID = 0, vertexID1 = 0, vertexID2 = 0;
		currentVertexID = mFunction_GetPixelIndexFromXY(picWidth - 1,i, picWidth);
		vertexID1 = mFunction_GetPixelIndexFromXY( picWidth - 2,i, picWidth);
		vertexID2 = mFunction_GetPixelIndexFromXY(0,i, picWidth);
		NVECTOR4 color1, color2;
		//the row (n-1) and row 0
		color1 = m_pTextureObjectList->at(validatedTexID).mPixelBuffer.at(vertexID1);
		color2 = m_pTextureObjectList->at(validatedTexID).mPixelBuffer.at(vertexID2);

		//compute average color
		m_pTextureObjectList->at(validatedTexID).mPixelBuffer.at(currentVertexID) = (color1 + color2) / 2;
	}


	//after modifying buffer in memory, update to GPU
	ID3D11Resource* pTmpRes;
	//resource reference count will increase ,so remember to release
	//so getResource() actually gets a interface to the resource BOUND to the view.
	m_pTextureObjectList->at(validatedTexID).m_pSRV->GetResource(&pTmpRes);

	//update resource which is bound to the corresponding SRV
	g_pImmediateContext->UpdateSubresource(
		pTmpRes,
		0,
		NULL,
		&m_pTextureObjectList->at(validatedTexID).mPixelBuffer.at(0),
		picWidth * NOISE_MACRO_DEFAULT_COLOR_BYTESIZE,
		NULL);

	ReleaseCOM(pTmpRes);

	return TRUE;
}

BOOL ITextureManager::ConvertHeightMapToNormalMap(std::string texName, float heightFieldScaleFactor)
{
	return ConvertHeightMapToNormalMap(GetTextureID(texName),heightFieldScaleFactor);
}

BOOL ITextureManager::SaveTextureToFile(UINT texID, NFilePath filePath, NOISE_TEXTURE_SAVE_FORMAT picFormat)
{
	if (texID >= 0 && texID < m_pTextureObjectList->size())
	{
		HRESULT hr = S_OK;
		ID3D11Texture2D* tmp_pResource;
		 m_pTextureObjectList->at(texID).m_pSRV->GetResource((ID3D11Resource**)&tmp_pResource);
		 //use d3dx11
		 hr= D3DX11SaveTextureToFileA(
			g_pImmediateContext, 
			tmp_pResource,
			D3DX11_IMAGE_FILE_FORMAT(picFormat),
			filePath.c_str()
			);
		 HR_DEBUG(hr, "Save Texture Failed!");
		ReleaseCOM(tmp_pResource);

	}

	return TRUE;
}

BOOL ITextureManager::SaveTextureToFile(std::string texName, NFilePath filePath, NOISE_TEXTURE_SAVE_FORMAT picFormat)
{
	return SaveTextureToFile(GetTextureID(texName),filePath,picFormat);
};

UINT	ITextureManager::GetTextureID(std::string texName)
{
	auto nameIndexPairIter = m_pTextureObjectHashTable->find(texName);
	if (nameIndexPairIter != m_pTextureObjectHashTable->end())
	{
		return nameIndexPairIter->second;//string-UINT pair
	}
	else
	{
		return NOISE_MACRO_INVALID_TEXTURE_ID;//Invalid texture Name
	}
}

void	 ITextureManager::GetTextureName(UINT index, std::string& outTextureName)
{
	//if index is valid, return an std::string name
	if (index < m_pTextureObjectList->size())
	{
		outTextureName = m_pTextureObjectList->at(index).mTexName;
	}
}

UINT ITextureManager::GetTextureWidth(UINT texID)
{
	
	if (texID >=0 && texID<m_pTextureObjectList->size())
	{
		ID3D11Texture2D* pTmpRes;
		D3D11_TEXTURE2D_DESC texDesc;
		//get resource from SRV, then get description from texture2D
		m_pTextureObjectList->at(texID).m_pSRV->GetResource((ID3D11Resource**)&pTmpRes);
		pTmpRes->GetDesc(&texDesc);
		ReleaseCOM(pTmpRes);
		return texDesc.Width;
	}

	return 0;
}

UINT ITextureManager::GetTextureWidth(std::string texName)
{
	return GetTextureWidth(GetTextureID(texName));
}

UINT ITextureManager::GetTextureHeight(UINT texID)
{
	if (texID >= 0 && texID < m_pTextureObjectList->size())
	{
		ID3D11Texture2D* pTmpRes;
		D3D11_TEXTURE2D_DESC texDesc;
		//get resource from SRV, then get description from texture2D
		m_pTextureObjectList->at(texID).m_pSRV->GetResource((ID3D11Resource**)&pTmpRes);
		pTmpRes->GetDesc(&texDesc);
		ReleaseCOM(pTmpRes);
		return texDesc.Height;
	}

	return 0;
}

UINT ITextureManager::GetTextureHeight(std::string texName)
{
	return GetTextureHeight(GetTextureID(texName));
}

UINT	 ITextureManager::GetTextureCount()
{
	return m_pTextureObjectList->size();
}


BOOL ITextureManager::DeleteTexture(UINT texID)
{
	auto iter = m_pTextureObjectList->begin();

	//validate index 
	UINT validatedTexID = ValidateIndex(texID, m_pTextureObjectList->at(texID).mTextureType);
	if (validatedTexID != NOISE_MACRO_INVALID_TEXTURE_ID)
	{
		//delete name-index pair
		std::string deleteTexName = m_pTextureObjectList->at(validatedTexID).mTexName;
		auto nameIndexPairIter = m_pTextureObjectHashTable->find(deleteTexName);
		//..............(the check might be useless...)
		if (nameIndexPairIter != m_pTextureObjectHashTable->end())
		{
			m_pTextureObjectHashTable->erase(nameIndexPairIter);
		}

		//clear TextureObject Data
		iter += texID;
		//safe_release SRV  interface
		ReleaseCOM(m_pTextureObjectList->at(texID).m_pSRV);
		m_pTextureObjectList->at(texID).mPixelBuffer.clear();
		m_pTextureObjectList->erase(iter);

		//update name-index pair (decrease the "val")
		mFunction_RefreshHashTableAfterDeletion(validatedTexID, 1);
	}
	else
	{
		DEBUG_MSG1("Delete Texture : texID invalid!!");
		return FALSE;
	}

	return TRUE;
}

BOOL ITextureManager::DeleteTexture(std::string texName)
{
	auto nameIndexPairIter = m_pTextureObjectHashTable->find(texName);
	//..............(the check might be useless...)
	if (nameIndexPairIter != m_pTextureObjectHashTable->end())
	{
		UINT deleteTexID = nameIndexPairIter->second;
		UINT validatedTexID = ValidateIndex(deleteTexID);//only check the ID boundary
		//delete the real data
		if (validatedTexID != NOISE_MACRO_INVALID_TEXTURE_ID)
		{
			//delete real Texture Data
			auto tmpIter = m_pTextureObjectList->begin() + validatedTexID;
			m_pTextureObjectList->erase(tmpIter);
		}

		//delete string-index pair
		m_pTextureObjectHashTable->erase(nameIndexPairIter);

		//update name-index pair (decrease the "val")
		mFunction_RefreshHashTableAfterDeletion(validatedTexID,1);
	}
	else
	{
		DEBUG_MSG1("Delete Texture : texture Name not found!!");
		return FALSE;
	}
	return TRUE;
}

UINT ITextureManager::ValidateIndex(UINT texID)
{
	if (texID < m_pTextureObjectList->size())
	{
		return texID;
	}
	else
	{
		return NOISE_MACRO_INVALID_TEXTURE_ID;
	}
}

UINT ITextureManager::ValidateIndex(UINT texID, NOISE_TEXTURE_TYPE texType)
{
	//validate if texID is within range of vector , and if the TextureType of this texture match the given 'texType'

	//if texID is invalid , an invalid flag should be returned
	if (texID == NOISE_MACRO_INVALID_TEXTURE_ID || texID >= m_pTextureObjectList->size())
	{
		return NOISE_MACRO_INVALID_TEXTURE_ID;
	}

	//then check the texture type : common / cubemap / volumn
	D3D11_SHADER_RESOURCE_VIEW_DESC tmpSRViewDesc;
	m_pTextureObjectList->at(texID).m_pSRV->GetDesc(&tmpSRViewDesc);

	switch (tmpSRViewDesc.ViewDimension)
	{
	case D3D11_SRV_DIMENSION_TEXTURECUBE:
		if (texType != NOISE_TEXTURE_TYPE_CUBEMAP)
		{
			return NOISE_MACRO_INVALID_TEXTURE_ID;
		}
		break;

	case D3D11_SRV_DIMENSION_TEXTURE3D:
		if (texType != NOISE_TEXTURE_TYPE_VOLUMN)
		{
			return NOISE_MACRO_INVALID_TEXTURE_ID;
		}
		break;

	case D3D11_SRV_DIMENSION_TEXTURE2D:
		if (texType != NOISE_TEXTURE_TYPE_COMMON)
		{
			return NOISE_MACRO_INVALID_TEXTURE_ID;
		}
		break;

	}

	//a no-problem texID
	return texID;
}


/*************************************************************
							P R I V A T E
*************************************************************/

inline void ITextureManager::mFunction_RefreshHashTableAfterDeletion(UINT deletedTexID_threshold, UINT indexDecrement)
{
	//deletions always happen in the middle of a list , so elements after it 
	//will be affected.
	for (auto& pair : *m_pTextureObjectHashTable)
	{
		if (pair.second>deletedTexID_threshold)pair.second -= indexDecrement;
	}
};

UINT ITextureManager::mFunction_CreateTextureFromFile_DirectlyLoadToGpu(NFilePath filePath, std::string& textureName, BOOL useDefaultSize, UINT pixelWidth, UINT pixelHeight)
{
	//!!!!!!!!!!!!!!!!File Size Maybe a problem???

	//check if the file existed
	std::fstream tmpFile(filePath, std::ios::binary | std::ios::in);
	if (tmpFile.bad())
	{
		DEBUG_MSG1("Texture file not found!!");
		return NOISE_MACRO_INVALID_TEXTURE_ID;//invalid
	}

	//some settings about loading image
	D3DX11_IMAGE_LOAD_INFO loadInfo;

	//we must check if user want to use default image size
	if (useDefaultSize)
	{
		loadInfo.Width = D3DX11_DEFAULT;
		loadInfo.Height = D3DX11_DEFAULT;
	}
	else
	{
		loadInfo.Width = (pixelWidth > 0 ? pixelWidth : D3DX11_DEFAULT);
		loadInfo.Height = (pixelHeight > 0 ? pixelHeight : D3DX11_DEFAULT);
	}

	//continue filling the settings
	loadInfo.Filter = D3DX11_FILTER_LINEAR;
	loadInfo.MiscFlags = D3DX11_DEFAULT;
	loadInfo.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	loadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	loadInfo.MiscFlags = D3DX11_DEFAULT;
	loadInfo.CpuAccessFlags = NULL;// cpuAccessFlag;
	loadInfo.Usage =  D3D11_USAGE_DEFAULT;//bufferUsage;


	//create New Texture Object
	HRESULT hr = S_OK;
	UINT newTexIndex = m_pTextureObjectList->size();
	N_TextureObject tmpTexObj;

	//we must check if new name has been used
	if(GetTextureID(textureName)!=NOISE_MACRO_INVALID_TEXTURE_ID)
	{
		DEBUG_MSG1("CreateTextureFromFile : Texture name has been used!!");
		return NOISE_MACRO_INVALID_TEXTURE_ID;//invalid
	}

	//then endow a pointer to new SRV

	D3DX11CreateShaderResourceViewFromFileA(
		g_pd3dDevice11,
		filePath.c_str(),
		&loadInfo,
		nullptr,
		&tmpTexObj.m_pSRV,
		&hr
		);
		
	//................
	HR_DEBUG_CREATETEX(hr, "CreateTextureFromFile : Create SRV failed ! ; keepCopyInMem:false");

	//endow the tex obj a name
	tmpTexObj.mTexName = textureName;
	tmpTexObj.mIsPixelBufferInMemValid = FALSE;
	tmpTexObj.mTextureType = NOISE_TEXTURE_TYPE_COMMON;
	m_pTextureObjectList->push_back(tmpTexObj);

	//new texture index
	newTexIndex = m_pTextureObjectList->size() - 1;

	//---------------ADD NAME-INDEX PAIR-----------------
	m_pTextureObjectHashTable->insert(std::make_pair(tmpTexObj.mTexName, newTexIndex));

	return newTexIndex;//invalid file or sth else
}

UINT ITextureManager::mFunction_CreateTextureFromFile_KeepACopyInMemory(NFilePath filePath, std::string& textureName, BOOL useDefaultSize, UINT pixelWidth, UINT pixelHeight)
{
	//!!!!!!!!!!!!!!!!File Size Maybe a problem???

	//check if the file existed
	std::fstream tmpFile(filePath, std::ios::binary | std::ios::in);
	if (tmpFile.bad())
	{
		DEBUG_MSG1("CreateTextureFromFile : Texture file not found!!");
		return NOISE_MACRO_INVALID_TEXTURE_ID;//invalid
	}


	//create New Texture Object
	HRESULT hr = S_OK;
	UINT newTexIndex = NOISE_MACRO_INVALID_TEXTURE_ID;

	//we must check if new name has been used
	//count() will return 0 if given key dont exists
	if (GetTextureID(textureName) != NOISE_MACRO_INVALID_TEXTURE_ID)
	{
		DEBUG_MSG1("CreateTextureFromFile : Texture name has been used!!");
		return NOISE_MACRO_INVALID_TEXTURE_ID;//invalid
	}

	//a temporary Texture Object
	N_TextureObject tmpTexObj;
	tmpTexObj.mIsPixelBufferInMemValid = TRUE;
	tmpTexObj.mTexName = textureName;
	tmpTexObj.mTextureType = NOISE_TEXTURE_TYPE_COMMON;


#pragma region LoadPixelMatrix


	//some settings about loading image
	D3DX11_IMAGE_LOAD_INFO loadInfo;

	//we must check if user want to use default image size
	if (useDefaultSize)
	{
		loadInfo.Width = D3DX11_DEFAULT;
		loadInfo.Height = D3DX11_DEFAULT;
	}
	else
	{
		loadInfo.Width = (pixelWidth > 0 ? pixelWidth : D3DX11_DEFAULT);
		loadInfo.Height = (pixelHeight > 0 ? pixelHeight : D3DX11_DEFAULT);
	}

	//we must create a texture with  STAGING / DYNAMIC or whatever not default   usage
	loadInfo.Filter = D3DX11_FILTER_NONE;//D3DX11_FILTER_LINEAR;
	loadInfo.MiscFlags = D3DX11_DEFAULT;
	loadInfo.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	loadInfo.BindFlags = NULL;// D3D11_BIND_SHADER_RESOURCE;
	loadInfo.MiscFlags = NULL;
	loadInfo.CpuAccessFlags = D3D11_CPU_ACCESS_READ;// cpuAccessFlag;
	loadInfo.Usage = D3D11_USAGE_STAGING;//bufferUsage;

	//create texture from files
	ID3D11Texture2D* pOriginTexture2D;
	D3DX11CreateTextureFromFileA(
		g_pd3dDevice11,
		filePath.c_str(),
		&loadInfo,
		NULL,
		(ID3D11Resource**)&pOriginTexture2D,
		&hr
		);

	HR_DEBUG_CREATETEX(hr, "CreateTexture : Load Texture From File failed! (keep memory copy)");

	//use Map() to get data
	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	g_pImmediateContext->Map(pOriginTexture2D, 0, D3D11_MAP_READ, NULL, &mappedSubresource);

	//get picture size info and load pixels to memory
	D3D11_TEXTURE2D_DESC	 originTexDesc;
	pOriginTexture2D->GetDesc(&originTexDesc);

	//I DONT KNOW WHY CreateTextureFromFile()  WILL CHANGE THE SIZE OF TEXTURE !!
	UINT loadedTexWidth = mappedSubresource.RowPitch / NOISE_MACRO_DEFAULT_COLOR_BYTESIZE;//loadedTexDesc.Width;
	UINT loadedTexHeight = originTexDesc.Height;

	//I DONT KNOW WHY CreateTextureFromFile() WILL CHANGE THE SIZE OF TEXTURE !!
	//it seems that the width will be scaled to ..... round off....
	(tmpTexObj.mPixelBuffer.assign(
		(NVECTOR4*)mappedSubresource.pData,
		(NVECTOR4*)mappedSubresource.pData + loadedTexWidth*loadedTexHeight));



	//remember to Unmap() after every mapping
	g_pImmediateContext->Unmap(pOriginTexture2D, 0);

	ReleaseCOM(pOriginTexture2D);
#pragma endregion LoadPixelMatrix

	
#pragma region CreateTex2D&SRV

	//texture2D desc (create a default usage texture)
	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width = loadedTexWidth;//determined by the loaded picture
	texDesc.Height = loadedTexHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage =  D3D11_USAGE_DEFAULT;//allow update subresource
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags =NULL;
	texDesc.MiscFlags = NULL;

	//SRV desc
	D3D11_SHADER_RESOURCE_VIEW_DESC SRViewDesc;
	SRViewDesc.Format = texDesc.Format;
	SRViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRViewDesc.TextureCube.MipLevels = texDesc.MipLevels;
	SRViewDesc.TextureCube.MostDetailedMip = 0;

	//init data
	D3D11_SUBRESOURCE_DATA texInitDataDesc;
	texInitDataDesc.pSysMem = &tmpTexObj.mPixelBuffer.at(0);
	texInitDataDesc.SysMemPitch = (texDesc.Width) * NOISE_MACRO_DEFAULT_COLOR_BYTESIZE;//bytesize for 1 row
	texInitDataDesc.SysMemSlicePitch = 0;//available for tex3D

	 //create texture2D first 
	ID3D11Texture2D* pTmpTexture2D;
	hr = g_pd3dDevice11->CreateTexture2D(&texDesc, &texInitDataDesc, &pTmpTexture2D);
	if (FAILED(hr))
	{
		ReleaseCOM(pTmpTexture2D);
		DEBUG_MSG1("CreateTextureFromFile : Create ID3D11Texture2D failed!");
		return NOISE_MACRO_INVALID_TEXTURE_ID;
	}

	//Create SRV from texture 2D (to a tmp textureObject)
	hr = g_pd3dDevice11->CreateShaderResourceView(pTmpTexture2D, &SRViewDesc, &tmpTexObj.m_pSRV);
	if (FAILED(hr))
	{
		ReleaseCOM(pTmpTexture2D);
		DEBUG_MSG1("CreateTextureFromFile : Create ID3D11SRV failed!");
		return NOISE_MACRO_INVALID_TEXTURE_ID;
	}

#pragma endregion CreateTex2D&SRV

	//clear tmp interfaces
	ReleaseCOM(pTmpTexture2D);

	//at last push back a new texture object
	m_pTextureObjectList->push_back(tmpTexObj);
	newTexIndex = m_pTextureObjectList->size()-1;

	//---------------ADD NAME-INDEX PAIR-----------------
	m_pTextureObjectHashTable->insert(std::make_pair(tmpTexObj.mTexName, newTexIndex));


	return newTexIndex;
}

inline UINT ITextureManager::mFunction_GetPixelIndexFromXY(UINT x, UINT y, UINT width)
{
	return y*width +x;
};

