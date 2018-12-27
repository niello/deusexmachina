using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace ContentForge
{
    public partial class Main : Form
    {
        String ProjectRoot = "..\\..\\..\\InsanePoet\\Content\\";
		String ExportRoot;
        
        public Main()
        {
            InitializeComponent();

			tProjDir.Text = ProjectRoot;
        }

        private void bOpenBT_Click(object sender, EventArgs e)
        {
            OD.Filter = "BT file|*.bt";
            if (OD.ShowDialog() == DialogResult.OK) tBTFName.Text = OD.FileName;
        }

        private void bOpenAlpha_Click(object sender, EventArgs e)
        {
            OD.Filter = "TGA file|*.tga";
            if (OD.ShowDialog() == DialogResult.OK) tAlphaFName.Text = OD.FileName;
        }

        private void bCreateTerrain_Click(object sender, EventArgs e)
        {
			tOutput.Clear();

			//!!!BAD!
			ExportRoot = tProjDir.Text + "Export\\";

            tBTFName.Text = tBTFName.Text.Trim();
            tAlphaFName.Text = tAlphaFName.Text.Trim();
            tTerrainResName.Text = tTerrainResName.Text.Trim();
            tTexR.Text = tTexR.Text.Trim();
            tTexG.Text = tTexG.Text.Trim();
            tTexB.Text = tTexB.Text.Trim();
            tTexA.Text = tTexA.Text.Trim();

            if (tBTFName.Text.Length < 1) return;
            if (tAlphaFName.Text.Length < 1) return;
            if (tTerrainResName.Text.Length < 1) return;

            //Directory.GetCurrentDirectory
            char[] sep = { '/', '\\' };
            string[] ResTokens = tTerrainResName.Text.Split(sep, StringSplitOptions.RemoveEmptyEntries);
            string RelResourcePath = "";
            for (int i = 0; i < ResTokens.Length - 1; i++) RelResourcePath += ResTokens[i];
            Directory.CreateDirectory(ExportRoot + RelResourcePath);

            String CmdLineArgs = "-d " + nChuDepth.Value.ToString();
            CmdLineArgs += " -e " + nChuError.Value.ToString().Replace(',', '.');
            if (Math.Abs((double)nChuScale.Value - 1.0) > 0.0001f)
                CmdLineArgs += " -v " + nChuScale.Value.ToString().Replace(',', '.');
            CmdLineArgs += " \"" + tBTFName.Text + "\" \"" + ExportRoot + tTerrainResName.Text + ".chu\"";

            tOutput.Text = CommandLine.Run("HFChunk.exe", CmdLineArgs, false);

            tOutput.Text += "\n\nCHU file size: " + File.ReadAllBytes(ExportRoot + tTerrainResName.Text + ".chu").Length / 1024.0f + " KB\n\n\n";

            String OutFile = ExportRoot + tTerrainResName.Text + ".tqt2";
            CmdLineArgs = "-depth " + nTqtDepth.Value.ToString();
            CmdLineArgs += " -in \"" + tAlphaFName.Text + "\" -out \"" + OutFile + "\"";

            tOutput.Text += CommandLine.Run("nmaketqt2.exe", CmdLineArgs, false);

            tOutput.Text += "\n\nTQT2 file size: " + File.ReadAllBytes(OutFile).Length / 1024.0f + " KB\n\n\n";

            if (cbAlphaDXT5.Checked)
            {
                File.Copy(OutFile, OutFile + "raw");
                
                tOutput.Text += CommandLine.Run("nmaketqt2.exe", "-in \"" + OutFile + "raw\" -out \"" + OutFile + "\" -dxt5", false);

                File.Delete(OutFile + "raw");
                
                tOutput.Text +=
                    "\n\nTQT2 file size (DXT5): " +
                    File.ReadAllBytes(ExportRoot + tTerrainResName.Text + ".tqt2").Length / 1024.0f +
                    " KB\n\n\n";
            }

            if (tTexR.Text.Length < 1) tTexR.Text = "system/mlp_black.dds";
            if (tTexG.Text.Length < 1) tTexG.Text = "system/mlp_black.dds";
            if (tTexB.Text.Length < 1) tTexB.Text = "system/mlp_black.dds";
            if (tTexA.Text.Length < 1) tTexA.Text = "system/mlp_black.dds";

            //!!!calc local box?! or N2 autocalcs it?

            tOutput.Text += "\n\nCreating N2 file...\n";
            OutFile = ExportRoot + "gfxlib\\" + tTerrainResName.Text + ".n2";
            Directory.CreateDirectory(ExportRoot + "gfxlib\\" + RelResourcePath);
            StreamWriter N2File = new StreamWriter(OutFile);
            N2File.WriteLine("# ---");
            N2File.WriteLine("# $parser:ntclserver$ $class:ntransformnode$");
            N2File.WriteLine("# ---");
            N2File.WriteLine("");
            N2File.WriteLine(".setlocalbox 0 0 0 1000 500 1000"); //!!!wrong!
            N2File.WriteLine("");
            N2File.WriteLine("new nterrainnode " + ResTokens.Last());
            N2File.WriteLine("    sel " + ResTokens.Last());
            N2File.WriteLine("	    .setchunkfile \"export:" + tTerrainResName.Text + ".chu\"");
            N2File.WriteLine("	    .settexquadfile \"export:" + tTerrainResName.Text + ".tqt2\"");
            N2File.WriteLine("	    .setmaxpixelerror 1.0");
            N2File.WriteLine("	    .setmaxtexelsize 1.0");
            N2File.WriteLine("	    .setterrainscale 1.0");
            N2File.WriteLine("	    .setterrainorigin 0.0 0.0 0.0");
            N2File.WriteLine("	    .settexture \"DiffMap0\" \"textures:" + tTexR.Text + "\"");
            N2File.WriteLine("	    .settexture \"DiffMap1\" \"textures:" + tTexG.Text + "\"");
            N2File.WriteLine("	    .settexture \"DiffMap2\" \"textures:" + tTexB.Text + "\"");
            N2File.WriteLine("	    .settexture \"DiffMap3\" \"textures:" + tTexA.Text + "\"");
            N2File.WriteLine("	    .setint \"CullMode\" 2");
            N2File.WriteLine("	    .setvector \"MatDiffuse\" 1.0 1.0 1.0 1.0");
            N2File.WriteLine("	    .setvector \"MatSpecular\" 0.0 0.0 0.0 1.0");
            N2File.WriteLine("	    .setvector \"MatEmissive\" 0.0 0.0 0.0 0.0");
            N2File.WriteLine("	    .setfloat \"MatEmissiveIntensity\" 0.0");
            N2File.WriteLine("	    .setshader \"terrain\"");
            N2File.WriteLine("    sel ..");
            N2File.WriteLine("");
            N2File.WriteLine("# ---");
            N2File.WriteLine("# Eof");
            N2File.Close();
            tOutput.Text += "\n\nN2 file size: " + File.ReadAllBytes(OutFile).Length + " B\n\n\n";

            if (cbPhysHRD.Checked)
            {
                //!!!only for BT, will not work with image-based heightmaps!
                tOutput.Text += "\n\nCopying BT file for collision...\n";
                OutFile = ExportRoot + tTerrainResName.Text + ".bt";
                File.Copy(tBTFName.Text, OutFile, true);
                tOutput.Text += "\n\nBT file size: " + File.ReadAllBytes(OutFile).Length / 1024.0f + " KB\n\n\n";

                tOutput.Text += "\n\nCreating physics HRD descriptor...\n";
                OutFile = ExportRoot + "physics\\" + tTerrainResName.Text + ".hrd";
                Directory.CreateDirectory(ExportRoot + "physics\\" + RelResourcePath);
                StreamWriter PhysicsFile = new StreamWriter(OutFile);
                PhysicsFile.WriteLine("Type = \"Composite\"");
                PhysicsFile.WriteLine("Shapes");
                PhysicsFile.WriteLine("[");
                PhysicsFile.WriteLine("\t{");
                PhysicsFile.WriteLine("\t\tType = \"HeightfieldShape\"");
                PhysicsFile.WriteLine("\t\tFile = \"export:" + tTerrainResName.Text + ".bt\"");
                PhysicsFile.WriteLine("\t}");
                PhysicsFile.WriteLine("]");
                PhysicsFile.Close();
                tOutput.Text += "\n\nHRD file size: " + File.ReadAllBytes(OutFile).Length + " B\n\n\n";
            }
        }

		private void bOpenModelSrc_Click(object sender, EventArgs e)
		{
			OD.Filter = "Wavefront OBJ file|*.obj";
			if (OD.ShowDialog() == DialogResult.OK) tModelName.Text = OD.FileName;
		}

		private void bCreateModel_Click(object sender, EventArgs e)
		{
            tOutput.Clear();

			//!!!BAD!
			ExportRoot = tProjDir.Text + "Export\\";

            tModelName.Text = tModelName.Text.Trim();
            tModelResName.Text = tModelResName.Text.Trim();

            String SrcPath = tModelName.Text;
            if (!Path.IsPathRooted(SrcPath)) SrcPath = tProjDir.Text + "Src\\" + SrcPath;
            if (SrcPath.Contains(' ')) SrcPath = "\"" + SrcPath + "\"";

            String ProjDir = tProjDir.Text;
            if (ProjDir.Contains(' ')) ProjDir = "\"" + ProjDir + "\"";

            String CmdLineArgs = "-in " + SrcPath + " -out " + tModelResName.Text + " -proj " + ProjDir;

            if (cbModelClean.Checked) CmdLineArgs += " -clean";
            if (cbModelNormals.Checked) CmdLineArgs += " -normal";
            if (cbModelEdges.Checked) CmdLineArgs += " -edge";
			if (cbModelInvertTexV.Checked) CmdLineArgs += " -invertv";
			if (cbModelGenerateShadow.Checked) CmdLineArgs += " -shadow";
			if (rbModelTangentsNoSplit.Checked) CmdLineArgs += " -tangent";
            else if (rbModelTangentsSplit.Checked) CmdLineArgs += " -tangentsplit";

            if (tfSX.Value != 1)
            {
                if (cbModelUniformScale.Checked) CmdLineArgs += " -scale " + tfSX.Value.ToString().Replace(",", ".");
                else
                {
                    CmdLineArgs += " -sx " + tfSX.Value.ToString().Replace(",", ".");
                    if (tfSY.Value != 1) CmdLineArgs += " -sy " + tfSY.Value.ToString().Replace(",", ".");
                    if (tfSZ.Value != 1) CmdLineArgs += " -sz " + tfSZ.Value.ToString().Replace(",", ".");
                }
            }

            if (tfRotX.Value != 0) CmdLineArgs += " -rx " + tfRotX.Value.ToString().Replace(",", ".");
            if (tfRotY.Value != 0) CmdLineArgs += " -ry " + tfRotY.Value.ToString().Replace(",", ".");
            if (tfRotZ.Value != 0) CmdLineArgs += " -rz " + tfRotZ.Value.ToString().Replace(",", ".");

            if (tfPosX.Value != 0) CmdLineArgs += " -tx " + tfPosX.Value.ToString().Replace(",", ".");
            if (tfPosY.Value != 0) CmdLineArgs += " -ty " + tfPosY.Value.ToString().Replace(",", ".");
            if (tfPosZ.Value != 0) CmdLineArgs += " -tz " + tfPosZ.Value.ToString().Replace(",", ".");
            
            tOutput.Text += CommandLine.Run("ConvMdl.exe", CmdLineArgs, false);
        }

		private void cbGenerateShadow_CheckedChanged(object sender, EventArgs e)
		{
			if (cbModelGenerateShadow.Checked) cbModelEdges.Checked = true;
		}
    }
}
