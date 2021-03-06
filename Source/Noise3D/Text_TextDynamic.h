/***********************************************************************

								h��Noise2DText

************************************************************************/

#pragma once

namespace Noise3D
{
	class /*_declspec(dllexport)*/ IDynamicText :
			public  IBasicContainerInfo,
			public	IBasicTextInfo
		{
		public:
			friend class IRenderer;
			friend class IFontManager;

			void		SetWidth(float w);

			void		SetHeight(float h);

			void		SetFont(N_UID fontName);

			N_UID		GetFontName();

			void		SetTextAscii(const std::string& newText);

			void		GetTextAscii(std::string& outString);

			void		SetLineSpacingOffset(int offset);

			int		GetLineSpacingOffset();

			void		SetWordSpacingOffset(int offset);

			int		GetWordSpacingOffset();

			NVECTOR2 GetWordLocalPosOffset(UINT wordIndex);//index of word in string,not ascii code

			NVECTOR2 GetWordRealSize(UINT wordIndex);

			NVECTOR2 GetFontSize(UINT fontID);

		private:

			friend IFactory<IDynamicText>;

			IDynamicText();

			~IDynamicText();

			void	NOISE_MACRO_FUNCTION_EXTERN_CALL mFunction_InitGraphicObject(IGraphicObject* pCreatedObj, UINT pxWidth, UINT pxHeight, NVECTOR4 color, N_UID texName);

			void  NOISE_MACRO_FUNCTION_EXTERN_CALL	mFunction_UpdateGraphicObject();//by renderer

		private:

			N_UID*					m_pFontName;
			UINT					mCharBoundarySizeX;//updated when SetFontID
			UINT					mCharBoundarySizeY;
			int					mWordSpacingOffset;
			int					mLineSpacingOffset;
			N_UID*				m_pTextureName;//which bitmap texture to refer to
			std::string*		m_pTextContent;//the target "string"
			BOOL				mIsTextContentChanged;
			BOOL				mIsSizeChanged;
		};
}