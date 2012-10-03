using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows.Forms;

namespace DialogDesigner
{
    public partial class SaveForm : Form
    {
        private readonly DialogObjectManager _manager;
        public List<string> ListToSave { get; set; }

        private bool _checkSelect = true;

        public SaveForm(DialogObjectManager manager)
        {
            InitializeComponent();
            
            _tree.ImageList=new ImageList();
            _tree.ImageList.Images.Add("Folder", Properties.Resources.folder);
            _tree.ImageList.Images.Add("FolderBlue", Properties.Resources.folder_blue);
            _tree.ImageList.Images.Add("Dialog", Properties.Resources.message);

            _manager = manager;
        }

        private void SaveFormLoad(object sender, EventArgs e)
        {
            var list = _manager.GetChangedFiles();
            GenerateTree(list);
        }

        private void GenerateTree(IEnumerable<string> list)
        {
            var rootNode = new TreeNode {Text = _manager.RootDirectory};
            rootNode.ImageKey = rootNode.SelectedImageKey = @"FolderBlue";
            
            foreach(var str in list.OrderBy(it=>it))
            {
                var path = str.Split(new[] {'\\'});
                var node = GetOrCreatePathNode(path, 0, path.Length - 1, rootNode);
                var newNode = new TreeNode {Text = path[path.Length - 1]};
                node.Nodes.Add(newNode);
                newNode.ImageKey = newNode.SelectedImageKey = @"Dialog";
            }

            _tree.Nodes.Add(rootNode);

            rootNode.Checked = true;
            _tree.ExpandAll();
        }

        private static TreeNode GetOrCreatePathNode(string[] relativePath, int position, int count, TreeNode parentNode)
        {
            if(count==0)
                return parentNode;

            var newNode = new TreeNode {Text = relativePath[position]};
            newNode.ImageKey = newNode.SelectedImageKey = @"Folder";
            parentNode.Nodes.Add(newNode);

            return GetOrCreatePathNode(relativePath, position + 1, count - 1, newNode);
        }

        private void TreeAfterCheck(object sender, TreeViewEventArgs e)
        {
            if(!_checkSelect)
                return;

            _checkSelect = false;

            CheckChildren(e.Node);
            CheckParents(e.Node);

            _checkSelect = true;
        }

        private static void CheckParents(TreeNode node)
        {
            var parent = node.Parent;
            if(parent==null)
                return;

            bool isChecked = parent.Nodes.Cast<TreeNode>().All(n => n.Checked);
            if(isChecked!=parent.Checked)
            {
                parent.Checked = isChecked;
                CheckParents(parent);
            }
        }

        private static void CheckChildren(TreeNode node)
        {
            foreach(TreeNode child in node.Nodes)
            {
                child.Checked = node.Checked;
                CheckChildren(child);
            }
        }

        private void ButtonNoClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.No;
            Close();
        }

        private void ButtonYesClick(object sender, EventArgs e)
        {
            ListToSave = new List<string>();
            CollectChangesList(_tree.Nodes[0], null);
            DialogResult = DialogResult.Yes;
            Close();
        }

        private void CollectChangesList(TreeNode startNode, string path)
        {
            foreach(TreeNode subNode in startNode.Nodes)
            {
                if(subNode.ImageKey==@"Dialog")
                {
                    if(subNode.Checked==false)
                        continue;

                    string nodeName = string.IsNullOrEmpty(path) ? subNode.Text : Path.Combine(path, subNode.Text);
                    ListToSave.Add(nodeName);
                    continue;
                }

                var subPath = string.IsNullOrEmpty(path) ? subNode.Text : Path.Combine(path, subNode.Text);
                CollectChangesList(subNode, subPath);
            }
   
        }
    }
}