using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using DialogLogic;

namespace DialogDesigner
{
    public partial class FormDialogEditor : Form
    {
        private readonly DialogObjectManager _dialogManager;
        private bool _activated;

        public FormDialogEditor()
        {
            InitializeComponent();
            _dialogManager=new DialogObjectManager();
            _controlDialogNode.Init(_dialogManager);

            _dialogManager.RootDirectoryChanged += DialogRootDirectoryChanged;
            _bindingDialogLink.CurrentChanged += DialogNodeChanged;
            _bindingDialog.CurrentChanged += DialogChanged;

            _treeDialogs.ImageList=new ImageList();
            _treeDialogs.ImageList.Images.Add("Folder", Properties.Resources.folder);
            _treeDialogs.ImageList.Images.Add("Dialog", Properties.Resources.messages);
            _treeDialogs.ImageList.Images.Add("FolderBlue", Properties.Resources.folder_blue);

            DialogRootDirectoryChanged();
        }

        private void DialogChanged(object sender, EventArgs e)
        {
            var dialog = ((BindingSource) sender).Current as DialogObject;
            _controlDialogNode.Dialog = dialog;
            _controlTree.Dialog = dialog == null ? null : dialog.Dialog;
        }

        private void DialogRootDirectoryChanged()
        {
            _treeDialogs.Nodes.Clear();
            TreeNode rootNode;
            if(_dialogManager.RootDirectory==null)
            {
                rootNode = new TreeNode {Text = @"[Virtual directory]"};
                rootNode.SelectedImageKey = rootNode.ImageKey = @"FolderBlue";
            }
            else
            {
                var dInfo = new DirectoryInfo(_dialogManager.RootDirectory);
                rootNode = CreateTreeNode(dInfo);
                rootNode.Text = dInfo.FullName;
            }
            rootNode.Tag = _dialogManager;
            _treeDialogs.Nodes.Add(rootNode);
        }

        private void DialogNodeChanged(object sender, EventArgs e)
        {
            _controlDialogNode.Link = (DialogGraphLink)_bindingDialogLink.Current;
        }

        private void ToolButtonNewClick(object sender, EventArgs e)
        {
            if(!CheckSaveDialogBool())
                return;

            CreateNewDialogs();
        }

        private bool CheckSaveDialogBool()
        {
            return CheckSaveDialog() == DialogResult.Yes;
        }

        private DialogResult CheckSaveDialog()
        {
            if (_dialogManager.HasChanges)
            {
                var saveDlg = new SaveForm(_dialogManager);
                List<string> list = null;
                DialogResult res;
                if (_dialogManager.CheckDialogsForChanges())
                {
                    res = saveDlg.ShowDialog(this);
                    list = saveDlg.ListToSave;
                }
                else
                    res = MessageBox.Show(this, @"Do you want to save changes?", Text, MessageBoxButtons.OK);

                switch (res)
                {
                    case DialogResult.Yes:
                        try
                        {
                            SaveDialog(list);
                            return DialogResult.Yes;
                        }
                        catch (Exception ex)
                        {
                            this.ShowError(ex);
                            return DialogResult.Cancel;
                        }

                    default:
                        return res;
                }
            }

            return DialogResult.Yes;
        }

        private void SaveDialog(List<string> list)
        {
            string rootDir = _dialogManager.RootDirectory;
            if(_dialogManager.RootDirectory==null)
            {
                if(MessageBox.Show(this,@"Root directory is not defined. Do you want to select it now?",@"Root directory",MessageBoxButtons.OKCancel)==DialogResult.OK)
                {
                    var fbd = new FolderBrowserDialog();
                    if(fbd.ShowDialog(this)==DialogResult.OK)
                    {
                        rootDir = fbd.SelectedPath;
                    }
                    else
                        return;
                }
                else
                    return;
            }

            if (_dialogManager.XmlFile == null)
            {
                var sfd = new SaveFileDialog
                              {
                                  Filter = @"*.xml|*xml|*.*|*.*",
                                  FileName = "default.xml",
                                  InitialDirectory = ProgramEnvironment.Configuration.DefaultDirectory
                              };
                if (sfd.ShowDialog(this) == DialogResult.OK)
                {
                    _dialogManager.SaveChanges(sfd.FileName, rootDir, list);
                    ProgramEnvironment.Configuration.DefaultDirectory = Path.GetDirectoryName(sfd.FileName);
                    ProgramEnvironment.Configuration.RecentProjectFile = Path.GetFullPath(sfd.FileName);
                }
            }
            else
                _dialogManager.SaveChanges(_dialogManager.XmlFile, rootDir, list);
        }

        private void CreateNewDialogs()
        {
            _dialogManager.Reset();
        }

        private void ToolButtonSaveAsClick(object sender, EventArgs e)
        {
            var sfd = new SaveFileDialog
            {
                Filter = @"*.xml|*.xml|*.*|*.*",
                FileName = "default.xml",
                InitialDirectory = ProgramEnvironment.Configuration.DefaultDirectory
            };
            if(sfd.ShowDialog(this)==DialogResult.OK)
            {
                try
                {
                    _dialogManager.SaveChanges(sfd.FileName, _dialogManager.RootDirectory, null);
                    ProgramEnvironment.Configuration.DefaultDirectory = Path.GetDirectoryName(sfd.FileName);
                }
                catch(Exception ex)
                {
                    this.ShowError(ex);
                }
            }
        }

        private void ControlTreeSelectedLinkChanged(object sender, EventArgs e)
        {
            var link = ((ControlDialogTree) sender).SelectedLink;
            if (ReferenceEquals(_bindingDialogLink.DataSource, link))
                _bindingDialogLink.ResetBindings(false);
            else
                _bindingDialogLink.DataSource = ((ControlDialogTree) sender).SelectedLink;
        }

        private void CmDialogPropertiesClick(object sender, EventArgs e)
        {
            var item = ((ToolStripMenuItem) sender).Tag as TreeNode;
            var path = GetRelativePath(item);
            var dlgObj=_dialogManager.GetOrCreateDialogObject(path);

            var propDlg = new FormDialogProperties(_dialogManager, dlgObj);
            if(propDlg.ShowDialog(this)==DialogResult.OK)
            {
                _bindingDialog.ResetBindings(false);
            }
        }

        private void CmDialogPlayClick(object sender, EventArgs e)
        {
            var node = ((ToolStripMenuItem)sender).Tag as TreeNode;
            if (node == null)
                return;

            var path = GetRelativePath(node);
            var item = _dialogManager.GetOrCreateDialogObject(path);

            var player = new FormDialogPlayer(item.Dialog);
            player.ShowDialog(this);
        }

        private void FormDialogEditorClosing(object sender, FormClosingEventArgs e)
        {
            if(CheckSaveDialog()==DialogResult.Cancel)
                e.Cancel = true;
        }

        private void ToolButtonCharacterEditorClick(object sender, EventArgs e)
        {
            var frm = new FormCharactersProperties(_dialogManager.Dialogs);
            frm.ShowDialog(this);
        }

        private static TreeNode CreateTreeNode(DirectoryInfo directoryInfo)
        {
            var node = new TreeNode(directoryInfo.Name);
            node.SelectedImageKey = node.ImageKey = @"Folder";
            foreach (var subDir in directoryInfo.GetDirectories())
            {
                if((subDir.Attributes&(FileAttributes.Hidden|FileAttributes.System))!=0)
                    continue;
                node.Nodes.Add(CreateTreeNode(subDir));
            }

            foreach (var fileInfo in directoryInfo.GetFiles("*.dlg"))
                node.Nodes.Add(CreateFileNode(fileInfo));

            return node;
        }

        private static TreeNode CreateFileNode(FileInfo info)
        {
            var node = new TreeNode(Path.GetFileNameWithoutExtension(info.FullName));
            node.SelectedImageKey = node.ImageKey = @"Dialog";
            return node;
        }

        private string GetRelativePath(TreeNode node)
        {
            var fullPath = node.FullPath;
            int len = 0;
            if(_treeDialogs.Nodes.Count>0)
            {
                var rootNode = _treeDialogs.Nodes[0];
                len = rootNode.Text.Length+ 1;
            }

            if (len > 0)
            {
                if (len < fullPath.Length)
                    fullPath = fullPath.Substring(len);
                else
                    fullPath = string.Empty;
            }

            return fullPath;
        }

        private void ToolButtonOpenClick(object sender, EventArgs e)
        {
            if (CheckSaveDialog() != DialogResult.Cancel)
            {
                var ofd = new OpenFileDialog {Filter = @"*.xml|*.xml|*.*|*.*", InitialDirectory=ProgramEnvironment.Configuration.DefaultDirectory};
                if(ofd.ShowDialog(this)==DialogResult.OK)
                {
                    _dialogManager.XmlFile = ofd.FileName;
                    ProgramEnvironment.Configuration.DefaultDirectory = Path.GetDirectoryName(ofd.FileName);
                    ProgramEnvironment.Configuration.RecentProjectFile = Path.GetFullPath(ofd.FileName);
                }
            }
        }

        private void ToolButtonSaveClick(object sender, EventArgs e)
        {
            SaveDialog(null);
        }

        private void TreeDialogsAfterSelect(object sender, TreeViewEventArgs e)
        {
            if(_treeDialogs.SelectedNode!=null)
            {
                if (_treeDialogs.SelectedNode.ImageKey == @"Dialog")
                {
                    try
                    {
                        var fPath = GetRelativePath(_treeDialogs.SelectedNode);
                        var obj = _dialogManager.GetOrCreateDialogObject(fPath);
                        _bindingDialog.DataSource = obj;
                    }
                    catch(Exception ex)
                    {
                        this.ShowError(ex);
                    }
                }
                else
                    _bindingDialog.DataSource = null;
            }
            else
                _bindingDialog.DataSource = null;
        }

        private void TreeDialogsNodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            if(e.Button==MouseButtons.Right)
            {
                _treeDialogs.SelectedNode = e.Node;
                if(e.Node!=null)
                {
                    switch(e.Node.ImageKey)
                    {
                        case "Folder":
                            foreach (ToolStripMenuItem item in _cmFolder.Items)
                            {
                                item.Tag = e.Node;
                                item.Visible = true;
                            }
                            _cmFolderMap.Visible = false;

                            _cmFolderDelete.Enabled = !_treeDialogs.Nodes.Contains(e.Node);

                            _cmFolder.Show(MousePosition);
                            break;
                        case "Dialog":
                            foreach (ToolStripMenuItem item in _cmDialog.Items)
                                item.Tag = e.Node;

                            _cmDialog.Show(MousePosition);
                            break;

                        // It's a virtual folder
                        case "FolderBlue":
                            foreach (ToolStripMenuItem item in _cmFolder.Items)
                            {
                                item.Tag = e.Node;
                                item.Visible = false;
                            }

                            _cmFolderMap.Enabled = true;
                            _cmFolderMap.Visible = true;

                            _cmFolder.Show(MousePosition);
                            break;
                    }
                }
            }
        }

        private void CmFolderMapClick(object sender, EventArgs e)
        {
            var fbd = new FolderBrowserDialog();
            if(fbd.ShowDialog(this)==DialogResult.OK)
            {
                _dialogManager.RootDirectory = fbd.SelectedPath;
            }
        }

        private void CmFolderAddFolderClick(object sender, EventArgs e)
        {
            var node = ((ToolStripMenuItem) sender).Tag as TreeNode;
            if(node==null)
                return;

            var folderNode = AddNode("Folder", "New folder", node);
            node.Expand();
            folderNode.BeginEdit();
        }

        private void CmFolderAddDialogClick(object sender, EventArgs e)
        {
            var node = ((ToolStripMenuItem) sender).Tag as TreeNode;
            if(node==null)
                return;

            var dialogNode = AddNode("Dialog", "New dialog", node);
            node.Expand();
            dialogNode.BeginEdit();
        }

        private void CmFolderDeleteClick(object sender, EventArgs e)
        {
            var node = ((ToolStripMenuItem) sender).Tag as TreeNode;
            if(node==null)
                return;

            var relPath = GetRelativePath(node);
            var fullPath = Path.Combine(_dialogManager.RootDirectory, relPath);
            var questionString = string.Format("Do you really want to delete folder '{0}' and all its content?", fullPath);

            if(MessageBox.Show(this,questionString,"Delete folder",MessageBoxButtons.YesNo)==DialogResult.Yes)
            {
                _dialogManager.RemoveFolder(relPath);
                node.Remove();
            }
        }

        private void CmDialogDeleteClick(object sender, EventArgs e)
        {
            var node = ((ToolStripMenuItem)sender).Tag as TreeNode;
            if (node == null)
                return;

            var relPath = GetRelativePath(node);
            var questionString = string.Format("Do you really want to delete dialog '{0}'?", node.Text);

            if (MessageBox.Show(this, questionString, "Delete dialog", MessageBoxButtons.YesNo) == DialogResult.Yes)
            {
                _dialogManager.RemoveDialog(relPath);
                node.Remove();
            }
        }

        private void TreeDialogsBeforeLabelEdit(object sender, NodeLabelEditEventArgs e)
        {
            if(_treeDialogs.Nodes.Contains(e.Node))
            {
                e.CancelEdit = true;
            }
            else if(_treeDialogs.SelectedNode==e.Node && _activated)
            {
                _activated = false;
                e.CancelEdit = true;
            }
        }

        private static TreeNode AddNode(string imageKey, string defaultName, TreeNode parentNode)
        {
            var childNode = new TreeNode { ImageKey = imageKey, SelectedImageKey = imageKey };
            HashSet<string> additionalNames = new HashSet<string>();
            string newName = defaultName.ToLower();
            foreach (var node in parentNode.Nodes.Cast<TreeNode>().Where(n => n.ImageKey == imageKey))
            {
                var cName = node.Text.ToLower();
                
                if (cName.StartsWith(newName))
                {
                    int len = cName.Length - newName.Length;
                    string str = len > 0 ? cName.Substring(newName.Length, len) : string.Empty;
                    Debug.Assert(!additionalNames.Contains(str));
                    additionalNames.Add(str);
                }
            }

            if (!additionalNames.Contains(string.Empty))
                childNode.Text = defaultName;
            else
            {
                int i = 1;
                for (; additionalNames.Contains(i.ToString()); i++) ; /*it's correct*/
                childNode.Text = defaultName + i;
            }

            parentNode.Nodes.Add(childNode);
            return childNode;
        }

        private void TreeDialogsAfterLabelEdit(object sender, NodeLabelEditEventArgs e)
        {
            var parent = e.Node.Parent;
            if (!string.IsNullOrEmpty(e.Label))
            {
                foreach (TreeNode sibling in parent.Nodes)
                {
                    if (sibling == e.Node || sibling.ImageKey != e.Node.ImageKey)
                        continue;

                    if (string.Equals(sibling.Text, e.Label, StringComparison.OrdinalIgnoreCase))
                    {
                        e.CancelEdit = true;
                        break;
                    }
                }
            }
            else
                e.CancelEdit = true;

            if(!e.CancelEdit)
            {
                var relPath = GetRelativePath(e.Node);
                if (e.Node.ImageKey == @"Folder")
                    _dialogManager.RenameFolder(relPath, e.Label);
                else if (e.Node.ImageKey == @"Dialog")
                    _dialogManager.RenameDialog(relPath, e.Label);
            }
        }

        private void FormDialogEditorLoad(object sender, EventArgs e)
        {
            string xmlFile = ProgramEnvironment.Configuration.RecentProjectFile;
            if(!string.IsNullOrEmpty(xmlFile) && File.Exists(xmlFile))
                _dialogManager.XmlFile = xmlFile;
        }

        private void TreeDialogsEnter(object sender, EventArgs e)
        {
            _activated = true;
        }
    }
}
