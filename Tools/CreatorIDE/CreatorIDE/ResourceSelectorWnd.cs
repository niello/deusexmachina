using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.IO;
using System.Windows.Forms;

namespace CreatorIDE
{
    public partial class ResourceSelectorWnd : Form
    {
        string ResName, RootPath, Extension;

        public class NodeData
        {
            public bool IsDirectory;
            public bool WasExpanded = false;

            public NodeData(bool IsDir)
            {
                IsDirectory = IsDir;
            }
        }

        public ResourceSelectorWnd(string Root, string Ext)
        {
            InitializeComponent();
            RootPath = Root.Replace('\\', '/');
            Extension = Ext;
        }

        public string ResourceName
        {
            get { return ResName; }
            set
            {
                ResName = value.Replace('\\', '/');
                if (FillDirectory(null))
                {
                    TreeNodeCollection Nodes = tvFS.Nodes;
                    char[] Sep = { '/' };
                    string[] Tokens = ResName.Split(Sep);
                    foreach (string Token in Tokens)
                    {
                        if (!Nodes.ContainsKey(Token)) break;
                        TreeNode Node = Nodes[Token];
                        
                        tvFS.SelectedNode = Node;

                        NodeData d = (NodeData)Node.Tag;
                        if (d.IsDirectory)
                        {
                            if (!d.WasExpanded)
                            {
                                FillDirectory(Node);
                                d.WasExpanded = true;
                            }
                            Node.Expand();
                        }
                        else break;

                        Nodes = Node.Nodes;
                    }
                }
            }
        }

        private bool FillDirectory(TreeNode Node)
        {
            TreeNodeCollection NodesToFill = (Node != null) ? Node.Nodes : tvFS.Nodes;
            string DirPath = RootPath;
            if (Node != null) DirPath += "/" + Node.FullPath;

            NodesToFill.Clear();

            if (Directory.Exists(DirPath))
            {
                string[] Dirs = Directory.GetDirectories(DirPath, "*", SearchOption.TopDirectoryOnly);
                foreach (string Dir in Dirs)
                {
                    string Key = Path.GetFileNameWithoutExtension(Dir);
                    TreeNode n = NodesToFill.Add(Key, Key);
                    n.Tag = new NodeData(true);
                }
                string[] Files = Directory.GetFiles(DirPath, "*." + Extension, SearchOption.TopDirectoryOnly);
                foreach (string File in Files)
                {
                    string Key = Path.GetFileNameWithoutExtension(File);
                    TreeNode n = NodesToFill.Add(Key, Key);
                    n.Tag = new NodeData(false);
                }
                return true;
            }
            return false;
        }

        private void bCancel_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
            Close();
        }

        private void bApply_Click(object sender, EventArgs e)
        {
            Apply();
        }

        private void Apply()
        {
            int Depth = 1;
            foreach (char c in ResName)
                if (c == '/') Depth++;

            if (Depth >= 16)
            {
                MessageBox.Show("Данный ресурс превышает предельную вложенность для NPK-файлов, максимальная глубина - 15",
                    "Невозможно использовать ресурс", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            DialogResult = DialogResult.OK;
            Close();
        }

        private void tvFS_NodeMouseDoubleClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            NodeData d = (NodeData)e.Node.Tag;
            if (d.IsDirectory)
            {
                if (!d.WasExpanded)
                {
                    FillDirectory(e.Node);
                    d.WasExpanded = true;
                }
                e.Node.Expand();
            }
            else Apply();
        }

        private void tvFS_AfterSelect(object sender, TreeViewEventArgs e)
        {
            bool IsFile = !(e.Node.Tag as NodeData).IsDirectory;
            bApply.Enabled = IsFile;
            if (IsFile)
            {
                ResName = e.Node.FullPath;
                tPreview.Text = File.ReadAllText(RootPath + "/" + e.Node.FullPath + "." + Extension);
            }
            else tPreview.Text = "<Это папка>";
        }

        private void bClear_Click(object sender, EventArgs e)
        {
            ResName = "";
            DialogResult = DialogResult.OK;
            Close();
        }
    }
}
