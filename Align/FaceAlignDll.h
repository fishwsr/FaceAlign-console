#ifdef FACEALIGNDLL_EXPORTS
#define FACEALIGNDLL_API __declspec(dllexport)
#else
#define FACEALIGNDLL_API __declspec(dllimport)
#endif

// This class is exported from the FaceAlignDll.dll
class FACEALIGNDLL_API CFaceAlignDll {
public:
	CFaceAlignDll(void);
	~CFaceAlignDll(void);
	// TODO: add your methods here.

public:
	HRESULT Init(const WCHAR* wzModelFile);

	//align by inputting rect of face detection
	HRESULT Align(BYTE* pImgData, int iImgW, int iImgH, int iStride, const RECT& rcFace, float* pPoints);

	int FacePtNum();

	void AvgShape(float* pAvgPoints);

protected:
	void * m_pFaceAlign;		 
};
