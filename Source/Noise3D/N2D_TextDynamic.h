/***********************************************************************

								h��Noise2DText

************************************************************************/

#pragma once

namespace Noise3D
{
		class _declspec(dllexport) IDynamicText :
			public  IBasicContainerInfo,
			public	IBasicTextInfo
		{
		public:
			friend class IRenderer;
			friend class IFontManager;

			IDynamicText();

			void		SetWidth(float w);

			void		SetHeight(float h);

			void		SetFont(UINT fontID);

			UINT		GetFontID();

			void		SetTextAscii(std::string newText);

			void		GetTextAscii(std::string& outString);

			void		SetLineSpacingOffset(int offset);

			int		GetLineSpacingOffset();

			void		SetWordSpacingOffset(int offset);

			int		GetWordSpacingOffset();

			NVECTOR2 GetWordLocalPosOffset(UINT wordIndex);//index of word in string,not ascii code

			NVECTOR2 GetWordRealSize(UINT wordIndex);

			NVECTOR2 GetFontSize(UINT fontID);

		private:

			void	Destroy();

			void	mFunction_InitGraphicObject(UINT pxWidth, UINT pxHeight, NVECTOR4 color, UINT texID);

			void  NOISE_MACRO_FUNCTION_EXTERN_CALL	mFunction_UpdateGraphicObject();//by renderer

		private:

			UINT					mFontID;
			UINT					mStringTextureID;
			UINT					mCharBoundarySizeX;//updated when SetFontID
			UINT					mCharBoundarySizeY;
			int					mWordSpacingOffset;
			int					mLineSpacingOffset;
			std::string*		m_pTextureName;//which bitmap texture to refer to
			std::string*		m_pTextContent;//the target "string"
			BOOL				mIsTextContentChanged;
			BOOL				mIsSizeChanged;
		};
}