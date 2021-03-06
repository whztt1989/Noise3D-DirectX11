
/***********************************************************************

							Desc: Atmosphere Renderer (D3D)
	
************************************************************************/

#include "Noise3D.h"

using namespace Noise3D;

const UINT c_VBstride_Default = sizeof(N_DefaultVertex);	
const UINT c_VBstride_Simple = sizeof(N_SimpleVertex);
const UINT c_VBoffset = 0;				


void IRenderer::RenderAtmosphere()
{
	//...................
	ICamera* const tmp_pCamera = GetScene()->GetCamera();

	//update view/proj matrix
	mFunction_CameraMatrix_Update(tmp_pCamera);

	//actually there is only 1 atmosphere because you dont need more 
	for (UINT i = 0;i < m_pRenderList_Atmosphere->size();i++)
	{
		IAtmosphere* const  pAtmo = m_pRenderList_Atmosphere->at(i);
		//texture manager singleton
		ITextureManager* pTexMgr = GetScene()->GetTextureMgr();

		if (pAtmo == nullptr)continue;

		//enable/disable fog effect 
		mFunction_Atmosphere_Fog_Update(pAtmo,pTexMgr);

#pragma region Draw Sky

		g_pImmediateContext->IASetInputLayout(g_pVertexLayout_Simple);
		g_pImmediateContext->IASetVertexBuffers(0, 1, &pAtmo->m_pVB_Gpu_Sky, &c_VBstride_Simple, &c_VBoffset);
		g_pImmediateContext->IASetIndexBuffer(pAtmo->m_pIB_Gpu_Sky, DXGI_FORMAT_R32_UINT, 0);
		g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//......Set States
		mFunction_SetRasterState(m_FillMode, m_CullMode);
		mFunction_SetBlendState(m_BlendMode);
		m_pFX_SamplerState_Default->SetSampler(0, m_pSamplerState_FilterAnis);
		g_pImmediateContext->OMSetDepthStencilState(m_pDepthStencilState_EnableDepthTest, 0xffffffff);


		//update Vertices or atmo param to GPU
		//shaders will decide to draw skybox or sky dome
		//( there are SkyboxValid & SkyDomeValid BOOL)
		//UINT skyDomeTexID = NOISE_MACRO_INVALID_TEXTURE_ID;
		//UINT skyBoxTexID = NOISE_MACRO_INVALID_TEXTURE_ID;
		N_UID skyDomeTexName = "";
		N_UID skyBoxTexName = "";
		mFunction_Atmosphere_SkyDome_Update(pAtmo, pTexMgr,skyDomeTexName);
		mFunction_Atmosphere_SkyBox_Update(pAtmo, pTexMgr,skyBoxTexName);
		mFunction_Atmosphere_UpdateCbAtmosphere(pAtmo, pTexMgr, skyDomeTexName, skyBoxTexName);


		//traverse passes in one technique ---- pass index starts from 1
		D3DX11_TECHNIQUE_DESC	tmpTechDesc;
		m_pFX_Tech_DrawSky->GetDesc(&tmpTechDesc);
		for (UINT k = 0;k < tmpTechDesc.Passes; k++)
		{
			m_pFX_Tech_DrawSky->GetPassByIndex(k)->Apply(0, g_pImmediateContext);
			g_pImmediateContext->DrawIndexed(pAtmo->m_pIB_Mem_Sky->size(), 0, 0);
		}

		//allow atmosphere to "add to render list" again 
		pAtmo->mFogHasBeenAddedToRenderList = FALSE;

#pragma endregion Draw Sky
	}

}


/***********************************************************************
									P R I V A T E
************************************************************************/


void		IRenderer::mFunction_Atmosphere_Fog_Update(IAtmosphere*const pAtmo,ITextureManager* const pTexMgr)
{
	if (pAtmo->mFogCanUpdateToGpu)
	{
		//update fog param
		m_CbAtmosphere.mFogColor = *(pAtmo->m_pFogColor);
		m_CbAtmosphere.mFogFar = pAtmo->mFogFar;
		m_CbAtmosphere.mFogNear = pAtmo->mFogNear;
		m_CbAtmosphere.mIsFogEnabled = (BOOL)(pAtmo->mFogEnabled && pAtmo->mFogHasBeenAddedToRenderList);

		//udpate to GPU
		m_pFX_CbAtmosphere->SetRawValue(&m_CbAtmosphere, 0, sizeof(m_CbAtmosphere));
		pAtmo->mFogCanUpdateToGpu = FALSE;
	}
};

void		IRenderer::mFunction_Atmosphere_SkyDome_Update(IAtmosphere*const pAtmo, ITextureManager* const pTexMgr, N_UID& outSkyDomeTexName)
{
	//validate texture and update BOOL value to gpu
	 N_UID skyDomeTexName = *pAtmo->m_pSkyDomeTexName;

	//check skyType
	if (pAtmo->mSkyType == NOISE_ATMOSPHERE_SKYTYPE_DOME)
	{
		BOOL isTextureUidValid = pTexMgr->FindUid(skyDomeTexName);
		//if texture pass UID validation and match current skytype
		m_CbAtmosphere.mIsSkyDomeValid = isTextureUidValid;//if texture is also valid, sky dome will be allowed to render
		outSkyDomeTexName = skyDomeTexName;
	}
	else
	{
		m_CbAtmosphere.mIsSkyDomeValid = FALSE;
		outSkyDomeTexName = "";
	}

};

void		IRenderer::mFunction_Atmosphere_SkyBox_Update(IAtmosphere*const pAtmo, ITextureManager* const pTexMgr, N_UID& outSkyBoxTexName)
{
	//skybox uses cube map to texture the box
	N_UID skyboxTexName =*pAtmo->m_pSkyBoxCubeTexName;

	//check skyType
	if (pAtmo->mSkyType == NOISE_ATMOSPHERE_SKYTYPE_BOX)
	{
		BOOL isTextureUidValid = pTexMgr->FindUid(skyboxTexName);
		//skybox texture must be a cube map
		m_CbAtmosphere.mIsSkyBoxValid = pTexMgr->ValidateUID(skyboxTexName, NOISE_TEXTURE_TYPE_CUBEMAP);
		m_CbAtmosphere.mSkyBoxWidth = pAtmo->mSkyBoxWidth;
		m_CbAtmosphere.mSkyBoxHeight = pAtmo->mSkyBoxHeight;
		m_CbAtmosphere.mSkyBoxDepth = pAtmo->mSkyBoxDepth;
		outSkyBoxTexName = skyboxTexName;
	}
	else
	{
		m_CbAtmosphere.mIsSkyBoxValid = FALSE;
		outSkyBoxTexName = "";
	}

};

void		IRenderer::mFunction_Atmosphere_UpdateCbAtmosphere(IAtmosphere*const pAtmo, ITextureManager* const pTexMgr, const N_UID& skyDomeTexName, const N_UID& skyBoxTexName)
{
	//update valid texture to gpu
	if (m_CbAtmosphere.mIsSkyDomeValid)
	{
		//texName has been validated in UPDATE function
		auto tmp_pSRV = pTexMgr->GetObjectPtr(skyDomeTexName)->m_pSRV;
		m_pFX_Texture_Diffuse->SetResource(tmp_pSRV);
	}


	//update skybox cube map to gpu
	if (m_CbAtmosphere.mIsSkyBoxValid)
	{
		//pAtmo->mSkyBoxTextureID has been validated  in UPDATE function
		//but how do you validate it's a valid cube map ?????
		auto tmp_pSRV = pTexMgr->GetObjectPtr(skyBoxTexName)->m_pSRV;
		m_pFX_Texture_CubeMap->SetResource(tmp_pSRV);
	}


	m_pFX_CbAtmosphere->SetRawValue(&m_CbAtmosphere, 0, sizeof(m_CbAtmosphere));
};
