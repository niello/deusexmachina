/*
    Based on nmeshtool (C) 2003 RadonLabs GmbH
*/
#include "kernel/nkernelserver.h"
#include <Data/DataServer.h>
#include <Data/Streams/FileStream.h>
#include <Data/TextReader.h>
#include "ncmdlineargs.h"
#include "nmeshbuilder.h"
#include "util/ndictionary.h"
//#include "scene/nshapenode.h"

#undef CreateDirectory
#undef CopyFile

bool LoadObj(nMeshBuilder* mesh, const char* filename);

nArray<nString>					MtlFiles;
nDictionary<int, nString>		GroupToName;
nDictionary<int, nString>		GroupToMtl;
nDictionary<nString, nString>	TexMap;
nString							inFileArg;
bool							InvertVArg;

nString ProcessTexture(const nString& TexFile, const nString& RsrcName, int MaterialIdx, const char* Postfix, const char* Default)
{
	nString InTex;
	InTex = inFileArg.ExtractDirName() + TexFile.ExtractFileName();

	if (TexMap.Contains(InTex)) return TexMap[InTex];
	else
	{
		nString OutTex;

		const int EXT_COUNT = 6;
		static const char* Exts[EXT_COUNT] = { ".dds", ".tga", ".png", ".bmp", ".jpg", ".jpeg" };
		int CurrExt = 0;

		bool FileIsFound = true;
		while (!DataSrv->FileExists(InTex))
		{
			if (CurrExt < EXT_COUNT)
			{
				n_printf("Texture '%s' not found, trying extension '%s'...\n", InTex.Get(), Exts[CurrExt]);
				InTex.StripExtension();
				InTex += Exts[CurrExt++];
			}
			else
			{
				FileIsFound = false;
				break;
			}
		}

		if (FileIsFound)
		{
			OutTex.Format("textures:%s_%d_%s.%s", RsrcName.Get(), MaterialIdx, Postfix, InTex.GetExtension());
			DataSrv->CreateDirectory(OutTex.ExtractDirName());
			DataSrv->CopyFile(InTex, OutTex);
		}
		else
		{
			n_printf("WARNING: no texture '%s' found to be used as '%s', '%s' is set\n",
				InTex.Get(), OutTex.Get(), Default);
			OutTex = "textures:";
			OutTex += Default;
		}

		TexMap.Add(InTex, OutTex);
		return OutTex;
	}
}
//---------------------------------------------------------------------

int main(int argc, const char** argv)
{
	// Debug cmd line
	// -clean -normal -tangentsplit -scale 0.005 -in "..\..\..\InsanePoet\Content\Src\Models\vgt/apple/apple.obj" -out vgt/apple -proj "..\..\..\InsanePoet\Content\"

    nCmdLineArgs args(argc, argv);
    nMeshBuilder mesh;

    // get cmd line args
    bool helpArg               = args.GetBoolArg("-help");
			inFileArg          = args.GetStringArg("-in");
    nString OutResourceName    = args.GetStringArg("-out");
    nString ProjDir			   = args.GetStringArg("-proj");
    bool cleanArg              = args.GetBoolArg("-clean");
    bool CreateNormals		   = args.GetBoolArg("-normal");
	bool CreateSceneFileArg    = args.GetBoolArg("-scnfile");
    bool tangentNoSplitArg     = args.GetBoolArg("-tangent");
    bool tangentSplitArg       = args.GetBoolArg("-tangentsplit");
    bool edgeArg               = args.GetBoolArg("-edge");
		 InvertVArg            = args.GetBoolArg("-invertv");
	bool ShadowArg             = args.GetBoolArg("-shadow");
    //nString groupArg           = args.GetStringArg("-group");
    float txArg                = args.GetFloatArg("-tx");
    float tyArg                = args.GetFloatArg("-ty");
    float tzArg                = args.GetFloatArg("-tz");
    float rxArg                = args.GetFloatArg("-rx");
    float ryArg                = args.GetFloatArg("-ry");
    float rzArg                = args.GetFloatArg("-rz");
    float sxArg                = args.GetFloatArg("-sx", 1.0f);
    float syArg                = args.GetFloatArg("-sy", 1.0f);
    float szArg                = args.GetFloatArg("-sz", 1.0f);
    float scaleArg             = args.GetFloatArg("-scale");
    nString delComponentArg    = args.GetStringArg("-delcomponent");

    // show help?
    if (helpArg)
    {
        printf( "ConvMdl - DeusExMachina model conversion tool\n"
                "Based on nmeshtool - Nebula2 mesh File tool (C) 2003 RadonLabs GmbH\n\n"
                 "Command line args:\n"
                 "------------------\n"
                 "-help                 show this help\n"
                 "-in [filename]        input mesh File (.obj, .n3d, .n3d2, .nvx or .nvx2 extension)\n"
                 "-out [filename]       output mesh File (.n3d2 or .nvx2 extension)\n"
                 "-proj [dirname]       project directory\n"
                 "-scnfile              create .scn resource file for this mesh\n"
                 "-clean                clean up mesh (removes redundant vertices)\n"
                 "-normal               generate vertex normals if not present in src\n"
                 "-tangent              generate vertex tangents for per pixel lighting using\n"
                 "                      a technique that will not split vertices\n"
                 "-tangentsplit         generate vertex tangents for per pixel lighting using\n"
                 "                      a technique that may split vertices\n"
                 "-edge                 generate edge data\n"
				 "-invertv              invert V texture coord (Blender convention)\n"
                 //"-group [groupname]    select a group inside the mesh\n"
                 "-tx [float]           translate vertices along x\n"
                 "-ty [float]           translate vertices along y\n"
                 "-tz [float]           translate vertices along z\n"
                 "-rx [float]           rotate in degree vertices around x\n"
                 "-ry [float]           rotate in degree vertices around y\n"
                 "-rz [float]           rotate in degree vertices around z\n"
                 "-sx [float]           scale vertices along x\n"
                 "-sy [float]           scale vertices along y\n"
                 "-sz [float]           scale vertices along z\n"
                 "-scale [float]        uniformly scale vertices\n"
                 "-delcomponent [c]     delete a vertex component from all vertices\n");
        return 5;
    }

    // error if no input File given
    if (inFileArg.IsEmpty() || OutResourceName.IsEmpty() || ProjDir.IsEmpty())
    {
        printf("ConvMdl error: No input File, output resource or project directory! (type 'nmeshtool -help' for help)\n");
        return 5;
    }

    // startup Nebula
    nKernelServer* kernelServer = new nKernelServer;

	Ptr<Data::CDataServer> DataServer;
	DataServer.Create();
	DataSrv->SetAssign("proj", ProjDir);
	DataSrv->SetAssign("export", "proj:export");
	DataSrv->SetAssign("textures", "export:textures");
	DataSrv->SetAssign("meshes", "export:meshes");

	MtlFiles.Clear();
	GroupToName.Clear();
	GroupToMtl.Clear();
	TexMap.Clear();

    // read input mesh
    n_printf("-> loading mesh '%s'\n", inFileArg.Get());
	if (inFileArg.CheckExtension("obj"))
	{
		if (!LoadObj(&mesh, inFileArg.Get()))
		{
			n_printf("ConvMdl error: Could not load '%s'\n", inFileArg.Get());
			DataServer = NULL;
			delete kernelServer;
			return 5;
		}
	}
	else if (!mesh.Load(inFileArg.Get()))
    {
        n_printf("ConvMdl error: Could not load '%s'\n", inFileArg.Get());
		DataServer = NULL;
        delete kernelServer;
        return 5;
    }

    // cleanup?
    if (cleanArg)
    {
        n_printf("-> cleaning...\n");
        mesh.Cleanup(0);
    }

    // decide if we should transform at all
    vector3 translate(txArg, tyArg, tzArg);
    vector3 rotate(rxArg, ryArg, rzArg);
    vector3 scale(sxArg, syArg, szArg);
    if (scaleArg != 0.0f) scale.set(scaleArg, scaleArg, scaleArg);

    bool doTransform = (translate.lensquared() > 0.0f) || (rotate.lensquared() > 0.0f) ||
        !(scale.x == 1.0f && scale.y == 1.0f && scale.z == 1.0f);

    // transform?
    if (doTransform)
    {
        n_printf("-> transforming...\n");
        matrix44 m;
        if (scale.len() != 1.0f) m.scale(scale);
        if (rotate.len() > 0.0f)
        {
            m.rotate_x(n_deg2rad(rotate.x));
            m.rotate_y(n_deg2rad(rotate.y));
            m.rotate_z(n_deg2rad(rotate.z));
        }
        if (translate.len() > 0.0f) m.translate(translate);
        mesh.Transform(m);
    }

	if (CreateNormals && !mesh.HasVertexComponent(nMeshBuilder::Vertex::NORMAL))
	{
        n_printf("-> generating normals...\n");
		mesh.BuildVertexNormals();
	}

    // generate tangents?
    if (tangentNoSplitArg || tangentSplitArg)
    {
        n_printf("-> generating tangents...\n");
        mesh.BuildTriangleNormals();
        mesh.BuildVertexTangents(tangentSplitArg);
    }

    // generate edges?
    if (edgeArg)
    {
        n_printf("-> generating edges...\n");
        mesh.CreateEdges();
    }

    if (delComponentArg.IsValid())
    {
        nMeshBuilder::Vertex::Component c = nMeshBuilder::Vertex::NUM_VERTEX_COMPONENTS;
        if (delComponentArg == "coord") c = nMeshBuilder::Vertex::COORD;
        else if (delComponentArg == "normal") c = nMeshBuilder::Vertex::NORMAL;
        else if (delComponentArg == "uv0") c = nMeshBuilder::Vertex::UV0;
        else if (delComponentArg == "uv1") c = nMeshBuilder::Vertex::UV1;
        else if (delComponentArg == "uv2") c = nMeshBuilder::Vertex::UV2;
        else if (delComponentArg == "uv3") c = nMeshBuilder::Vertex::UV3;
        else if (delComponentArg == "color") c = nMeshBuilder::Vertex::COLOR;
        else if (delComponentArg == "tangent") c = nMeshBuilder::Vertex::TANGENT;
        else if (delComponentArg == "binormal") c = nMeshBuilder::Vertex::BINORMAL;
        else if (delComponentArg == "weights") c = nMeshBuilder::Vertex::WEIGHTS;
        else if (delComponentArg == "jindices") c = nMeshBuilder::Vertex::JINDICES;
        if (c != nMeshBuilder::Vertex::NUM_VERTEX_COMPONENTS)
        {
            n_printf("-> deleting vertex component %s...\n", delComponentArg.Get());
            mesh.DelVertexComponent(c);
        }
    }

	if (InvertVArg)
	{
		n_printf("-> inverting V texcoord...\n");
		bool HasTex = false;
		for (int Layer = 0; Layer < nMeshBuilder::Vertex::MAX_TEXTURE_LAYERS; Layer++)
			if (mesh.HasVertexComponent(nMeshBuilder::Vertex::Component(nMeshBuilder::Vertex::UV0 + Layer)))
			{
				HasTex = true;
				for (int i = 0; i < mesh.vertexArray.Size(); i++)
					mesh.vertexArray[i].uv[Layer].y = 1.f - mesh.vertexArray[i].uv[Layer].y;
			}
		if (!HasTex) n_printf(" * no texture coordinates found in the mesh\n");
	}

	//nShapeNode* n = n_new(nShapeNode);

	nDictionary<nString, nString> Materials;

	for (int i = 0; i < MtlFiles.Size(); i++)
	{
		nString MtlFName = inFileArg.ExtractDirName() + MtlFiles[i];
		Data::CFileStream File;
	    
		if (File.Open(MtlFName, Data::SAM_READ))
		{
			Data::CTextReader Reader(File);

			nString CurrMtl, CurrMtlName, Transparency = "1.000", Shader = "static";
			bool HasBump = false;
			bool BlenderWarning = false;
			
			char line[1024];
			while (Reader.ReadLine(line, sizeof(line)))
			{
				// get keyword
				char* keyWord = strtok(line, N_WHITESPACE);
				if (!keyWord) continue;

				if (!strcmp(keyWord, "#"))
				{
					if (!BlenderWarning && !InvertVArg)
					{
						nString RestOfLine = strtok(0, "\r\n");
						RestOfLine.ToLower();
						if (RestOfLine.FindStringIndex("blender", 0) > -1)
						{
							n_printf("WARNING: 'Blender' found in comments, but -invertv isn't specified. Textures may appear flipped upside-down!\n");
							BlenderWarning = true;
						}
					}
				}
				else if (!strcmp(keyWord, "newmtl"))
				{
					if (CurrMtlName.IsValid() && CurrMtl.IsValid())
					{
						if (!HasBump)
							CurrMtl += "\t.settexture \"BumpMap0\" \"textures:system/nobump.dds\"\n";
						CurrMtl += "\t.setshader \"";
						CurrMtl += Shader;
						CurrMtl += "\"";
						CurrMtl.ConvertBackslashes();
						Materials.Add(CurrMtlName, CurrMtl);
					}
					
					HasBump = false;
					Transparency = "1.000";
					Shader = "static";
					
					const char* MtlName = strtok(0, N_WHITESPACE);
					if (MtlName)
					{
						CurrMtlName = MtlName;
						CurrMtl = "";
					}
				}
				else if (!strcmp(keyWord, "Tr") || !strcmp(keyWord, "d"))
				{
					const char* Factor = strtok(0, N_WHITESPACE);
					Transparency = Factor;
	                if ((float)atof(Factor) < 1.f) Shader = "alpha";
					else Shader = "static";
				}
				else if (!strcmp(keyWord, "Ka"))
				{
					const char* RestOfLine = strtok(0, "\r\n");
					CurrMtl += "\t.setvector \"MatAmbient\" ";
					CurrMtl += RestOfLine;
					CurrMtl += " ";
					CurrMtl += Transparency;
					CurrMtl += "\n";
				}
				else if (!strcmp(keyWord, "Kd"))
				{
					const char* RestOfLine = strtok(0, "\r\n");
					CurrMtl += "\t.setvector \"MatDiffuse\" ";
					CurrMtl += RestOfLine;
					CurrMtl += " ";
					CurrMtl += Transparency;
					CurrMtl += "\n";
				}
				else if (!strcmp(keyWord, "Ks"))
				{
					const char* RestOfLine = strtok(0, "\r\n");
					CurrMtl += "\t.setvector \"MatSpecular\" ";
					CurrMtl += RestOfLine;
					CurrMtl += " ";
					CurrMtl += Transparency;
					CurrMtl += "\n";
				}
				else if (!strcmp(keyWord, "Ke"))
				{
					// Now skipped
				}
				else if (!strcmp(keyWord, "Ns"))
				{
					const char* Factor = strtok(0, N_WHITESPACE);
					CurrMtl += "\t.setfloat \"MatSpecularPower\" ";
					CurrMtl += Factor;
					CurrMtl += "\n";
				}
				else if (!strcmp(keyWord, "map_Ka"))
				{
					nString TexFile = strtok(0, "\r\n");
					if (TexFile.IsValid())
					{
						CurrMtl += "\t.settexture \"AmbientMap0\" \"";
						CurrMtl += ProcessTexture(TexFile, OutResourceName, Materials.Size(), "a", "system/white.dds");
						CurrMtl += "\"\n";
					}
				}
				else if (!strcmp(keyWord, "map_Kd"))
				{
					nString TexFile = strtok(0, "\r\n");
					if (TexFile.IsValid())
					{
						CurrMtl += "\t.settexture \"DiffMap0\" \"";
						CurrMtl += ProcessTexture(TexFile, OutResourceName, Materials.Size(), "d", "system/white.dds");
						CurrMtl += "\"\n";
					}
				}
				else if (!strcmp(keyWord, "map_bump") || !strcmp(keyWord, "map_Bump") || !strcmp(keyWord, "bump"))
				{
					char* RestOfLine = strtok(0, "\r\n");
					nString TexFile = RestOfLine;
					const char* Next = strtok(RestOfLine, N_WHITESPACE);
					if (!strcmp(Next, "-bm"))
					{
						// Skip, not used now
						Next = strtok(0, N_WHITESPACE);
						TexFile = strtok(0, N_WHITESPACE);
					}

					if (TexFile.IsValid())
					{
						CurrMtl += "\t.settexture \"BumpMap0\" \"";
						CurrMtl += ProcessTexture(TexFile, OutResourceName, Materials.Size(), "b", "system/nobump.dds");
						CurrMtl += "\"\n";
						//if (TexFile.Length() < 2 || TexFile[1] != ":") TexFile = inFileArg.ExtractDirName() + TexFile;
						HasBump = true;
					}
				}
				else n_printf("WARNING: Token '%s' is unknown, ignored\n", keyWord);
			}

			if (CurrMtlName.IsValid() && CurrMtl.IsValid())
			{
				if (!HasBump)
					CurrMtl += "\t.settexture \"BumpMap0\" \"textures:system/nobump.dds\"\n";
				CurrMtl += "\t.setshader \"";
				CurrMtl += Shader;
				CurrMtl += "\"";
				CurrMtl.ConvertBackslashes();
				Materials.Add(CurrMtlName, CurrMtl);
			}

			File.Close();
		}
	}

	// Create mesh
	nString OutMesh = "meshes:" + OutResourceName + ".nvx2";
	DataSrv->CreateDirectory(OutMesh.ExtractDirName());
	n_printf("-> saving mesh '%s'\n", OutMesh.Get());
    if (!mesh.Save(OutMesh.Get()))
    {
        n_printf("Error: Could not save '%s'\n", OutMesh.Get());
		DataServer = NULL;
        delete kernelServer;
        return 5;
    }

	if (CreateSceneFileArg)
	{
		// Create n2 File group-by-group
		nString OutGfx = "export:gfxlib/" + OutResourceName + ".n2";
		DataSrv->CreateDirectory(OutGfx.ExtractDirName());
		n_printf("-> saving N2 object '%s'\n", OutGfx.Get());
		Data::CFileStream File;
		if (File.Open(OutGfx, Data::SAM_WRITE))
		{
			nString N2(	"# ---\n"
						"# $parser:ntclserver$ $class:ntransformnode$\n"
						"# ---\n"
						".setlocalbox ");
			bbox3 LocalBox = mesh.GetBBox();
			vector3 Center = LocalBox.center(), Extents = LocalBox.extents();
			N2.AppendFloat(Center.x);
			N2 += " ";
			N2.AppendFloat(Center.y);
			N2 += " ";
			N2.AppendFloat(Center.z);
			N2 += " ";
			N2.AppendFloat(Extents.x);
			N2 += " ";
			N2.AppendFloat(Extents.y);
			N2 += " ";
			N2.AppendFloat(Extents.z);
			N2 += "\n\n";

			for (int i = 0; i < GroupToName.Size(); i++)
			{
				nString GroupName;
				if (GroupToName.ValueAtIndex(i).IsValid()) GroupName = GroupToName.ValueAtIndex(i);
				else GroupName.Format("group_%d", i);

				n_printf("-> Creating group '%s'...\n", GroupName.Get());

				N2 += "new nshapenode ";
				N2 += GroupName;
				N2 += "\n\tsel ";
				N2 += GroupName;
				N2 += "\n\t.setlocalbox ";
				
				LocalBox = mesh.GetGroupBBox(i);
				Center = LocalBox.center();
				Extents = LocalBox.extents();
				N2.AppendFloat(Center.x);
				N2 += " ";
				N2.AppendFloat(Center.y);
				N2 += " ";
				N2.AppendFloat(Center.z);
				N2 += " ";
				N2.AppendFloat(Extents.x);
				N2 += " ";
				N2.AppendFloat(Extents.y);
				N2 += " ";
				N2.AppendFloat(Extents.z);
				N2 += "\n";
				
				if (GroupToMtl.Contains(GroupToName.KeyAtIndex(i)))
				{
					const nString& MtlName = GroupToMtl[GroupToName.KeyAtIndex(i)];
					if (Materials.Contains(MtlName)) N2 += Materials[MtlName];
				}
				
				N2 += "\n\t.setmesh \"";
				N2 += OutMesh;
				N2 += "\"\n\t.setgroupindex ";
				N2.AppendInt(GroupToName.KeyAtIndex(i));
				N2 += "\n\t.setneedsvertexshader false\nsel ..\n\n";

				if (ShadowArg)
				{
					n_printf("-> Creating shadow for the current group...\n");
					N2 += "new nshadownode ";
					N2 += GroupName;
					N2 += "_shadow\n\tsel ";
					N2 += GroupName;
					N2 += "_shadow\n\t.setmesh \"";
					N2 += OutMesh;
					N2 += "\"\n\t.setgroupindex ";
					N2.AppendInt(GroupToName.KeyAtIndex(i));
					N2 += "\nsel ..\n\n";
				}
			}
			
			N2 += "# ---\n# Eof";
			File.Write(N2.Get(), N2.Length());
			File.Close();
		}
		else
		{
			n_printf("Error: Could not save '%s'\n", OutGfx.Get());
			DataServer = NULL;
			delete kernelServer;
			return 5;
		}
	}

    // success
	n_printf("Done successfully.\n");
	DataServer = NULL;
	delete kernelServer;
    return 0;
}
//---------------------------------------------------------------------

bool LoadObj(nMeshBuilder* mesh, const char* filename)
{
    
    n_assert(filename);

    nArray<nMeshBuilder::Group> groupMap;
    //int numGroups = 0;
    //int curGroup = 0;
    int numVertices = 0;
    int firstTriangle = 0;
    int numTriangles = 0;
    int vertexComponents = 0;
    nArray<vector3> coordArray;
    nArray<vector3> normalArray;
    nArray<vector2> uvArray;
	nString CurrGroupName = "default";

    nMeshBuilder::Group group;
    nMeshBuilder::Vertex vertex;

    bool retval = false;
    Data::CFileStream File;
    
	if (File.Open(filename, Data::SAM_READ))
    {
		Data::CTextReader Reader(File);

		group.SetId(0);
		group.SetFirstTriangle(0);

		int LineNum = 0, LastGroupLine = -1;
        char line[1024];
        while (Reader.ReadLine(line, sizeof(line)))
        {
			LineNum++;

            // get keyword
            char* keyWord = strtok(line, N_WHITESPACE);
            if (!keyWord) continue;
           
			bool IsUseMtl = (0 == strcmp(keyWord, "usemtl"));

			if (0 == strcmp(keyWord, "mtllib"))
			{
                const char* MtlFileName = strtok(0, N_WHITESPACE);
				if (MtlFileName)
					MtlFiles.Append(MtlFileName);
			}
			else if ((0 == strcmp(keyWord, "g")) || IsUseMtl)
            {
                // a triangle group
                const char* groupName = strtok(0, N_WHITESPACE);

                if (groupName)
                {
					if (IsUseMtl && LastGroupLine == LineNum - 1)
					{
						//just set curr group material
						GroupToMtl.Add(group.GetId(), groupName);
						continue;
					}

					if (!IsUseMtl) LastGroupLine = LineNum;

                    // Check that name is different from default Maya obj exporter adds
					// default groups for some stupid reason.
                    if (0 != strcmp(groupName, "default"))
                    {
						if (firstTriangle < numTriangles)
						{
							GroupToName.Add(group.GetId(), CurrGroupName);
							group.SetNumTriangles(numTriangles - firstTriangle);
							groupMap.Append(group);
							firstTriangle = numTriangles;
						}

						CurrGroupName = groupName;
						group.SetId(groupMap.Size());
						group.SetFirstTriangle(firstTriangle);
						if (IsUseMtl) GroupToMtl.Add(group.GetId(), CurrGroupName);
                    }
                }
            }
            else if (0 == strcmp(keyWord, "v"))
            {
                vertexComponents |= nMeshBuilder::Vertex::COORD;
                const char* xStr = strtok(0, N_WHITESPACE);
                const char* yStr = strtok(0, N_WHITESPACE);
                const char* zStr = strtok(0, N_WHITESPACE);
                n_assert(xStr && yStr && zStr);
                coordArray.Append(vector3((float) atof(xStr), (float) atof(yStr), (float) atof(zStr)));
            }
            else if (0 == strcmp(keyWord, "vn"))
            {
                vertexComponents |= nMeshBuilder::Vertex::NORMAL;
                const char* iStr = strtok(0, N_WHITESPACE);
                const char* jStr = strtok(0, N_WHITESPACE);
                const char* kStr = strtok(0, N_WHITESPACE);
                n_assert(iStr && jStr && kStr);
                normalArray.Append(vector3((float) atof(iStr), (float) atof(jStr), (float) atof(kStr)));
            }
            else if (0 == strcmp(keyWord, "vt"))
            {
                vertexComponents |= nMeshBuilder::Vertex::UV0;
                const char* uStr = strtok(0, N_WHITESPACE);
                const char* vStr = strtok(0, N_WHITESPACE);
                n_assert(uStr && vStr);
                uvArray.Append(vector2((float) atof(uStr), (float) atof(vStr)));
            }
            else if (0 == strcmp(keyWord, "f"))
            {
                int coordIndex;
                int normalIndex = 0;
                int uvIndex;

                char* vertexStr = strtok(0, N_WHITESPACE);
                int vertexNo;
                for (vertexNo = 0; vertexStr != NULL; vertexNo++, vertexStr=strtok(0, N_WHITESPACE))
                {
                    // unpack poly vertex info (coord/uv/normal)

                    // check if uv info exists
                    char* uvStr = strchr(vertexStr, '/');
                    if (uvStr != NULL)
                    {
                        uvStr[0] = 0;
                        uvStr++;
                        coordIndex = atoi(vertexStr);

                        // check if normal info exits
                        char* normalStr = strchr(uvStr, '/');
                        if (normalStr != NULL)
                        {
                            normalStr[0] = 0;
                            normalStr++;
							normalIndex = (normalStr[0] != 0) ? atoi(normalStr) : 0;
                        }

						uvIndex = (uvStr[0] != 0) ? atoi(uvStr) : 0;
                    }
                    else
                    {
                        // no uv and normal info found so presume
                        // that the value is used for all of them
                        coordIndex = atoi(vertexStr);
                        normalIndex = coordIndex;
                        uvIndex = coordIndex;
                    }

                    // if index is negative then calculate positive index
                    if (coordIndex < 0) coordIndex += coordArray.Size() + 1;
                    if (uvIndex < 0) uvIndex += uvArray.Size() + 1;
                    if (normalIndex < 0) normalIndex += normalArray.Size() + 1;

                    // obj files first index is 1 when in c++ it's 0
                    coordIndex--;
                    uvIndex--;
                    normalIndex--;

                    nMeshBuilder::Vertex vertex;
                    if (vertexComponents & nMeshBuilder::Vertex::COORD)
                        vertex.SetCoord(coordArray[coordIndex]);
                    if (vertexComponents & nMeshBuilder::Vertex::NORMAL)
                        vertex.SetNormal(normalArray[normalIndex]);
                    if (vertexComponents & nMeshBuilder::Vertex::UV0)
                        vertex.SetUv(0, uvArray[uvIndex]);
                    mesh->AddVertex(vertex);
                    numVertices++;
                }

                // create triangles from convex polygon
                int firstTriangleVertex = numVertices - vertexNo;
                int lastTriangleVertex = firstTriangleVertex + 2;
                for (; lastTriangleVertex < (numVertices); lastTriangleVertex++)
                {
                    nMeshBuilder::Triangle triangle;
                    triangle.SetVertexIndices(firstTriangleVertex, lastTriangleVertex-1, lastTriangleVertex);
                    mesh->AddTriangle(triangle);
                    numTriangles++;
                }
            } // if-elseif-... token check
        } // line loop---

        if (firstTriangle < numTriangles)
        {
			GroupToName.Add(group.GetId(), CurrGroupName);
            group.SetNumTriangles(numTriangles - firstTriangle);
            groupMap.Append(group);
        }

        File.Close();
        retval = true;
    }

    mesh->BuildTriangleNormals();

    // update the triangle group ids from the group map
    mesh->UpdateTriangleIds(groupMap);
    return retval;
}
//---------------------------------------------------------------------
