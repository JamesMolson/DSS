#ifndef __AHDDEMOSAICING_H__
#define __AHDDEMOSAICING_H__

/* ------------------------------------------------------------------- */

class CRGBToLab
{
public :
	CRGBToLab();

	void	RGBToLab(double fRed, double fGreen, double fBlue, double & L, double & a, double & b);
};

/* ------------------------------------------------------------------- */

static const LONG							AHDWS = 256;

template <typename TType>
class CAHDTaskVariables
{
public :
	CSmartPtr<CColorBitmapT<TType> >		pWindowV;
	CSmartPtr<CColorBitmapT<TType> >		pWindowH;
	CSmartPtr<C96BitFloatColorBitmap>		pLabWindowV;
	CSmartPtr<C96BitFloatColorBitmap>		pLabWindowH;
	CSmartPtr<C8BitGrayBitmap>				pHomoH;
	CSmartPtr<C8BitGrayBitmap>				pHomoV;

public :
	CAHDTaskVariables() {};
	virtual ~CAHDTaskVariables() {};

	BOOL		Init()
	{
		BOOL				bResult;

		pWindowV.Attach(new C48BitColorBitmap);
		pWindowH.Attach(new C48BitColorBitmap);
		pLabWindowV.Attach(new C96BitFloatColorBitmap);
		pLabWindowH.Attach(new C96BitFloatColorBitmap);
		pHomoH.Attach(new C8BitGrayBitmap);
		pHomoV.Attach(new C8BitGrayBitmap);

		bResult = pWindowV->Init(AHDWS, AHDWS);
		bResult = bResult && pWindowH->Init(AHDWS, AHDWS);
		bResult = bResult && pLabWindowV->Init(AHDWS, AHDWS);
		bResult = bResult && pLabWindowH->Init(AHDWS, AHDWS);
		bResult = bResult && pHomoH->Init(AHDWS, AHDWS);
		bResult = bResult && pHomoV->Init(AHDWS, AHDWS);

		return bResult;
	};
};

template <typename TType>
class CAHDTask : public CMultitask
{
public :
	CDSSProgress *							pProgress;
	CRGBToLab								RGBToLab;
	CSmartPtr<CGrayBitmapT<TType> >			pGrayBitmap;
	CSmartPtr<CColorBitmapT<TType> >		pColorBitmap;
	CSmartPtr<CMemoryBitmap>				pOutputBitmap;
	LONG									lWidth;
	LONG									lHeight;

	double									fMultiplier;

public :
	CAHDTask()
    {
        lWidth = 0;
        lHeight = 0;
        fMultiplier = 0;
    }

	virtual ~CAHDTask() {};

	BOOL	Init(CGrayBitmapT<TType> * pGrayBitmap, CDSSProgress * pProgress);

	virtual BOOL	Process();
	virtual void	DoSubWindow(LONG x, LONG y, CAHDTaskVariables<TType> & var);
	virtual BOOL	DoTask(HANDLE hEvent);

	virtual void	InterpolateBorders();
};

/* ------------------------------------------------------------------- */

template <typename TType>
inline BOOL	CAHDTask<TType>::Init(CGrayBitmapT<TType> * pGrayBmp, CDSSProgress * pPgs)
{
	BOOL						bResult;

	pProgress		= pPgs;
	pGrayBitmap		= pGrayBmp;
	lWidth = pGrayBitmap->Width();
	lHeight = pGrayBitmap->Height();

	fMultiplier = pGrayBitmap->GetMultiplier()*256.0;

	pColorBitmap.Attach(new CColorBitmapT<TType>);
	bResult = pColorBitmap->Init(lWidth, lHeight);

	return bResult;
};

/* ------------------------------------------------------------------- */

template <typename TType>
inline BOOL	CAHDTask<TType>::Process()
{
	BOOL			bResult = TRUE;
	LONG			lNrWindows;
	LONG			lNrWindowsWidth,
					lNrWindowsHeight;
	LONG			x, y;

	lNrWindowsWidth = lWidth/(AHDWS-4);
	if (lWidth % (AHDWS-4))
		lNrWindowsWidth++;

	lNrWindowsHeight = lHeight/(AHDWS-4);
	if (lHeight % (AHDWS-4))
		lNrWindowsHeight++;

	lNrWindows = lNrWindowsWidth * lNrWindowsHeight;

	if (pProgress)
	{
		pProgress->Start2(nullptr, lNrWindows);
		pProgress->SetNrUsedProcessors(GetNrThreads());
	};

	lNrWindows = 0;

	for (y = 0;y<lHeight;y+=AHDWS-4)
	{
		for (x= 0;x<lWidth;x+=AHDWS-4)
		{
			DWORD			dwThreadId;

			dwThreadId = GetAvailableThreadId();
			PostThreadMessage(dwThreadId, WM_MT_PROCESS, x, y);

			lNrWindows++;
			if (pProgress)
				pProgress->Progress2(nullptr, lNrWindows);
		};
	};

	CloseAllThreads();
	if (pProgress)
	{
		pProgress->SetNrUsedProcessors();
		pProgress->End2();
	};

	return bResult;
};

/* ------------------------------------------------------------------- */

template <typename TType>
inline void	CAHDTask<TType>::DoSubWindow(LONG x, LONG y, CAHDTaskVariables<TType> & var)
{
	// Iterate through the window
	LONG						wx, wy;
	TType *						pBaseGrayPixel;
	TType *						pVBaseGreenPixel;
	TType *						pHBaseGreenPixel;
	TType *						pVBaseRedPixel;
	TType *						pHBaseRedPixel;
	TType *						pVBaseBluePixel;
	TType *						pHBaseBluePixel;
	float *						pVBaseLPixel;
	float *						pVBaseaPixel;
	float *						pVBasebPixel;
	float *						pHBaseLPixel;
	float *						pHBaseaPixel;
	float *						pHBasebPixel;
	BYTE *						pHBaseHomoPixel;
	BYTE *						pVBaseHomoPixel;

	TType *						pBaseOutputRedPixel;
	TType *						pBaseOutputGreenPixel;
	TType *						pBaseOutputBluePixel;

	pBaseGrayPixel			= pGrayBitmap->GetGrayPixel(x, y);
	pBaseOutputRedPixel		= pColorBitmap->GetRedPixel(x, y);
	pBaseOutputGreenPixel	= pColorBitmap->GetGreenPixel(x, y);
	pBaseOutputBluePixel	= pColorBitmap->GetBluePixel(x, y);

	pVBaseGreenPixel		= var.pWindowV->GetGreenPixel(0, 0);
	pHBaseGreenPixel		= var.pWindowH->GetGreenPixel(0, 0);
	pVBaseRedPixel			= var.pWindowV->GetRedPixel(0, 0);
	pHBaseRedPixel			= var.pWindowH->GetRedPixel(0, 0);
	pVBaseBluePixel			= var.pWindowV->GetBluePixel(0, 0);
	pHBaseBluePixel			= var.pWindowH->GetBluePixel(0, 0);

	pVBaseLPixel			= var.pLabWindowV->GetRedPixel(0, 0);
	pVBaseaPixel			= var.pLabWindowV->GetGreenPixel(0, 0);
	pVBasebPixel			= var.pLabWindowV->GetBluePixel(0, 0);
	pHBaseLPixel			= var.pLabWindowH->GetRedPixel(0, 0);
	pHBaseaPixel			= var.pLabWindowH->GetGreenPixel(0, 0);
	pHBasebPixel			= var.pLabWindowH->GetBluePixel(0, 0);

	pHBaseHomoPixel			= var.pHomoH->GetGrayPixel(0, 0);
	pVBaseHomoPixel			= var.pHomoV->GetGrayPixel(0, 0);

	// Interpolate green horizontally and vertically
	for (wy = y;wy<lHeight && wy<y+AHDWS;wy++)
	{
		TType *					pGrayPixel;
		TType *					pHGreenPixel;
		TType *					pVGreenPixel;

		pGrayPixel = pBaseGrayPixel + (wy-y)*lWidth;
		pHGreenPixel = pHBaseGreenPixel + (wy-y)*AHDWS;
		pVGreenPixel = pVBaseGreenPixel + (wy-y)*AHDWS;

		for (wx = x;wx<lWidth && wx <x+AHDWS;wx++)
		{
			BAYERCOLOR			BayerColor;
            double				fVGreen = 0;
            double              fHGreen = 0;

			BayerColor = pGrayBitmap->GetBayerColor(wx, wy);
			switch (BayerColor)
			{
			case BAYER_BLUE :
			case BAYER_RED :
				{
					// Blue and Red line and column
					// Horizontal green interpolation
					double		g1, g3;

					g1 = (wx > 0) ? (*(pGrayPixel-1))/fMultiplier : 0;
					g3 = (wx < lWidth-1) ? (*(pGrayPixel+1))/fMultiplier : 0;

					if (g1 == g3)
						fHGreen = g1;
					else
					{
						double	v0, v2, v4;

						v0 = (*pGrayPixel)/fMultiplier;
						v4 = (wx > 1) ? (*(pGrayPixel-2))/fMultiplier : 0;
						v2 = (wx < lWidth-2) ? (*(pGrayPixel+2))/fMultiplier : 0;
						fHGreen = (g1+v0+g3)/2.0 - (v4+v2)/4.0;
						if (g1 > g3)
						{
							if (fHGreen > g1 || fHGreen < g3)
								fHGreen = (fabs(v0-v4) < fabs(v0-v2)) ? g1 + (v0-v4)/2.0 : g3 + (v0-v2)/2.0;
							if  (fHGreen > g1)
								fHGreen = g1;
							else if (fHGreen < g3)
								fHGreen = g3;
						}
						else
						{
							if (fHGreen < g1 || fHGreen > g3)
								fHGreen = (fabs(v0-v4) < fabs(v0-v2)) ? g1 + (v0-v4)/2.0 : g3 + (v0-v2)/2.0;
//								fHGreen = (fabs(v2-v0) - fabs(v2-v4)) ? g1 + (v0-v4)/2.0 : g3 + (v0-v2)/2.0;
							if (fHGreen < g1)
								fHGreen = g1;
							else if (fHGreen > g3)
								fHGreen = g3;
						}
					};
					// Vertical green interpolation
					g1 = (wy>0) ? (*(pGrayPixel-lWidth))/fMultiplier : 0;
					g3 = (wy<lHeight-1) ? (*(pGrayPixel+lWidth))/fMultiplier : 0;
					if (g1 == g3)
						fVGreen = g1;
					else
					{
						double	v0, v1, v3;

						v0 = (*pGrayPixel)/fMultiplier;
						v1 = (wy>1) ? (*(pGrayPixel-2*lWidth))/fMultiplier : 0;
						v3 = (wy<lHeight-2) ? (*(pGrayPixel+2*lWidth))/fMultiplier : 0;
						fVGreen = (g1+v0+g3/*+1*/)/2 - (v1+v3)/4;
						if (g1 > g3)
						{
							if (fVGreen > g1 || fVGreen < g3)
								fVGreen = (fabs(v0-v1) < fabs(v0-v3)) ?	g1 + (v0-v1/*+1*/)/2.0 : g3 + (v0-v3/*+1*/)/2.0;
							if (fVGreen > g1)
								fVGreen = g1;
							else if (fVGreen < g3)
								fVGreen = g3;
						}
						else
						{
							if (fVGreen < g1 || fVGreen > g3)
								fVGreen = (fabs(v0-v1) < fabs(v0-v3)) ?	g1 + (v0-v1/*+1*/)/2.0 : g3 + (v0-v3/*+1*/)/2.0;
//								fVGreen = (fabs(v0-v1) - fabs(v0-v3)) ?	g1 + (v0-v1/*+1*/)/2.0 : g3 + (v0-v3/*+1*/)/2.0;
							if (fVGreen < g1)
								fVGreen = g1;
							else if (fVGreen > g3)
								fVGreen = g3;
						}
					};
				};
				break;
			case BAYER_GREEN :
				// Pixel value
				fVGreen = fHGreen = *(pGrayPixel)/fMultiplier;
				break;
			};

			*pHGreenPixel = ClampPixel(fHGreen * fMultiplier);
			*pVGreenPixel = ClampPixel(fVGreen * fMultiplier);

			pHGreenPixel++;
			pVGreenPixel++;
			pGrayPixel++;
		};
	};
	// End of green interpolation

	BOOL						bBlueLine;

	bBlueLine = IsBayerBlueLine(y, pGrayBitmap->GetCFAType(), pGrayBitmap->yOffset());

	// Interpolate red and blue horizontally and vertically
	for (wy = y;wy<lHeight && wy<y+AHDWS;wy++)
	{
		TType *					pGrayPixel;
		TType *					pHGreenPixel;
		TType *					pVGreenPixel;
		TType *					pHRedPixel;
		TType *					pVRedPixel;
		TType *					pHBluePixel;
		TType *					pVBluePixel;

		pGrayPixel		= pBaseGrayPixel + (wy-y)*lWidth;
		pHGreenPixel	= pHBaseGreenPixel + (wy-y)*AHDWS;
		pVGreenPixel	= pVBaseGreenPixel + (wy-y)*AHDWS;
		pHRedPixel		= pHBaseRedPixel + (wy-y)*AHDWS;
		pVRedPixel		= pVBaseRedPixel + (wy-y)*AHDWS;
		pHBluePixel		= pHBaseBluePixel + (wy-y)*AHDWS;
		pVBluePixel		= pVBaseBluePixel + (wy-y)*AHDWS;

		for (wx = x;wx<lWidth && wx <x+AHDWS;wx++)
		{
			BAYERCOLOR			BayerColor;

			BayerColor = pGrayBitmap->GetBayerColor(wx, wy);
			switch (BayerColor)
			{
			case BAYER_BLUE :
			case BAYER_RED :
				{
					//  B G B G B   R G R G R
					//  G R G R G   G B G B G
					//  B G[B]G B   R G[R]G R
					//  G R G R G   G B G B G
					//  B G B G B   R G R G R

					//  v0  G  v1
					//  G  [v] G
					//  v2  G  v3
					if (bBlueLine)
						*pVBluePixel = *pHBluePixel = *pGrayPixel;
					else
						*pVRedPixel = *pHRedPixel = *pGrayPixel;

					double			v0, v1, v2, v3;

					v0 = (wx>0) && (wy>0) ? (*(pGrayPixel-1-lWidth))/fMultiplier : 0;
					v1 = (wx<lWidth-1) && (wy>0) ? (*(pGrayPixel+1-lWidth))/fMultiplier : 0;
					v2 = (wx>0) && (wy<lHeight-1) ? (*(pGrayPixel-1+lWidth))/fMultiplier : 0;
					v3 = (wx<lWidth-1) && (wy<lHeight-1) ? (*(pGrayPixel+1+lWidth))/fMultiplier : 0;

					// Horizontal interpolation
					double			valH;
					double			g, g0, g1, g2, g3;

					g  = (*pHGreenPixel)/fMultiplier;
					g0 = (wx-x>0) && (wy-y>0) ? (*(pHGreenPixel-1-AHDWS))/fMultiplier : 0;
					g1 = (wx-x<AHDWS-1) && (wy-y>0) ? (*(pHGreenPixel+1-AHDWS))/fMultiplier : 0;
					g2 = (wx-x>0) && (wy-y<AHDWS-1) ? (*(pHGreenPixel-1+AHDWS))/fMultiplier : 0;
					g3 = (wx-x<AHDWS-1) && (wy-y<AHDWS-1) ? (*(pHGreenPixel+1+AHDWS))/fMultiplier : 0;

					valH = g + (v0+v1+v2+v3-g0-g1-g2-g3)/4.0;

					if (bBlueLine)
						*pHRedPixel = ClampPixel(valH*fMultiplier);
					else
						*pHBluePixel = ClampPixel(valH*fMultiplier);

					// Vertical interpolation
					double			valV;

					g  = (*pVGreenPixel)/fMultiplier;
					g0 = (wx-x>0) && (wy-y>0) ? (*(pVGreenPixel-1-AHDWS))/fMultiplier : 0;
					g1 = (wx-x<AHDWS-1) && (wy-y>0) ? (*(pVGreenPixel+1-AHDWS))/fMultiplier : 0;
					g2 = (wx-x>0) && (wy-y<AHDWS-1) ? (*(pVGreenPixel-1+AHDWS))/fMultiplier : 0;
					g3 = (wx-x<AHDWS-1) && (wy-y<AHDWS-1) ? (*(pVGreenPixel+1+AHDWS))/fMultiplier : 0;

					valV = g + (v0+v1+v2+v3-g0-g1-g2-g3)/4.0;

					if (bBlueLine)
						*pVRedPixel = ClampPixel(valV*fMultiplier);
					else
						*pVBluePixel = ClampPixel(valV*fMultiplier);
				};
				break;
			case BAYER_GREEN :
				{
					//  G B G B G    G R G R G
					//  R G R G R    B G B G B
					//  G B[G]B G    G R[G]R G
					//  R G R G R    B G B G B
					//  G B G B G    G R G R G
					double		g, v1, v2, valV, valH;

					// interpolating horizontally and vertically
					//  v1 [v] v2

					g = (*pGrayPixel)/fMultiplier;
					v1 = (wx>0) ? (*(pGrayPixel-1))/fMultiplier : 0;
					v2 = (wx<lWidth-1) ? (*(pGrayPixel+1))/fMultiplier : 0;

					if (v1 == v2)
						valH = valV = v1;
					else
					{
						double			g1, g2;

						g1 = (wx-x>0) ? (*(pVGreenPixel-1))/fMultiplier : 0;
						g2 = (wx-x<AHDWS-1) ? (*(pVGreenPixel+1))/fMultiplier : 0;

						valV = g + (v1+v2-g1-g2)/2.0;
						//valV = Median(v1, v2, valV);

						g1 = (wx-x>0) ? (*(pHGreenPixel-1))/fMultiplier : 0;
						g2 = (wx-x<AHDWS-1) ? (*(pHGreenPixel+1))/fMultiplier : 0;

						valH = (v1+v2)/2.0 + (2.0*g - g1 - g2)/4.0;
						valH = Median(v1, v2, valH);
					};

					if (bBlueLine)
					{
						*pVBluePixel = ClampPixel(valV*fMultiplier);
						*pHBluePixel = ClampPixel(valH*fMultiplier);
					}
					else
					{
						*pVRedPixel = ClampPixel(valV*fMultiplier);
						*pHRedPixel = ClampPixel(valH*fMultiplier);
					};

					// interpolating vertically
					//   v1
					//  [v]
					//   v2
					v1 = (wy>0) ? (*(pGrayPixel-lWidth))/fMultiplier : 0;
					v2 = (wy<lHeight-1) ? (*(pGrayPixel+lWidth))/fMultiplier : 0;

					if (v1 == v2)
						valH = valV = v1;
					else
					{
						double		g1, g2;

						g1 = (wy-y>0) ? (*(pHGreenPixel-AHDWS))/fMultiplier : 0;
						g2 = (wy-y<AHDWS-1) ? (*(pHGreenPixel+AHDWS))/fMultiplier : 0;

						valH = g + (v1 + v2 - g1 - g2)/2.0;
						//valH = Median(v1, v2, valH);

						g1 = (wy-y>0) ? (*(pVGreenPixel-AHDWS))/fMultiplier : 0;
						g2 = (wy-y<AHDWS-1) ? (*(pVGreenPixel+AHDWS))/fMultiplier : 0;

						valV = (v1+v2)/2.0 + (2*g - g1 - g2)/4.0;
						valV = Median(v1, v2, valV);
					};

					if (bBlueLine)
					{
						*pVRedPixel = ClampPixel(valV*fMultiplier);
						*pHRedPixel = ClampPixel(valH*fMultiplier);
					}
					else
					{
						*pVBluePixel = ClampPixel(valV*fMultiplier);
						*pHBluePixel = ClampPixel(valH*fMultiplier);
					};
				};
				break;
			};

			pHRedPixel++;		pHGreenPixel++;			pHBluePixel++;
			pVRedPixel++;		pVGreenPixel++;			pVBluePixel++;
			pGrayPixel++;
		};
		bBlueLine = !bBlueLine;
	};

	// Transform to Lab
	for (wy = y;wy<lHeight && wy<y+AHDWS;wy++)
	{
		TType *					pHGreenPixel;
		TType *					pVGreenPixel;
		TType *					pHRedPixel;
		TType *					pVRedPixel;
		TType *					pHBluePixel;
		TType *					pVBluePixel;
		float *					pHLPixel;
		float *					pHaPixel;
		float *					pHbPixel;
		float *					pVLPixel;
		float *					pVaPixel;
		float *					pVbPixel;

		pHGreenPixel	= pHBaseGreenPixel + (wy-y)*AHDWS;
		pVGreenPixel	= pVBaseGreenPixel + (wy-y)*AHDWS;
		pHRedPixel		= pHBaseRedPixel + (wy-y)*AHDWS;
		pVRedPixel		= pVBaseRedPixel + (wy-y)*AHDWS;
		pHBluePixel		= pHBaseBluePixel + (wy-y)*AHDWS;
		pVBluePixel		= pVBaseBluePixel + (wy-y)*AHDWS;

		pHLPixel		= pHBaseLPixel + (wy-y)*AHDWS;
		pHaPixel		= pHBaseaPixel + (wy-y)*AHDWS;
		pHbPixel		= pHBasebPixel + (wy-y)*AHDWS;
		pVLPixel		= pVBaseLPixel + (wy-y)*AHDWS;
		pVaPixel		= pVBaseaPixel + (wy-y)*AHDWS;
		pVbPixel		= pVBasebPixel + (wy-y)*AHDWS;

		for (wx = x;wx<lWidth && wx <x+AHDWS;wx++)
		{
			double				fRed, fGreen, fBlue;
			double				L, a, b;

			fRed	= (*pHRedPixel)/fMultiplier;
			fGreen	= (*pHGreenPixel)/fMultiplier;
			fBlue	= (*pHBluePixel)/fMultiplier;
			RGBToLab.RGBToLab(fRed, fGreen, fBlue, L, a, b);
			*pHLPixel = L;
			*pHaPixel = a;
			*pHbPixel = b;

			fRed	= (*pVRedPixel)/fMultiplier;
			fGreen	= (*pVGreenPixel)/fMultiplier;
			fBlue	= (*pVBluePixel)/fMultiplier;
			RGBToLab.RGBToLab(fRed, fGreen, fBlue, L, a, b);
			*pVLPixel = L;
			*pVaPixel = a;
			*pVbPixel = b;

			pHRedPixel++;	pHGreenPixel++;	pHBluePixel++;;
			pVRedPixel++;	pVGreenPixel++;	pVBluePixel++;;

			pHLPixel++;		pHaPixel++;		pHbPixel++;
			pVLPixel++;		pVaPixel++;		pVbPixel++;
		};
	};

	// Build homogeneity maps from the CIELab images
	double								lDiffH[4],
										lDiffV[4],
										abDiffH[4],
										abDiffV[4],
										lEpsilon,
										abEpsilon;
	const int							dir[4] = {-1, 1, -AHDWS, AHDWS};

	for (wy = y+1;wy<lHeight && wy<y+AHDWS-1;wy++)
	{
		float *					pHLPixel;
		float *					pHaPixel;
		float *					pHbPixel;
		float *					pVLPixel;
		float *					pVaPixel;
		float *					pVbPixel;
		BYTE *					pHHomoPixel;
		BYTE *					pVHomoPixel;

		pHLPixel		= pHBaseLPixel + (wy-y)*AHDWS;
		pHaPixel		= pHBaseaPixel + (wy-y)*AHDWS;
		pHbPixel		= pHBasebPixel + (wy-y)*AHDWS;
		pVLPixel		= pVBaseLPixel + (wy-y)*AHDWS;
		pVaPixel		= pVBaseaPixel + (wy-y)*AHDWS;
		pVbPixel		= pVBasebPixel + (wy-y)*AHDWS;
		pHHomoPixel		= pHBaseHomoPixel + (wy-y)*AHDWS;
		pVHomoPixel		= pVBaseHomoPixel + (wy-y)*AHDWS;

		for (wx = x;wx<lWidth && wx <x+AHDWS;wx++)
		{
			LONG				i;

			*pHHomoPixel = *pVHomoPixel = 0;

			// For each pixel in the V and H approximations
			// iterate over its neighbors
			for(i=0; i<4; i++)
			{
				lDiffH[i] = fabs((*pHLPixel) - (*(pHLPixel+dir[i])));
				lDiffV[i] = fabs((*pVLPixel) - (*(pVLPixel+dir[i])));
			}

			lEpsilon = min(max(lDiffH[0], lDiffH[1]), max(lDiffV[2], lDiffV[3]));

			for(i=0; i<4; i++)
			{
				double	aDiff, bDiff;

				if (lDiffH[i] <= lEpsilon || i < 2)
				{
					aDiff = ((*pHaPixel)-(*(pHaPixel+dir[i])));
					bDiff = ((*pHbPixel)-(*(pHbPixel+dir[i])));

					abDiffH[i] = aDiff*aDiff + bDiff*bDiff;
				}
				else
					abDiffH[i] = 0;

				if (lDiffV[i] <= lEpsilon || i >= 2)
				{
					aDiff = ((*pVaPixel)-(*(pVaPixel+dir[i])));
					bDiff = ((*pVbPixel)-(*(pVbPixel+dir[i])));
					abDiffV[i] = aDiff*aDiff + bDiff*bDiff;
				}
				else
					abDiffV[i] = 0;
			}

			abEpsilon = min(max(abDiffH[0], abDiffH[1]), max(abDiffV[2], abDiffV[3]));

			// iterate over neighbors
			for(i=0; i<4; ++i)
			{
				if ((lDiffH[i] <= lEpsilon) && (abDiffH[i] <= abEpsilon))
					(*pHHomoPixel)++;
				if ((lDiffV[i] <= lEpsilon) && (abDiffV[i] <= abEpsilon))
					(*pVHomoPixel)++;;
			}

			pHHomoPixel++;		pVHomoPixel++;
			pHLPixel++;			pHaPixel++;		pHbPixel++;
			pVLPixel++;			pVaPixel++;		pVbPixel++;
		};
	};

	// Combine the most homogenous pixels for the final result
	for (wy = y+1;wy<lHeight-1 && wy<y+AHDWS-1;wy++)
	{
		TType *					pHGreenPixel;
		TType *					pVGreenPixel;
		TType *					pHRedPixel;
		TType *					pVRedPixel;
		TType *					pHBluePixel;
		TType *					pVBluePixel;
		BYTE *					pHHomoPixel;
		BYTE *					pVHomoPixel;
		TType *					pOutputRedPixel;
		TType *					pOutputGreenPixel;
		TType *					pOutputBluePixel;

		pOutputRedPixel		= pBaseOutputRedPixel + (wy-y) * lWidth + 1 ;
		pOutputGreenPixel	= pBaseOutputGreenPixel + (wy-y) * lWidth + 1;
		pOutputBluePixel	= pBaseOutputBluePixel + (wy-y) * lWidth + 1;

		pHGreenPixel	= pHBaseGreenPixel + (wy-y)*AHDWS+1;
		pVGreenPixel	= pVBaseGreenPixel + (wy-y)*AHDWS+1;
		pHRedPixel		= pHBaseRedPixel + (wy-y)*AHDWS+1;
		pVRedPixel		= pVBaseRedPixel + (wy-y)*AHDWS+1;
		pHBluePixel		= pHBaseBluePixel + (wy-y)*AHDWS+1;
		pVBluePixel		= pVBaseBluePixel + (wy-y)*AHDWS+1;
		pHHomoPixel		= pHBaseHomoPixel + (wy-y)*AHDWS+1;
		pVHomoPixel		= pVBaseHomoPixel + (wy-y)*AHDWS+1;

		for (wx = x+1;wx<lWidth-1 && wx <x+AHDWS-1;wx++)
		{
			LONG			hmV,
							hmH;

			hmV = *(pVHomoPixel) + (*(pVHomoPixel-1)) + (*(pVHomoPixel+1)) +
				  (*(pVHomoPixel-AHDWS)) + (*(pVHomoPixel-AHDWS-1)) + (*(pVHomoPixel-AHDWS+1)) +
				  (*(pVHomoPixel+AHDWS)) + (*(pVHomoPixel+AHDWS-1)) + (*(pVHomoPixel+AHDWS+1));

			hmH = *(pHHomoPixel) + (*(pHHomoPixel-1)) + (*(pHHomoPixel+1)) +
				  (*(pHHomoPixel-AHDWS)) + (*(pHHomoPixel-AHDWS-1)) + (*(pHHomoPixel-AHDWS+1)) +
				  (*(pHHomoPixel+AHDWS)) + (*(pHHomoPixel+AHDWS-1)) + (*(pHHomoPixel+AHDWS+1));

			if (hmV > hmH)
			{
				*pOutputRedPixel	= *pVRedPixel;
				*pOutputGreenPixel	= *pVGreenPixel;
				*pOutputBluePixel	= *pVBluePixel;
			}
			else if (hmV < hmH)
			{
				*pOutputRedPixel	= *pHRedPixel;
				*pOutputGreenPixel	= *pHGreenPixel;
				*pOutputBluePixel	= *pHBluePixel;
			}
			else
			{
				*pOutputRedPixel	= ((*pVRedPixel)	+ (*pHRedPixel))/2.0;
				*pOutputGreenPixel	= ((*pVGreenPixel)	+ (*pHGreenPixel))/2.0;
				*pOutputBluePixel	= ((*pVBluePixel)	+ (*pHBluePixel))/2.0;
			}

			pHHomoPixel++;		pVHomoPixel++;
			pHRedPixel++;		pHGreenPixel++;			pHBluePixel++;;
			pVRedPixel++;		pVGreenPixel++;			pVBluePixel++;;
			pOutputRedPixel++;	pOutputGreenPixel++;	pOutputBluePixel++;
		};
	};
};

/* ------------------------------------------------------------------- */

template <typename TType>
inline BOOL	CAHDTask<TType>::DoTask(HANDLE hEvent)
{
	LONG						x, y;
	BOOL						bEnd = FALSE;
	MSG							msg;
	CAHDTaskVariables<TType>	var;

	if (var.Init())
	{
		// Create a message queue and signal the event
		PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE);
		SetEvent(hEvent);
		while (!bEnd && GetMessage(&msg, nullptr, 0, 0))
		{
			if (msg.message == WM_MT_PROCESS)
			{
				x = msg.wParam;
				y = msg.lParam;
				DoSubWindow(x, y, var);
				SetEvent(hEvent);
			}
			else if (msg.message == WM_MT_STOP)
				bEnd = TRUE;
		};
	};

	return TRUE;
};

/* ------------------------------------------------------------------- */

template <typename TType>
inline void	CAHDTask<TType>::InterpolateBorders()
{
	LONG							x, y;
	TType *							pGrayPixel1;
	TType *							pGrayPixel2;
	TType *							pOutputRedPixel1;
	TType *							pOutputRedPixel2;
	TType *							pOutputGreenPixel1;
	TType *							pOutputGreenPixel2;
	TType *							pOutputBluePixel1;
	TType *							pOutputBluePixel2;
	LONG							x1 = 0,
									x2 = lWidth-1;
	BOOL							bBlueLine;

	pGrayPixel1			= pGrayBitmap->GetGrayPixel(0, 1);
	pGrayPixel2			= pGrayBitmap->GetGrayPixel(lWidth-1, 1);
	pOutputRedPixel1	= pColorBitmap->GetRedPixel(0, 1);
	pOutputGreenPixel1	= pColorBitmap->GetGreenPixel(0, 1);
	pOutputBluePixel1	= pColorBitmap->GetBluePixel(0, 1);
	pOutputRedPixel2	= pColorBitmap->GetRedPixel(lWidth-1, 1);
	pOutputGreenPixel2	= pColorBitmap->GetGreenPixel(lWidth-1, 1);
	pOutputBluePixel2	= pColorBitmap->GetBluePixel(lWidth-1, 1);
	// Vertical interpolation
	bBlueLine = IsBayerBlueLine(1, pGrayBitmap->GetCFAType(), pGrayBitmap->yOffset());
	for (y = 1;y<lHeight-1;y++)
	{
		BAYERCOLOR				BayerColor;
		TType					v1, v2;

		BayerColor = pGrayBitmap->GetBayerColor(x1, y);
		v1 = *(pGrayPixel1-lWidth);
		v2 = *(pGrayPixel1+lWidth);
		switch (BayerColor)
		{
		case BAYER_GREEN :
			*pOutputGreenPixel1 = *pGrayPixel1;
			if (bBlueLine)
			{
				*pOutputBluePixel1 = *(pGrayPixel1+1);
				*pOutputRedPixel1 = (v1+v2)/2.0;
			}
			else
			{
				*pOutputRedPixel1 = *(pGrayPixel1+1);
				*pOutputBluePixel1 = (v1+v2)/2.0;
			};
			break;
		case BAYER_RED :
			*pOutputRedPixel1 = *pGrayPixel1;
			*pOutputGreenPixel1 = (v1+v2)/2.0;
			*pOutputBluePixel1  = *(pOutputBluePixel1+1);
			break;
		case BAYER_BLUE :
			*pOutputBluePixel1 = *pGrayPixel1;
			*pOutputGreenPixel1 = (v1+v2)/2.0;
			*pOutputRedPixel1   = *(pOutputRedPixel1+1);
			break;
		};

		BayerColor = pGrayBitmap->GetBayerColor(x2, y);
		v1 = *(pGrayPixel2-lWidth);
		v2 = *(pGrayPixel2+lWidth);
		switch (BayerColor)
		{
		case BAYER_GREEN :
			*pOutputGreenPixel2 = *pGrayPixel2;
			if (bBlueLine)
			{
				*pOutputBluePixel2 = *(pGrayPixel2-1);
				*pOutputRedPixel2 = (v1+v2)/2.0;
			}
			else
			{
				*pOutputRedPixel2 = *(pGrayPixel2-1);
				*pOutputBluePixel2 = (v1+v2)/2.0;
			};
			break;
		case BAYER_RED :
			*pOutputRedPixel2 = *pGrayPixel2;
			*pOutputGreenPixel2 = (v1+v2)/2.0;
			*pOutputBluePixel2  = *(pOutputBluePixel2-1);
			break;
		case BAYER_BLUE :
			*pOutputBluePixel2 = *pGrayPixel2;
			*pOutputGreenPixel2 = (v1+v2)/2.0;
			*pOutputRedPixel2   = *(pOutputRedPixel2-1);
			break;
		};

		pOutputRedPixel1+=lWidth;		pOutputGreenPixel1+=lWidth;	pOutputBluePixel1+=lWidth;
		pOutputRedPixel2+=lWidth;		pOutputGreenPixel2+=lWidth;	pOutputBluePixel2+=lWidth;
		pGrayPixel1+=lWidth;			pGrayPixel2+=lWidth;

		bBlueLine = !bBlueLine;
	};

	LONG								y1 = 0,
										y2 = lHeight-1;
	BOOL								bBlueLine1,
										bBlueLine2;

	pGrayPixel1			= pGrayBitmap->GetGrayPixel(1, 0);
	pGrayPixel2			= pGrayBitmap->GetGrayPixel(1, lHeight-1);
	pOutputRedPixel1	= pColorBitmap->GetRedPixel(1, 0);
	pOutputGreenPixel1	= pColorBitmap->GetGreenPixel(1, 0);
	pOutputBluePixel1	= pColorBitmap->GetBluePixel(1, 0);
	pOutputRedPixel2	= pColorBitmap->GetRedPixel(1, lHeight-1);
	pOutputGreenPixel2	= pColorBitmap->GetGreenPixel(1, lHeight-1);
	pOutputBluePixel2	= pColorBitmap->GetBluePixel(1, lHeight-1);
	// Vertical interpolation
	bBlueLine1 = IsBayerBlueLine(0, pGrayBitmap->GetCFAType(), pGrayBitmap->yOffset());
	bBlueLine2 = IsBayerBlueLine(lHeight-1, pGrayBitmap->GetCFAType(), pGrayBitmap->yOffset());
	for (x = 1;x<lWidth-1;x++)
	{
		BAYERCOLOR				BayerColor;
		TType					v1, v2;

		BayerColor = pGrayBitmap->GetBayerColor(x, y1);
		v1 = *(pGrayPixel1-1);
		v2 = *(pGrayPixel1+1);
		switch (BayerColor)
		{
		case BAYER_GREEN :
			*pOutputGreenPixel1 = *pGrayPixel1;
			if (bBlueLine1)
			{
				*pOutputBluePixel1 = (v1+v2)/2.0;
				*pOutputRedPixel1 = *(pGrayPixel1+lWidth);
			}
			else
			{
				*pOutputBluePixel1 = *(pGrayPixel1+lWidth);
				*pOutputRedPixel1 = (v1+v2)/2.0;
			};
			break;
		case BAYER_RED :
			*pOutputRedPixel1 = *pGrayPixel1;
			*pOutputGreenPixel1 = (v1+v2)/2.0;
			*pOutputBluePixel1  = *(pOutputBluePixel1+lWidth);
			break;
		case BAYER_BLUE :
			*pOutputBluePixel1 = *pGrayPixel1;
			*pOutputGreenPixel1 = (v1+v2)/2.0;
			*pOutputRedPixel1   = *(pOutputRedPixel1+lWidth);
			break;
		};

		BayerColor = pGrayBitmap->GetBayerColor(x, y2);
		v1 = *(pGrayPixel2-1);
		v2 = *(pGrayPixel2+1);
		switch (BayerColor)
		{
		case BAYER_GREEN :
			*pOutputGreenPixel2 = *pGrayPixel2;
			if (bBlueLine2)
			{
				*pOutputBluePixel2 = (v1+v2)/2.0;
				*pOutputRedPixel2 = *(pGrayPixel2-lWidth);
			}
			else
			{
				*pOutputRedPixel2 = (v1+v2)/2.0;
				*pOutputBluePixel2 = *(pGrayPixel2-lWidth);
			};
			break;
		case BAYER_RED :
			*pOutputRedPixel2 = *pGrayPixel2;
			*pOutputGreenPixel2 = (v1+v2)/2.0;
			*pOutputBluePixel2  = *(pOutputBluePixel2-lWidth);
			break;
		case BAYER_BLUE :
			*pOutputBluePixel2 = *pGrayPixel2;
			*pOutputGreenPixel2 = (v1+v2)/2.0;
			*pOutputRedPixel2   = *(pOutputRedPixel2-lWidth);
			break;
		};

		pOutputRedPixel1++;		pOutputGreenPixel1++;	pOutputBluePixel1++;
		pOutputRedPixel2++;		pOutputGreenPixel2++;	pOutputBluePixel2++;
		pGrayPixel1++;			pGrayPixel2++;
	};

	// Interpolate the 4 corners
	pOutputRedPixel1	= pColorBitmap->GetRedPixel(0, 0);
	pOutputGreenPixel1	= pColorBitmap->GetGreenPixel(0, 0);
	pOutputBluePixel1	= pColorBitmap->GetBluePixel(0, 0);

	*pOutputRedPixel1   = ((*(pOutputRedPixel1+1)) + (*(pOutputRedPixel1+lWidth)) + (*(pOutputRedPixel1+1+lWidth)))/3.0;
	*pOutputGreenPixel1 = ((*(pOutputGreenPixel1+1)) + (*(pOutputGreenPixel1+lWidth)) + (*(pOutputGreenPixel1+1+lWidth)))/3.0;
	*pOutputBluePixel1  = ((*(pOutputBluePixel1+1)) + (*(pOutputBluePixel1+lWidth)) + (*(pOutputBluePixel1+1+lWidth)))/3.0;

	pOutputRedPixel1	= pColorBitmap->GetRedPixel(0, lHeight-1);
	pOutputGreenPixel1	= pColorBitmap->GetGreenPixel(0, lHeight-1);
	pOutputBluePixel1	= pColorBitmap->GetBluePixel(0, lHeight-1);

	*pOutputRedPixel1   = ((*(pOutputRedPixel1+1)) + (*(pOutputRedPixel1-lWidth)) + (*(pOutputRedPixel1+1-lWidth)))/3.0;
	*pOutputGreenPixel1   = ((*(pOutputGreenPixel1+1)) + (*(pOutputGreenPixel1-lWidth)) + (*(pOutputGreenPixel1+1-lWidth)))/3.0;
	*pOutputBluePixel1   = ((*(pOutputBluePixel1+1)) + (*(pOutputBluePixel1-lWidth)) + (*(pOutputBluePixel1+1-lWidth)))/3.0;

	pOutputRedPixel1	= pColorBitmap->GetRedPixel(lWidth-1, lHeight-1);
	pOutputGreenPixel1	= pColorBitmap->GetGreenPixel(lWidth-1, lHeight-1);
	pOutputBluePixel1	= pColorBitmap->GetBluePixel(lWidth-1, lHeight-1);

	*pOutputRedPixel1   = ((*(pOutputRedPixel1-1)) + (*(pOutputRedPixel1-lWidth)) + (*(pOutputRedPixel1-1-lWidth)))/3.0;
	*pOutputGreenPixel1   = ((*(pOutputGreenPixel1-1)) + (*(pOutputGreenPixel1-lWidth)) + (*(pOutputGreenPixel1-1-lWidth)))/3.0;
	*pOutputBluePixel1   = ((*(pOutputBluePixel1-1)) + (*(pOutputBluePixel1-lWidth)) + (*(pOutputBluePixel1-1-lWidth)))/3.0;

	pOutputRedPixel1	= pColorBitmap->GetRedPixel(lWidth-1, 0);
	pOutputGreenPixel1	= pColorBitmap->GetGreenPixel(lWidth-1, 0);
	pOutputBluePixel1	= pColorBitmap->GetBluePixel(lWidth-1, 0);

	*pOutputRedPixel1   = ((*(pOutputRedPixel1-1)) + (*(pOutputRedPixel1+lWidth)) + (*(pOutputRedPixel1-1+lWidth)))/3.0;
	*pOutputGreenPixel1   = ((*(pOutputGreenPixel1-1)) + (*(pOutputGreenPixel1+lWidth)) + (*(pOutputGreenPixel1-1+lWidth)))/3.0;
	*pOutputBluePixel1   = ((*(pOutputBluePixel1-1)) + (*(pOutputBluePixel1+lWidth)) + (*(pOutputBluePixel1-1+lWidth)))/3.0;
};

/* ------------------------------------------------------------------- */

template <typename TType>
inline BOOL	AHDDemosaicing(CGrayBitmapT<TType> * pGrayBitmap, CMemoryBitmap ** ppColorBitmap, CDSSProgress * pProgress)
{
	BOOL									bResult;
	CAHDTask<TType>							AHDTask;

	bResult = AHDTask.Init(pGrayBitmap, pProgress);
	if (bResult)
	{
		AHDTask.StartThreads();
		AHDTask.Process();

		AHDTask.InterpolateBorders();
		AHDTask.pOutputBitmap.Attach(AHDTask.pColorBitmap);
		AHDTask.pOutputBitmap.CopyTo(ppColorBitmap);
	};

	return bResult;
};

/* ------------------------------------------------------------------- */

template <typename TType>
inline BOOL	AHDDemosaicing2(CGrayBitmapT<TType> * pGrayBitmap, CMemoryBitmap ** ppColorBitmap, CDSSProgress * pProgress)
{
	BOOL									bResult = FALSE;
	CSmartPtr<CColorBitmapT<TType> >		pColorBitmap;
	CSmartPtr<CMemoryBitmap>				pOutputBitmap;
	LONG									lWidth = pGrayBitmap->Width();
	LONG									lHeight = pGrayBitmap->Height();

	const LONG								AHDWS = 256;
	double									fMultiplier = 65536.0;

	CSmartPtr<CColorBitmapT<TType> >		pWindowV;
	CSmartPtr<CColorBitmapT<TType> >		pWindowH;
	CSmartPtr<C96BitFloatColorBitmap>		pLabWindowV;
	CSmartPtr<C96BitFloatColorBitmap>		pLabWindowH;
	CSmartPtr<C8BitGrayBitmap>				pHomoH;
	CSmartPtr<C8BitGrayBitmap>				pHomoV;

	pWindowV.Attach(new C48BitColorBitmap);
	pWindowH.Attach(new C48BitColorBitmap);
	pLabWindowV.Attach(new C96BitFloatColorBitmap);
	pLabWindowH.Attach(new C96BitFloatColorBitmap);
	pHomoH.Attach(new C8BitGrayBitmap);
	pHomoV.Attach(new C8BitGrayBitmap);

	bResult = pWindowV->Init(AHDWS, AHDWS);
	bResult = bResult && pWindowH->Init(AHDWS, AHDWS);
	bResult = bResult && pLabWindowV->Init(AHDWS, AHDWS);
	bResult = bResult && pLabWindowH->Init(AHDWS, AHDWS);
	bResult = bResult && pHomoH->Init(AHDWS, AHDWS);
	bResult = bResult && pHomoV->Init(AHDWS, AHDWS);

	pColorBitmap.Attach(new CColorBitmapT<TType>);
	bResult = bResult && pColorBitmap->Init(lWidth, lHeight);

	if (bResult)
	{
		CRGBToLab							RGBToLab;
		LONG								x, y;

		LONG								lNrWindows;
		LONG								lNrWindowsWidth,
											lNrWindowsHeight;

		lNrWindowsWidth = lWidth/(AHDWS-4);
		if (lWidth % (AHDWS-4))
			lNrWindowsWidth++;

		lNrWindowsHeight = lHeight/(AHDWS-4);
		if (lHeight % (AHDWS-4))
			lNrWindowsHeight++;

		lNrWindows = lNrWindowsWidth * lNrWindowsHeight;

		if (pProgress)
			pProgress->Start2(nullptr, lNrWindows);

		lNrWindows = 0;

		for (y = 0;y<lHeight;y+=AHDWS-4)
		{
			for (x= 0;x<lWidth;x+=AHDWS-4)
			{
				// Iterate through the window
				LONG						wx, wy;
				TType *						pBaseGrayPixel;
				TType *						pVBaseGreenPixel;
				TType *						pHBaseGreenPixel;
				TType *						pVBaseRedPixel;
				TType *						pHBaseRedPixel;
				TType *						pVBaseBluePixel;
				TType *						pHBaseBluePixel;
				float *						pVBaseLPixel;
				float *						pVBaseaPixel;
				float *						pVBasebPixel;
				float *						pHBaseLPixel;
				float *						pHBaseaPixel;
				float *						pHBasebPixel;
				BYTE *						pHBaseHomoPixel;
				BYTE *						pVBaseHomoPixel;

				TType *						pBaseOutputRedPixel;
				TType *						pBaseOutputGreenPixel;
				TType *						pBaseOutputBluePixel;

				pBaseGrayPixel		= pGrayBitmap->GetGrayPixel(x, y);
				pBaseOutputRedPixel		= pColorBitmap->GetRedPixel(x, y);
				pBaseOutputGreenPixel	= pColorBitmap->GetGreenPixel(x, y);
				pBaseOutputBluePixel	= pColorBitmap->GetBluePixel(x, y);

				pVBaseGreenPixel	= pWindowV->GetGreenPixel(0, 0);
				pHBaseGreenPixel	= pWindowH->GetGreenPixel(0, 0);
				pVBaseRedPixel		= pWindowV->GetRedPixel(0, 0);
				pHBaseRedPixel		= pWindowH->GetRedPixel(0, 0);
				pVBaseBluePixel		= pWindowV->GetBluePixel(0, 0);
				pHBaseBluePixel		= pWindowH->GetBluePixel(0, 0);

				pVBaseLPixel		= pLabWindowV->GetRedPixel(0, 0);
				pVBaseaPixel		= pLabWindowV->GetGreenPixel(0, 0);
				pVBasebPixel		= pLabWindowV->GetBluePixel(0, 0);
				pHBaseLPixel		= pLabWindowH->GetRedPixel(0, 0);
				pHBaseaPixel		= pLabWindowH->GetGreenPixel(0, 0);
				pHBasebPixel		= pLabWindowH->GetBluePixel(0, 0);

				pHBaseHomoPixel		= pHomoH->GetGrayPixel(0, 0);
				pVBaseHomoPixel		= pHomoV->GetGrayPixel(0, 0);

				// Interpolate green horizontally and vertically
				for (wy = y;wy<lHeight && wy<y+AHDWS;wy++)
				{
					TType *					pGrayPixel;
					TType *					pHGreenPixel;
					TType *					pVGreenPixel;

					pGrayPixel = pBaseGrayPixel + (wy-y)*lWidth;
					pHGreenPixel = pHBaseGreenPixel + (wy-y)*AHDWS;
					pVGreenPixel = pVBaseGreenPixel + (wy-y)*AHDWS;

					for (wx = x;wx<lWidth && wx <x+AHDWS;wx++)
					{
						BAYERCOLOR			BayerColor;
                        double				fVGreen = 0;
                        double              fHGreen = 0;

						BayerColor = pGrayBitmap->GetBayerColor(wx, wy);
						switch (BayerColor)
						{
						case BAYER_BLUE :
						case BAYER_RED :
							{
								// Blue and Red line and column
								// Horizontal green interpolation
								double		g1, g3;

								g1 = (wx > 0) ? (*(pGrayPixel-1))/fMultiplier : 0;
								g3 = (wx < lWidth-1) ? (*(pGrayPixel+1))/fMultiplier : 0;

								if (g1 == g3)
									fHGreen = g1;
								else
								{
									double	v0, v2, v4;

									v0 = (*pGrayPixel)/fMultiplier;
									v4 = (wx > 1) ? (*(pGrayPixel-2))/fMultiplier : 0;
									v2 = (wx < lWidth-2) ? (*(pGrayPixel+2))/fMultiplier : 0;
									fHGreen = (g1+v0+g3)/2.0 - (v4+v2)/4.0;
									if (g1 > g3)
									{
										if (fHGreen > g1 || fHGreen < g3)
											fHGreen = (fabs(v0-v4) < fabs(v0-v2)) ? g1 + (v0-v4)/2.0 : g3 + (v0-v2)/2.0;
										if  (fHGreen > g1)
											fHGreen = g1;
										else if (fHGreen < g3)
											fHGreen = g3;
									}
									else
									{
										if (fHGreen < g1 || fHGreen > g3)
											fHGreen = (fabs(v2-v0) - fabs(v2-v4)) ? g1 + (v0-v4)/2.0 : g3 + (v0-v2)/2.0;
										if (fHGreen < g1)
											fHGreen = g1;
										else if (fHGreen > g3)
											fHGreen = g3;
									}
								};
								// Vertical green interpolation
								g1 = (wy>0) ? (*(pGrayPixel-lWidth))/fMultiplier : 0;
								g3 = (wy<lHeight-1) ? (*(pGrayPixel+lWidth))/fMultiplier : 0;
								if (g1 == g3)
									fVGreen = g1;
								else
								{
									double	v0, v1, v3;

									v0 = (*pGrayPixel)/fMultiplier;
									v1 = (wy>1) ? (*(pGrayPixel-2*lWidth))/fMultiplier : 0;
									v3 = (wy<lHeight-2) ? (*(pGrayPixel+2*lWidth))/fMultiplier : 0;
									fVGreen = (g1+v0+g3/*+1*/)/2 - (v1+v3)/4;
									if (g1 > g3)
									{
										if (fVGreen > g1 || fVGreen < g3)
											fVGreen = (fabs(v0-v1) < fabs(v0-v3)) ?	g1 + (v0-v1/*+1*/)/2.0 : g3 + (v0-v3/*+1*/)/2.0;
										if (fVGreen > g1)
											fVGreen = g1;
										else if (fVGreen < g3)
											fVGreen = g3;
									}
									else
									{
										if (fVGreen < g1 || fVGreen > g3)
											fVGreen = (fabs(v0-v1) - fabs(v0-v3)) ?	g1 + (v0-v1/*+1*/)/2.0 : g3 + (v0-v3/*+1*/)/2.0;
										if (fVGreen < g1)
											fVGreen = g1;
										else if (fVGreen > g3)
											fVGreen = g3;
									}
								};
							};
							break;
						case BAYER_GREEN :
							// Pixel value
							fVGreen = fHGreen = *(pGrayPixel)/fMultiplier;
							break;
						};

						*pHGreenPixel = ClampPixel(fHGreen * fMultiplier);
						*pVGreenPixel = ClampPixel(fVGreen * fMultiplier);

						pHGreenPixel++;
						pVGreenPixel++;
						pGrayPixel++;
					};
				};
				// End of green interpolation

				BOOL						bBlueLine;

				bBlueLine = IsBayerBlueLine(y, pGrayBitmap->GetCFAType(), pGrayBitmap->yOffset());

				// Interpolate red and blue horizontally and vertically
				for (wy = y;wy<lHeight && wy<y+AHDWS;wy++)
				{
					TType *					pGrayPixel;
					TType *					pHGreenPixel;
					TType *					pVGreenPixel;
					TType *					pHRedPixel;
					TType *					pVRedPixel;
					TType *					pHBluePixel;
					TType *					pVBluePixel;

					pGrayPixel		= pBaseGrayPixel + (wy-y)*lWidth;
					pHGreenPixel	= pHBaseGreenPixel + (wy-y)*AHDWS;
					pVGreenPixel	= pVBaseGreenPixel + (wy-y)*AHDWS;
					pHRedPixel		= pHBaseRedPixel + (wy-y)*AHDWS;
					pVRedPixel		= pVBaseRedPixel + (wy-y)*AHDWS;
					pHBluePixel		= pHBaseBluePixel + (wy-y)*AHDWS;
					pVBluePixel		= pVBaseBluePixel + (wy-y)*AHDWS;

					for (wx = x;wx<lWidth && wx <x+AHDWS;wx++)
					{
						BAYERCOLOR			BayerColor;

						BayerColor = pGrayBitmap->GetBayerColor(wx, wy);
						switch (BayerColor)
						{
						case BAYER_BLUE :
						case BAYER_RED :
							{
								//  B G B G B   R G R G R
								//  G R G R G   G B G B G
								//  B G[B]G B   R G[R]G R
								//  G R G R G   G B G B G
								//  B G B G B   R G R G R

								//  v0  G  v1
								//  G  [v] G
								//  v2  G  v3
								if (bBlueLine)
									*pVBluePixel = *pHBluePixel = *pGrayPixel;
								else
									*pVRedPixel = *pHRedPixel = *pGrayPixel;

								double			v0, v1, v2, v3;

								v0 = (wx>0) && (wy>0) ? (*(pGrayPixel-1-lWidth))/fMultiplier : 0;
								v1 = (wx<lWidth-1) && (wy>0) ? (*(pGrayPixel+1-lWidth))/fMultiplier : 0;
								v2 = (wx>0) && (wy<lHeight-1) ? (*(pGrayPixel-1+lWidth))/fMultiplier : 0;
								v3 = (wx<lWidth-1) && (wy<lHeight-1) ? (*(pGrayPixel+1+lWidth))/fMultiplier : 0;

								// Horizontal interpolation
								double			valH;
								double			g, g0, g1, g2, g3;

								g  = (*pHGreenPixel)/fMultiplier;
								g0 = (wx-x>0) && (wy-y>0) ? (*(pHGreenPixel-1-AHDWS))/fMultiplier : 0;
								g1 = (wx-x<AHDWS-1) && (wy-y>0) ? (*(pHGreenPixel+1-AHDWS))/fMultiplier : 0;
								g2 = (wx-x>0) && (wy-y<AHDWS-1) ? (*(pHGreenPixel-1+AHDWS))/fMultiplier : 0;
								g3 = (wx-x<AHDWS-1) && (wy-y<AHDWS-1) ? (*(pHGreenPixel+1+AHDWS))/fMultiplier : 0;

								valH = g + (v0+v1+v2+v3-g0-g1-g2-g3)/4.0;

								if (bBlueLine)
									*pHRedPixel = ClampPixel(valH*fMultiplier);
								else
									*pHBluePixel = ClampPixel(valH*fMultiplier);

								// Vertical interpolation
								double			valV;

								g  = (*pVGreenPixel)/fMultiplier;
								g0 = (wx-x>0) && (wy-y>0) ? (*(pVGreenPixel-1-AHDWS))/fMultiplier : 0;
								g1 = (wx-x<AHDWS-1) && (wy-y>0) ? (*(pVGreenPixel+1-AHDWS))/fMultiplier : 0;
								g2 = (wx-x>0) && (wy-y<AHDWS-1) ? (*(pVGreenPixel-1+AHDWS))/fMultiplier : 0;
								g3 = (wx-x<AHDWS-1) && (wy-y<AHDWS-1) ? (*(pVGreenPixel+1+AHDWS))/fMultiplier : 0;

								valV = g + (v0+v1+v2+v3-g0-g1-g2-g3)/4.0;

								if (bBlueLine)
									*pVRedPixel = ClampPixel(valV*fMultiplier);
								else
									*pVBluePixel = ClampPixel(valV*fMultiplier);
							};
							break;
						case BAYER_GREEN :
							{
								//  G B G B G    G R G R G
								//  R G R G R    B G B G B
								//  G B[G]B G    G R[G]R G
								//  R G R G R    B G B G B
								//  G B G B G    G R G R G
								double		g, v1, v2, valV, valH;

								// interpolating horizontally and vertically
								//  v1 [v] v2

								g = (*pGrayPixel)/fMultiplier;
								v1 = (wx>0) ? (*(pGrayPixel-1))/fMultiplier : 0;
								v2 = (wx<lWidth-1) ? (*(pGrayPixel+1))/fMultiplier : 0;

								if (v1 == v2)
									valH = valV = v1;
								else
								{
									double			g1, g2;

									g1 = (wx-x>0) ? (*(pVGreenPixel-1))/fMultiplier : 0;
									g2 = (wx-x<AHDWS-1) ? (*(pVGreenPixel+1))/fMultiplier : 0;

									valV = g + (v1+v2-g1-g2)/2.0;

									g1 = (wx-x>0) ? (*(pHGreenPixel-1))/fMultiplier : 0;
									g2 = (wx-x<AHDWS-1) ? (*(pHGreenPixel+1))/fMultiplier : 0;

									valH = (v1+v2)/2.0 + (2.0*g - g1 - g2)/4.0;
									if (v1 > v2)
									{
										if (valH > v1)
											valH = v1;
										else if (valH < v2)
											valH = v2;
									}
									else
									{
										if (valH < v1)
											valH = v1;
										else if (valH > v2)
											valH = v2;
									};
								};

								if (bBlueLine)
								{
									*pVBluePixel = ClampPixel(valV*fMultiplier);
									*pHBluePixel = ClampPixel(valH*fMultiplier);
								}
								else
								{
									*pVRedPixel = ClampPixel(valV*fMultiplier);
									*pHRedPixel = ClampPixel(valH*fMultiplier);
								};

								// interpolating vertically
								//   v1
								//  [v]
								//   v2
								v1 = (wy>0) ? (*(pGrayPixel-lWidth))/fMultiplier : 0;
								v2 = (wy<lHeight-1) ? (*(pGrayPixel+lWidth))/fMultiplier : 0;

								if (v1 == v2)
									valH = valV = v1;
								else
								{
									double		g1, g2;

									g1 = (wy-y>0) ? (*(pHGreenPixel-AHDWS))/fMultiplier : 0;
									g2 = (wy-y<AHDWS-1) ? (*(pHGreenPixel+AHDWS))/fMultiplier : 0;

									valH = g + (v1 + v2 - g1 - g2)/2.0;

									g1 = (wy-y>0) ? (*(pVGreenPixel-AHDWS))/fMultiplier : 0;
									g2 = (wy-y<AHDWS-1) ? (*(pVGreenPixel+AHDWS))/fMultiplier : 0;

									valV = (v1+v2)/2.0 + (2*g - g1 - g2)/2.0;

									if (v1 > v2)
									{
										if (valV > v1)
											valV = v1;
										else if (valV < v2)
											valV = v2;
									}
									else
									{
										if (valV < v1)
											valV = v1;
										else if (valV > v2)
											valV = v2;
									};
								};

								if (bBlueLine)
								{
									*pVRedPixel = ClampPixel(valV*fMultiplier);
									*pHRedPixel = ClampPixel(valH*fMultiplier);
								}
								else
								{
									*pVBluePixel = ClampPixel(valV*fMultiplier);
									*pHBluePixel = ClampPixel(valH*fMultiplier);
								};
							};
							break;
						};

						pHRedPixel++;		pHGreenPixel++;			pHBluePixel++;
						pVRedPixel++;		pVGreenPixel++;			pVBluePixel++;
						pGrayPixel++;
					};
					bBlueLine = !bBlueLine;
				};

				// Transform to Lab
				for (wy = y;wy<lHeight && wy<y+AHDWS;wy++)
				{
					TType *					pHGreenPixel;
					TType *					pVGreenPixel;
					TType *					pHRedPixel;
					TType *					pVRedPixel;
					TType *					pHBluePixel;
					TType *					pVBluePixel;
					float *					pHLPixel;
					float *					pHaPixel;
					float *					pHbPixel;
					float *					pVLPixel;
					float *					pVaPixel;
					float *					pVbPixel;

					pHGreenPixel	= pHBaseGreenPixel + (wy-y)*AHDWS;
					pVGreenPixel	= pVBaseGreenPixel + (wy-y)*AHDWS;
					pHRedPixel		= pHBaseRedPixel + (wy-y)*AHDWS;
					pVRedPixel		= pVBaseRedPixel + (wy-y)*AHDWS;
					pHBluePixel		= pHBaseBluePixel + (wy-y)*AHDWS;
					pVBluePixel		= pVBaseBluePixel + (wy-y)*AHDWS;

					pHLPixel		= pHBaseLPixel + (wy-y)*AHDWS;
					pHaPixel		= pHBaseaPixel + (wy-y)*AHDWS;
					pHbPixel		= pHBasebPixel + (wy-y)*AHDWS;
					pVLPixel		= pVBaseLPixel + (wy-y)*AHDWS;
					pVaPixel		= pVBaseaPixel + (wy-y)*AHDWS;
					pVbPixel		= pVBasebPixel + (wy-y)*AHDWS;

					for (wx = x;wx<lWidth && wx <x+AHDWS;wx++)
					{
						double				fRed, fGreen, fBlue;
						double				L, a, b;

						fRed	= (*pHRedPixel)/fMultiplier;
						fGreen	= (*pHGreenPixel)/fMultiplier;
						fBlue	= (*pHBluePixel)/fMultiplier;
						RGBToLab.RGBToLab(fRed, fGreen, fBlue, L, a, b);
						*pHLPixel = L;
						*pHaPixel = a;
						*pHbPixel = b;

						fRed	= (*pVRedPixel)/fMultiplier;
						fGreen	= (*pVGreenPixel)/fMultiplier;
						fBlue	= (*pVBluePixel)/fMultiplier;
						RGBToLab.RGBToLab(fRed, fGreen, fBlue, L, a, b);
						*pVLPixel = L;
						*pVaPixel = a;
						*pVbPixel = b;

						pHRedPixel++;	pHGreenPixel++;	pHBluePixel++;;
						pVRedPixel++;	pVGreenPixel++;	pVBluePixel++;;

						pHLPixel++;		pHaPixel++;		pHbPixel++;
						pVLPixel++;		pVaPixel++;		pVbPixel++;
					};
				};

				// Build homogeneity maps from the CIELab images
				double								lDiffH[4],
													lDiffV[4],
													abDiffH[4],
													abDiffV[4],
													lEpsilon,
													abEpsilon;
				const int							dir[4] = {-1, 1, -AHDWS, AHDWS};

				for (wy = y+1;wy<lHeight && wy<y+AHDWS-1;wy++)
				{
					float *					pHLPixel;
					float *					pHaPixel;
					float *					pHbPixel;
					float *					pVLPixel;
					float *					pVaPixel;
					float *					pVbPixel;
					BYTE *					pHHomoPixel;
					BYTE *					pVHomoPixel;

					pHLPixel		= pHBaseLPixel + (wy-y)*AHDWS;
					pHaPixel		= pHBaseaPixel + (wy-y)*AHDWS;
					pHbPixel		= pHBasebPixel + (wy-y)*AHDWS;
					pVLPixel		= pVBaseLPixel + (wy-y)*AHDWS;
					pVaPixel		= pVBaseaPixel + (wy-y)*AHDWS;
					pVbPixel		= pVBasebPixel + (wy-y)*AHDWS;
					pHHomoPixel		= pHBaseHomoPixel + (wy-y)*AHDWS;
					pVHomoPixel		= pVBaseHomoPixel + (wy-y)*AHDWS;

					for (wx = x;wx<lWidth && wx <x+AHDWS;wx++)
					{
						LONG				i;

						*pHHomoPixel = *pVHomoPixel = 0;

						// For each pixel in the V and H approximations
						// iterate over its neighbors
						for(i=0; i<4; i++)
						{
							lDiffH[i] = fabs((*pHLPixel) - (*(pHLPixel+dir[i])));
							lDiffV[i] = fabs((*pVLPixel) - (*(pVLPixel+dir[i])));

							double	aDiff = ((*pHaPixel)-(*(pHaPixel+dir[i])));
							double	bDiff = ((*pHbPixel)-(*(pHbPixel+dir[i])));
							abDiffH[i] = aDiff*aDiff + bDiff*bDiff;

							aDiff = ((*pVaPixel)-(*(pVaPixel+dir[i])));
							bDiff = ((*pVbPixel)-(*(pVbPixel+dir[i])));
							abDiffV[i] = aDiff*aDiff + bDiff*bDiff;
						}

						lEpsilon = min(max(lDiffH[0], lDiffH[1]), max(lDiffV[2], lDiffV[3]));
						abEpsilon = min(max(abDiffH[0], abDiffH[1]), max(abDiffV[2], abDiffV[3]));

						// iterate over neighbors
						for(i=0; i<4; ++i)
						{
							if ((lDiffH[i] <= lEpsilon) && (abDiffH[i] <= abEpsilon))
								(*pHHomoPixel)++;
							if ((lDiffV[i] <= lEpsilon) && (abDiffV[i] <= abEpsilon))
								(*pVHomoPixel)++;;
						}

						pHHomoPixel++;		pVHomoPixel++;
						pHLPixel++;			pHaPixel++;		pHbPixel++;
						pVLPixel++;			pVaPixel++;		pVbPixel++;
					};
				};

				// Combine the most homogenous pixels for the final result
				for (wy = y+1;wy<lHeight-1 && wy<y+AHDWS-1;wy++)
				{
					TType *					pHGreenPixel;
					TType *					pVGreenPixel;
					TType *					pHRedPixel;
					TType *					pVRedPixel;
					TType *					pHBluePixel;
					TType *					pVBluePixel;
					BYTE *					pHHomoPixel;
					BYTE *					pVHomoPixel;
					TType *					pOutputRedPixel;
					TType *					pOutputGreenPixel;
					TType *					pOutputBluePixel;

					pOutputRedPixel		= pBaseOutputRedPixel + (wy-y) * lWidth + 1 ;
					pOutputGreenPixel	= pBaseOutputGreenPixel + (wy-y) * lWidth + 1;
					pOutputBluePixel	= pBaseOutputBluePixel + (wy-y) * lWidth + 1;

					pHGreenPixel	= pHBaseGreenPixel + (wy-y)*AHDWS+1;
					pVGreenPixel	= pVBaseGreenPixel + (wy-y)*AHDWS+1;
					pHRedPixel		= pHBaseRedPixel + (wy-y)*AHDWS+1;
					pVRedPixel		= pVBaseRedPixel + (wy-y)*AHDWS+1;
					pHBluePixel		= pHBaseBluePixel + (wy-y)*AHDWS+1;
					pVBluePixel		= pVBaseBluePixel + (wy-y)*AHDWS+1;
					pHHomoPixel		= pHBaseHomoPixel + (wy-y)*AHDWS+1;
					pVHomoPixel		= pVBaseHomoPixel + (wy-y)*AHDWS+1;

					for (wx = x+1;wx<lWidth-1 && wx <x+AHDWS-1;wx++)
					{
						LONG			hmV,
										hmH;

						hmV = *(pVHomoPixel) + (*(pVHomoPixel-1)) + (*(pVHomoPixel+1)) +
							  (*(pVHomoPixel-AHDWS)) + (*(pVHomoPixel-AHDWS-1)) + (*(pVHomoPixel-AHDWS+1)) +
							  (*(pVHomoPixel+AHDWS)) + (*(pVHomoPixel+AHDWS-1)) + (*(pVHomoPixel+AHDWS+1));

						hmH = *(pHHomoPixel) + (*(pHHomoPixel-1)) + (*(pHHomoPixel+1)) +
							  (*(pHHomoPixel-AHDWS)) + (*(pHHomoPixel-AHDWS-1)) + (*(pHHomoPixel-AHDWS+1)) +
							  (*(pHHomoPixel+AHDWS)) + (*(pHHomoPixel+AHDWS-1)) + (*(pHHomoPixel+AHDWS+1));

						if (hmV > hmH)
						{
							*pOutputRedPixel	= *pVRedPixel;
							*pOutputGreenPixel	= *pVGreenPixel;
							*pOutputBluePixel	= *pVBluePixel;
						}
						else if (hmV < hmH)
						{
							*pOutputRedPixel	= *pHRedPixel;
							*pOutputGreenPixel	= *pHGreenPixel;
							*pOutputBluePixel	= *pHBluePixel;
						}
						else
						{
							*pOutputRedPixel	= ((*pVRedPixel)	+ (*pHRedPixel))/2.0;
							*pOutputGreenPixel	= ((*pVGreenPixel)	+ (*pHGreenPixel))/2.0;
							*pOutputBluePixel	= ((*pVBluePixel)	+ (*pHBluePixel))/2.0;
						}

						pHHomoPixel++;		pVHomoPixel++;
						pHRedPixel++;		pHGreenPixel++;			pHBluePixel++;;
						pVRedPixel++;		pVGreenPixel++;			pVBluePixel++;;
						pOutputRedPixel++;	pOutputGreenPixel++;	pOutputBluePixel++;
					};
				};

				lNrWindows++;
				if (pProgress)
					pProgress->Progress2(nullptr, lNrWindows);
			};
		};

		// Interpolate borders
		{
			TType *							pGrayPixel1;
			TType *							pGrayPixel2;
			TType *							pOutputRedPixel1;
			TType *							pOutputRedPixel2;
			TType *							pOutputGreenPixel1;
			TType *							pOutputGreenPixel2;
			TType *							pOutputBluePixel1;
			TType *							pOutputBluePixel2;
			LONG							x1 = 0,
											x2 = lWidth-1;
			BOOL							bBlueLine;

			pGrayPixel1			= pGrayBitmap->GetGrayPixel(0, 1);
			pGrayPixel2			= pGrayBitmap->GetGrayPixel(lWidth-1, 1);
			pOutputRedPixel1	= pColorBitmap->GetRedPixel(0, 1);
			pOutputGreenPixel1	= pColorBitmap->GetGreenPixel(0, 1);
			pOutputBluePixel1	= pColorBitmap->GetBluePixel(0, 1);
			pOutputRedPixel2	= pColorBitmap->GetRedPixel(lWidth-1, 1);
			pOutputGreenPixel2	= pColorBitmap->GetGreenPixel(lWidth-1, 1);
			pOutputBluePixel2	= pColorBitmap->GetBluePixel(lWidth-1, 1);
			// Vertical interpolation
			bBlueLine = IsBayerBlueLine(1, pGrayBitmap->GetCFAType(), pGrayBitmap->yOffset());
			for (y = 1;y<lHeight-1;y++)
			{
				BAYERCOLOR				BayerColor;
				TType					v1, v2;

				BayerColor = pGrayBitmap->GetBayerColor(x1, y);
				v1 = *(pGrayPixel1-lWidth);
				v2 = *(pGrayPixel1+lWidth);
				switch (BayerColor)
				{
				case BAYER_GREEN :
					*pOutputGreenPixel1 = *pGrayPixel1;
					if (bBlueLine)
					{
						*pOutputBluePixel1 = *(pGrayPixel1+1);
						*pOutputRedPixel1 = (v1+v2)/2.0;
					}
					else
					{
						*pOutputRedPixel1 = *(pGrayPixel1+1);
						*pOutputBluePixel1 = (v1+v2)/2.0;
					};
					break;
				case BAYER_RED :
					*pOutputRedPixel1 = *pGrayPixel1;
					*pOutputGreenPixel1 = (v1+v2)/2.0;
					*pOutputBluePixel1  = *(pOutputBluePixel1+1);
					break;
				case BAYER_BLUE :
					*pOutputBluePixel1 = *pGrayPixel1;
					*pOutputGreenPixel1 = (v1+v2)/2.0;
					*pOutputRedPixel1   = *(pOutputRedPixel1+1);
					break;
				};

				BayerColor = pGrayBitmap->GetBayerColor(x2, y);
				v1 = *(pGrayPixel2-lWidth);
				v2 = *(pGrayPixel2+lWidth);
				switch (BayerColor)
				{
				case BAYER_GREEN :
					*pOutputGreenPixel2 = *pGrayPixel2;
					if (bBlueLine)
					{
						*pOutputBluePixel2 = *(pGrayPixel2-1);
						*pOutputRedPixel2 = (v1+v2)/2.0;
					}
					else
					{
						*pOutputRedPixel2 = *(pGrayPixel2-1);
						*pOutputBluePixel2 = (v1+v2)/2.0;
					};
					break;
				case BAYER_RED :
					*pOutputRedPixel2 = *pGrayPixel2;
					*pOutputGreenPixel2 = (v1+v2)/2.0;
					*pOutputBluePixel2  = *(pOutputBluePixel2-1);
					break;
				case BAYER_BLUE :
					*pOutputBluePixel2 = *pGrayPixel2;
					*pOutputGreenPixel2 = (v1+v2)/2.0;
					*pOutputRedPixel2   = *(pOutputRedPixel2-1);
					break;
				};

				pOutputRedPixel1+=lWidth;		pOutputGreenPixel1+=lWidth;	pOutputBluePixel1+=lWidth;
				pOutputRedPixel2+=lWidth;		pOutputGreenPixel2+=lWidth;	pOutputBluePixel2+=lWidth;
				pGrayPixel1+=lWidth;			pGrayPixel2+=lWidth;

				bBlueLine = !bBlueLine;
			};

			LONG								y1 = 0,
												y2 = lHeight-1;
			BOOL								bBlueLine1,
												bBlueLine2;

			pGrayPixel1			= pGrayBitmap->GetGrayPixel(1, 0);
			pGrayPixel2			= pGrayBitmap->GetGrayPixel(1, lHeight-1);
			pOutputRedPixel1	= pColorBitmap->GetRedPixel(1, 0);
			pOutputGreenPixel1	= pColorBitmap->GetGreenPixel(1, 0);
			pOutputBluePixel1	= pColorBitmap->GetBluePixel(1, 0);
			pOutputRedPixel2	= pColorBitmap->GetRedPixel(1, lHeight-1);
			pOutputGreenPixel2	= pColorBitmap->GetGreenPixel(1, lHeight-1);
			pOutputBluePixel2	= pColorBitmap->GetBluePixel(1, lHeight-1);
			// Vertical interpolation
			bBlueLine1 = IsBayerBlueLine(0, pGrayBitmap->GetCFAType(), pGrayBitmap->yOffset());
			bBlueLine2 = IsBayerBlueLine(lHeight-1, pGrayBitmap->GetCFAType(), pGrayBitmap->yOffset());
			for (x = 1;x<lWidth-1;x++)
			{
				BAYERCOLOR				BayerColor;
				TType					v1, v2;

				BayerColor = pGrayBitmap->GetBayerColor(x, y1);
				v1 = *(pGrayPixel1-1);
				v2 = *(pGrayPixel1+1);
				switch (BayerColor)
				{
				case BAYER_GREEN :
					*pOutputGreenPixel1 = *pGrayPixel1;
					if (bBlueLine1)
					{
						*pOutputBluePixel1 = (v1+v2)/2.0;
						*pOutputRedPixel1 = *(pGrayPixel1+lWidth);
					}
					else
					{
						*pOutputBluePixel1 = *(pGrayPixel1+lWidth);
						*pOutputRedPixel1 = (v1+v2)/2.0;
					};
					break;
				case BAYER_RED :
					*pOutputRedPixel1 = *pGrayPixel1;
					*pOutputGreenPixel1 = (v1+v2)/2.0;
					*pOutputBluePixel1  = *(pOutputBluePixel1+lWidth);
					break;
				case BAYER_BLUE :
					*pOutputBluePixel1 = *pGrayPixel1;
					*pOutputGreenPixel1 = (v1+v2)/2.0;
					*pOutputRedPixel1   = *(pOutputRedPixel1+lWidth);
					break;
				};

				BayerColor = pGrayBitmap->GetBayerColor(x, y2);
				v1 = *(pGrayPixel2-1);
				v2 = *(pGrayPixel2+1);
				switch (BayerColor)
				{
				case BAYER_GREEN :
					*pOutputGreenPixel2 = *pGrayPixel2;
					if (bBlueLine2)
					{
						*pOutputBluePixel2 = (v1+v2)/2.0;
						*pOutputRedPixel2 = *(pGrayPixel2-lWidth);
					}
					else
					{
						*pOutputRedPixel2 = (v1+v2)/2.0;
						*pOutputBluePixel2 = *(pGrayPixel2-lWidth);
					};
					break;
				case BAYER_RED :
					*pOutputRedPixel2 = *pGrayPixel2;
					*pOutputGreenPixel2 = (v1+v2)/2.0;
					*pOutputBluePixel2  = *(pOutputBluePixel2-lWidth);
					break;
				case BAYER_BLUE :
					*pOutputBluePixel2 = *pGrayPixel2;
					*pOutputGreenPixel2 = (v1+v2)/2.0;
					*pOutputRedPixel2   = *(pOutputRedPixel2-lWidth);
					break;
				};

				pOutputRedPixel1++;		pOutputGreenPixel1++;	pOutputBluePixel1++;
				pOutputRedPixel2++;		pOutputGreenPixel2++;	pOutputBluePixel2++;
				pGrayPixel1++;			pGrayPixel2++;
			};

			// Interpolate the 4 corners
			pOutputRedPixel1	= pColorBitmap->GetRedPixel(0, 0);
			pOutputGreenPixel1	= pColorBitmap->GetGreenPixel(0, 0);
			pOutputBluePixel1	= pColorBitmap->GetBluePixel(0, 0);

			*pOutputRedPixel1   = ((*(pOutputRedPixel1+1)) + (*(pOutputRedPixel1+lWidth)) + (*(pOutputRedPixel1+1+lWidth)))/3.0;
			*pOutputGreenPixel1 = ((*(pOutputGreenPixel1+1)) + (*(pOutputGreenPixel1+lWidth)) + (*(pOutputGreenPixel1+1+lWidth)))/3.0;
			*pOutputBluePixel1  = ((*(pOutputBluePixel1+1)) + (*(pOutputBluePixel1+lWidth)) + (*(pOutputBluePixel1+1+lWidth)))/3.0;

			pOutputRedPixel1	= pColorBitmap->GetRedPixel(0, lHeight-1);
			pOutputGreenPixel1	= pColorBitmap->GetGreenPixel(0, lHeight-1);
			pOutputBluePixel1	= pColorBitmap->GetBluePixel(0, lHeight-1);

			*pOutputRedPixel1   = ((*(pOutputRedPixel1+1)) + (*(pOutputRedPixel1-lWidth)) + (*(pOutputRedPixel1+1-lWidth)))/3.0;
			*pOutputGreenPixel1   = ((*(pOutputGreenPixel1+1)) + (*(pOutputGreenPixel1-lWidth)) + (*(pOutputGreenPixel1+1-lWidth)))/3.0;
			*pOutputBluePixel1   = ((*(pOutputBluePixel1+1)) + (*(pOutputBluePixel1-lWidth)) + (*(pOutputBluePixel1+1-lWidth)))/3.0;

			pOutputRedPixel1	= pColorBitmap->GetRedPixel(lWidth-1, lHeight-1);
			pOutputGreenPixel1	= pColorBitmap->GetGreenPixel(lWidth-1, lHeight-1);
			pOutputBluePixel1	= pColorBitmap->GetBluePixel(lWidth-1, lHeight-1);

			*pOutputRedPixel1   = ((*(pOutputRedPixel1-1)) + (*(pOutputRedPixel1-lWidth)) + (*(pOutputRedPixel1-1-lWidth)))/3.0;
			*pOutputGreenPixel1   = ((*(pOutputGreenPixel1-1)) + (*(pOutputGreenPixel1-lWidth)) + (*(pOutputGreenPixel1-1-lWidth)))/3.0;
			*pOutputBluePixel1   = ((*(pOutputBluePixel1-1)) + (*(pOutputBluePixel1-lWidth)) + (*(pOutputBluePixel1-1-lWidth)))/3.0;

			pOutputRedPixel1	= pColorBitmap->GetRedPixel(lWidth-1, 0);
			pOutputGreenPixel1	= pColorBitmap->GetGreenPixel(lWidth-1, 0);
			pOutputBluePixel1	= pColorBitmap->GetBluePixel(lWidth-1, 0);

			*pOutputRedPixel1   = ((*(pOutputRedPixel1-1)) + (*(pOutputRedPixel1+lWidth)) + (*(pOutputRedPixel1-1+lWidth)))/3.0;
			*pOutputGreenPixel1   = ((*(pOutputGreenPixel1-1)) + (*(pOutputGreenPixel1+lWidth)) + (*(pOutputGreenPixel1-1+lWidth)))/3.0;
			*pOutputBluePixel1   = ((*(pOutputBluePixel1-1)) + (*(pOutputBluePixel1+lWidth)) + (*(pOutputBluePixel1-1+lWidth)))/3.0;
		};

		if (pProgress)
			pProgress->End2();

		bResult = TRUE;
	};

	if (bResult)
	{
		pOutputBitmap.Attach(pColorBitmap);
		pOutputBitmap.CopyTo(ppColorBitmap);
	};

	return bResult;
};

#endif __AHDDEMOSAICING_H__
