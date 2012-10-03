//-------------------------------------------------------------------------------
//  shared.fx
//
//  A pseudo-shader which lists all shared effect parameters.
//
//  (C) 2004 RadonLabs GmbH
//-------------------------------------------------------------------------------

shared float4x4 Model;                  // the model matrix
shared float4x4 View;                   // the view matrix
shared float4x4 Projection;             // the projection matrix
shared float4x4 ModelView;              // the model * view matrix
shared float4x4 ModelViewProjection;    // the model * view * projection matrix
shared float4x4 InvModel;               // the inverse model matrix
shared float4x4 InvView;                // the inverse view matrix
shared float4x4 InvModelView;           // the inverse model * view matrix

shared float3   LightPos;               // light position in world space
shared int      LightType;              // light type
shared float    LightRange;             // light range
shared float4   LightDiffuse;           // light diffuse component
shared float4   LightSpecular;          // light specular component
shared float4   LightAmbient;           // light ambient component
shared float4   ShadowIndex;            // shadow light index

shared float3   ModelEyePos;            // the eye pos in model space
shared float3   EyePos;                 // the eye pos in world space
shared float3   EyeDir;                 // the eye dir in world space
shared float4x4 TextureTransform0;      // texture transform for uv set 0
shared float4x4 TextureTransform1;      // texture transform for uv set 1
shared float4x4 TextureTransform2;      // texture transform for uv set 0
shared float4x4 TextureTransform3;      // texture transform for uv set 0
shared float Time;                      // the current global time
shared float4 DisplayResolution;        // the current display resolution
shared float4 HalfPixelSize;            // half size of a display pixel
shared float4 RenderTargetOffset;		// see nCaptureServer

technique t0
{
    pass p0
    {

    }
}
