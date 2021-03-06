
/***********************************************************************

                           h��NoiseTextureManager

************************************************************************/
#pragma once

namespace Noise3D
{

	class /*_declspec(dllexport)*/ ITextureManager :
		public IFactory<ITexture>
	{
	public:
		friend class IRenderer;
		//friend class IFontManager;//Let it Create\Access bitmap table Texture

		ITexture*		CreatePureColorTexture(N_UID texName, UINT pixelWidth, UINT pixelHeight, NVECTOR4 color, BOOL keepCopyInMemory = FALSE);

		ITexture*		CreateTextureFromFile(NFilePath filePath, N_UID texName, BOOL useDefaultSize, UINT pixelWidth, UINT pixelHeight, BOOL keepCopyInMemory = FALSE);

		ITexture*		CreateCubeMapFromFiles(NFilePath fileName[6], N_UID cubeTextureName, NOISE_CUBEMAP_SIZE faceSize);

		ITexture*		CreateCubeMapFromDDS(NFilePath dds_FileName, N_UID cubeTextureName, NOISE_CUBEMAP_SIZE faceSize);

		ITexture*		GetTexture(N_UID texName);

		UINT			GetTextureCount();

		BOOL			DeleteTexture(ITexture* pTex);

		BOOL			DeleteTexture(N_UID texName);

		void				DeleteAllTexture();

		BOOL			ValidateUID(N_UID texName);

		BOOL			ValidateUID(N_UID texName, NOISE_TEXTURE_TYPE texType);

	private:

		friend IFactory<ITextureManager>;

		//���캯��
		ITextureManager();

		~ITextureManager();

		ITexture*		mFunction_CreateTextureFromFile_DirectlyLoadToGpu(NFilePath filePath, std::string& texName, BOOL useDefaultSize, UINT pixelWidth, UINT pixelHeight);

		ITexture*		mFunction_CreateTextureFromFile_KeepACopyInMemory(NFilePath filePath, std::string& texName, BOOL useDefaultSize, UINT pixelWidth, UINT pixelHeight);
	};
}