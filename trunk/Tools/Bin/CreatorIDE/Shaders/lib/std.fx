#define DIRLIGHTS_ENABLEOPPOSITECOLOR 1
#define DIRLIGHTS_OPPOSITECOLOR float3(0.45f, 0.52f, 0.608f)
#define DEBUG_LIGHTCOMPLEXITY 0

#define DECLARE_SCREENPOS(X) float4 screenPos : X;
#define PS_SCREENPOS psComputeScreenCoord(psIn.screenPos)
#define VS_SETSCREENPOS(X) vsOut.screenPos = vsComputeScreenCoord(X);

typedef half3 color3;
typedef half4 color4;