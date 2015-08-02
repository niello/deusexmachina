//class CRenderServer: public Core::CObject
//{
//protected:
//
//	CDict<CStrID, PFrameShader>			FrameShaders;
//	CStrID								ScreenFrameShaderID;
//
//	//???can write better?
//	PShader								SharedShader;
//	CShader::HVar						hLightAmbient;
//	CShader::HVar						hEyePos;
//	CShader::HVar						hViewProj;
//	vector3								CurrCameraPos;
//	matrix44							CurrViewProj; //???or store camera ref?
//
//public:
//
//	Data::CDynamicEnum32					ShaderFeatures;
//
//	void				SetAmbientLight(const vector4& Color);
//	void				SetCameraPosition(const vector3& Pos);
//	void				SetViewProjection(const matrix44& VP);
//
//	void				AddFrameShader(CStrID ID, PFrameShader FrameShader); //???or always load internally?
//	void				SetScreenFrameShaderID(CStrID ID) { ScreenFrameShaderID = ID; }
//	CFrameShader*		GetScreenFrameShader() const;
//
//	DWORD				GetFeatureFlagSkinned() const { return FFlagSkinned; }
//	DWORD				GetFeatureFlagInstanced() const { return FFlagInstanced; }
//
//	const vector3&		GetCameraPosition() const { return CurrCameraPos; }
//	const matrix44&		GetViewProjection() const { return CurrViewProj; }
//};
//
//inline CRenderServer::CRenderServer():
//	InstanceCount(0),
//	hLightAmbient(NULL),
//	hEyePos(NULL),
//	hViewProj(NULL),
//	FrameID(0)
//{
//	memset(CurrVBOffset, 0, sizeof(CurrVBOffset));
//}
////---------------------------------------------------------------------
//
//inline void CRenderServer::SetAmbientLight(const vector4& Color)
//{
//	if (hLightAmbient) SharedShader->SetFloat4(hLightAmbient, Color);
//}
////---------------------------------------------------------------------
//
//inline void CRenderServer::SetCameraPosition(const vector3& Pos)
//{
//	CurrCameraPos = Pos;
//	if (hEyePos) SharedShader->SetFloat4(hEyePos, vector4(Pos));
//}
////---------------------------------------------------------------------
//
//inline void CRenderServer::SetViewProjection(const matrix44& VP)
//{
//	CurrViewProj = VP;
//	if (hViewProj) SharedShader->SetMatrix(hViewProj, CurrViewProj);
//}
////---------------------------------------------------------------------
//
//inline void CRenderServer::AddFrameShader(CStrID ID, PFrameShader FrameShader)
//{
//	n_assert(ID.IsValid() && FrameShader.IsValid());
//	FrameShaders.Add(ID, FrameShader);
//}
////---------------------------------------------------------------------
//
//inline CFrameShader* CRenderServer::GetScreenFrameShader() const
//{
//	int Idx = FrameShaders.FindIndex(ScreenFrameShaderID);
//	return (Idx != INVALID_INDEX) ? FrameShaders.ValueAt(Idx).GetUnsafe() : NULL;
//}
////---------------------------------------------------------------------
