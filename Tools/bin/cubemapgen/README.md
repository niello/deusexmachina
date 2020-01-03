Name:
ModifiedCubeMapGen.exe
Note that exe was renamed for shortness.

URLs:
https://code.google.com/archive/p/cubemapgen/downloads
https://seblagarde.wordpress.com/2012/06/10/amd-cubemapgen-for-physically-based-rendering/

License:
see LICENSE.md

Current version used:
1.66

Purpose in DEM:
Irradiance environment map (IEM) and prefiltered mipmaped radiance environment map (PMREM) calculation for image-based lighting.

Command line example:
mcmg.exe -consoleErrorOutput -importDegamma:1.0 -exportGamma:1.0 -solidAngleWeighting -edgeFixupTech:Warp -filterTech:CosinePower -NumMipmap:%d -CosinePowerMipmapChainMode:Mipmap -GlossScale:10 -GlossBias:1 %s -LightingModel:PhongBRDF -importCubeDDS:%s -exportCubeDDS -exportMipChain -exportFilename:%s -exportPixelFormat:A16B16G16R16F -exit

Or use mcmg.exe -help
