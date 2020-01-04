Name:
ModifiedCubeMapGen.exe
Note that exe was renamed to mcmg.exe for shortness.

URLs:
https://code.google.com/archive/p/cubemapgen/downloads
https://seblagarde.wordpress.com/2012/06/10/amd-cubemapgen-for-physically-based-rendering/

License:
see LICENSE.md

Current version used:
1.66

Purpose in DEM:
Irradiance environment map (IEM) and prefiltered mipmaped radiance environment map (PMREM) calculation for image-based lighting.

Command line example for IEM:
mcmg.exe -IrradianceCubemap -importCubeDDS:"input_cubemap.dds" -exportCubeDDS -exportFilename:"output_cubemap_iem.dds" -exportSize:128 -exportPixelFormat:A8R8G8B8 -consoleErrorOutput -exit

Command line example for PMREM (Mipmap mode is used in DEM):
mcmg.exe -filterTech:CosinePower -CosinePowerMipmapChainMode:Mipmap -NumMipmap:7 -GlossScale:10 -GlossBias:1 -LightingModel:PhongBRDF -ExcludeBase -solidAngleWeighting -edgeFixupTech:Warp -importDegamma:1.0 -exportGamma:1.0 -importCubeDDS:"input_cubemap.dds" -exportCubeDDS -exportFilename:"output_cubemap_pmrem.dds" -exportSize:256 -exportPixelFormat:A8R8G8B8 -exportMipChain -consoleErrorOutput -exit

Use mcmg.exe --help to explore all available options and their meaning.
