using System.Collections.Generic;
using DialogLogic;

namespace DialogDesigner
{
    partial class FormDialogEditor
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this._splitContainerV = new System.Windows.Forms.SplitContainer();
            this._treeDialogs = new System.Windows.Forms.TreeView();
            this._controlTree = new DialogDesigner.ControlDialogTree();
            this._cmDialog = new System.Windows.Forms.ContextMenuStrip(this.components);
            this._cmDialogDelete = new System.Windows.Forms.ToolStripMenuItem();
            this._cmDialogPlay = new System.Windows.Forms.ToolStripMenuItem();
            this._cmDialogProperties = new System.Windows.Forms.ToolStripMenuItem();
            this._splitContainerH = new System.Windows.Forms.SplitContainer();
            this._controlDialogNode = new DialogDesigner.ControlDialogNodeEditor();
            this._toolStrip = new System.Windows.Forms.ToolStrip();
            this._toolButtonNew = new System.Windows.Forms.ToolStripButton();
            this._toolButtonOpen = new System.Windows.Forms.ToolStripButton();
            this._toolButtonSave = new System.Windows.Forms.ToolStripButton();
            this._toolButtonSaveAs = new System.Windows.Forms.ToolStripButton();
            this._toolButtonCharacterEditor = new System.Windows.Forms.ToolStripButton();
            this._cmFolder = new System.Windows.Forms.ContextMenuStrip(this.components);
            this._cmFolderMap = new System.Windows.Forms.ToolStripMenuItem();
            this._cmFolderAddDialog = new System.Windows.Forms.ToolStripMenuItem();
            this._cmFolderAddFolder = new System.Windows.Forms.ToolStripMenuItem();
            this._cmFolderDelete = new System.Windows.Forms.ToolStripMenuItem();
            this._bindingDialogLink = new System.Windows.Forms.BindingSource(this.components);
            this._bindingDialog = new System.Windows.Forms.BindingSource(this.components);
            this._splitContainerV.Panel1.SuspendLayout();
            this._splitContainerV.Panel2.SuspendLayout();
            this._splitContainerV.SuspendLayout();
            this._cmDialog.SuspendLayout();
            this._splitContainerH.Panel1.SuspendLayout();
            this._splitContainerH.Panel2.SuspendLayout();
            this._splitContainerH.SuspendLayout();
            this._toolStrip.SuspendLayout();
            this._cmFolder.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._bindingDialogLink)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._bindingDialog)).BeginInit();
            this.SuspendLayout();
            // 
            // _splitContainerV
            // 
            this._splitContainerV.Dock = System.Windows.Forms.DockStyle.Fill;
            this._splitContainerV.Location = new System.Drawing.Point(0, 0);
            this._splitContainerV.Name = "_splitContainerV";
            // 
            // _splitContainerV.Panel1
            // 
            this._splitContainerV.Panel1.Controls.Add(this._treeDialogs);
            // 
            // _splitContainerV.Panel2
            // 
            this._splitContainerV.Panel2.Controls.Add(this._controlTree);
            this._splitContainerV.Size = new System.Drawing.Size(540, 255);
            this._splitContainerV.SplitterDistance = 248;
            this._splitContainerV.TabIndex = 0;
            // 
            // _treeDialogs
            // 
            this._treeDialogs.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._treeDialogs.HideSelection = false;
            this._treeDialogs.LabelEdit = true;
            this._treeDialogs.Location = new System.Drawing.Point(0, 3);
            this._treeDialogs.Name = "_treeDialogs";
            this._treeDialogs.Size = new System.Drawing.Size(248, 252);
            this._treeDialogs.TabIndex = 0;
            this._treeDialogs.AfterLabelEdit += new System.Windows.Forms.NodeLabelEditEventHandler(this.TreeDialogsAfterLabelEdit);
            this._treeDialogs.Enter += new System.EventHandler(this.TreeDialogsEnter);
            this._treeDialogs.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.TreeDialogsAfterSelect);
            this._treeDialogs.NodeMouseClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.TreeDialogsNodeMouseClick);
            this._treeDialogs.BeforeLabelEdit += new System.Windows.Forms.NodeLabelEditEventHandler(this.TreeDialogsBeforeLabelEdit);
            // 
            // _controlTree
            // 
            this._controlTree.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._controlTree.Dialog = null;
            this._controlTree.Enabled = false;
            this._controlTree.Location = new System.Drawing.Point(0, 3);
            this._controlTree.Name = "_controlTree";
            this._controlTree.Size = new System.Drawing.Size(285, 251);
            this._controlTree.TabIndex = 0;
            this._controlTree.SelectedLinkChanged += new System.EventHandler(this.ControlTreeSelectedLinkChanged);
            // 
            // _cmDialog
            // 
            this._cmDialog.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._cmDialogDelete,
            this._cmDialogPlay,
            this._cmDialogProperties});
            this._cmDialog.Name = "_cmDialogs";
            this._cmDialog.Size = new System.Drawing.Size(128, 70);
            // 
            // _cmDialogDelete
            // 
            this._cmDialogDelete.Image = global::DialogDesigner.Properties.Resources.message_delete;
            this._cmDialogDelete.Name = "_cmDialogDelete";
            this._cmDialogDelete.Size = new System.Drawing.Size(127, 22);
            this._cmDialogDelete.Text = "Delete";
            this._cmDialogDelete.Click += new System.EventHandler(this.CmDialogDeleteClick);
            // 
            // _cmDialogPlay
            // 
            this._cmDialogPlay.Image = global::DialogDesigner.Properties.Resources.media_play;
            this._cmDialogPlay.Name = "_cmDialogPlay";
            this._cmDialogPlay.Size = new System.Drawing.Size(127, 22);
            this._cmDialogPlay.Text = "Play";
            this._cmDialogPlay.Click += new System.EventHandler(this.CmDialogPlayClick);
            // 
            // _cmDialogProperties
            // 
            this._cmDialogProperties.Name = "_cmDialogProperties";
            this._cmDialogProperties.Size = new System.Drawing.Size(127, 22);
            this._cmDialogProperties.Text = "Properties";
            this._cmDialogProperties.Click += new System.EventHandler(this.CmDialogPropertiesClick);
            // 
            // _splitContainerH
            // 
            this._splitContainerH.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._splitContainerH.Location = new System.Drawing.Point(0, 28);
            this._splitContainerH.Name = "_splitContainerH";
            this._splitContainerH.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // _splitContainerH.Panel1
            // 
            this._splitContainerH.Panel1.Controls.Add(this._splitContainerV);
            // 
            // _splitContainerH.Panel2
            // 
            this._splitContainerH.Panel2.Controls.Add(this._controlDialogNode);
            this._splitContainerH.Size = new System.Drawing.Size(540, 402);
            this._splitContainerH.SplitterDistance = 255;
            this._splitContainerH.TabIndex = 0;
            // 
            // _controlDialogNode
            // 
            this._controlDialogNode.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._controlDialogNode.Dialog = null;
            this._controlDialogNode.Link = null;
            this._controlDialogNode.Location = new System.Drawing.Point(3, 1);
            this._controlDialogNode.Name = "_controlDialogNode";
            this._controlDialogNode.Size = new System.Drawing.Size(534, 139);
            this._controlDialogNode.TabIndex = 0;
            // 
            // _toolStrip
            // 
            this._toolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._toolButtonNew,
            this._toolButtonOpen,
            this._toolButtonSave,
            this._toolButtonSaveAs,
            this._toolButtonCharacterEditor});
            this._toolStrip.Location = new System.Drawing.Point(0, 0);
            this._toolStrip.Name = "_toolStrip";
            this._toolStrip.Size = new System.Drawing.Size(540, 25);
            this._toolStrip.TabIndex = 1;
            this._toolStrip.Text = "toolStrip1";
            // 
            // _toolButtonNew
            // 
            this._toolButtonNew.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._toolButtonNew.Image = global::DialogDesigner.Properties.Resources.document_new;
            this._toolButtonNew.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._toolButtonNew.Name = "_toolButtonNew";
            this._toolButtonNew.Size = new System.Drawing.Size(23, 22);
            this._toolButtonNew.Text = "New";
            this._toolButtonNew.Click += new System.EventHandler(this.ToolButtonNewClick);
            // 
            // _toolButtonOpen
            // 
            this._toolButtonOpen.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._toolButtonOpen.Image = global::DialogDesigner.Properties.Resources.folder;
            this._toolButtonOpen.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._toolButtonOpen.Name = "_toolButtonOpen";
            this._toolButtonOpen.Size = new System.Drawing.Size(23, 22);
            this._toolButtonOpen.Text = "Open";
            this._toolButtonOpen.Click += new System.EventHandler(this.ToolButtonOpenClick);
            // 
            // _toolButtonSave
            // 
            this._toolButtonSave.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._toolButtonSave.Image = global::DialogDesigner.Properties.Resources.disk_blue;
            this._toolButtonSave.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._toolButtonSave.Name = "_toolButtonSave";
            this._toolButtonSave.Size = new System.Drawing.Size(23, 22);
            this._toolButtonSave.Text = "Save";
            this._toolButtonSave.Click += new System.EventHandler(this.ToolButtonSaveClick);
            // 
            // _toolButtonSaveAs
            // 
            this._toolButtonSaveAs.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._toolButtonSaveAs.Image = global::DialogDesigner.Properties.Resources.disk_blue_edit;
            this._toolButtonSaveAs.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._toolButtonSaveAs.Name = "_toolButtonSaveAs";
            this._toolButtonSaveAs.Size = new System.Drawing.Size(23, 22);
            this._toolButtonSaveAs.Text = "Save as...";
            this._toolButtonSaveAs.Click += new System.EventHandler(this.ToolButtonSaveAsClick);
            // 
            // _toolButtonCharacterEditor
            // 
            this._toolButtonCharacterEditor.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._toolButtonCharacterEditor.Image = global::DialogDesigner.Properties.Resources.knight2;
            this._toolButtonCharacterEditor.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._toolButtonCharacterEditor.Name = "_toolButtonCharacterEditor";
            this._toolButtonCharacterEditor.Size = new System.Drawing.Size(23, 22);
            this._toolButtonCharacterEditor.Text = "Edit characters";
            this._toolButtonCharacterEditor.Visible = false;
            this._toolButtonCharacterEditor.Click += new System.EventHandler(this.ToolButtonCharacterEditorClick);
            // 
            // _cmFolder
            // 
            this._cmFolder.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._cmFolderMap,
            this._cmFolderAddDialog,
            this._cmFolderAddFolder,
            this._cmFolderDelete});
            this._cmFolder.Name = "_cmFolder";
            this._cmFolder.Size = new System.Drawing.Size(133, 92);
            // 
            // _cmFolderMap
            // 
            this._cmFolderMap.Image = global::DialogDesigner.Properties.Resources.folder;
            this._cmFolderMap.Name = "_cmFolderMap";
            this._cmFolderMap.Size = new System.Drawing.Size(132, 22);
            this._cmFolderMap.Text = "Map folder";
            this._cmFolderMap.Click += new System.EventHandler(this.CmFolderMapClick);
            // 
            // _cmFolderAddDialog
            // 
            this._cmFolderAddDialog.Image = global::DialogDesigner.Properties.Resources.message;
            this._cmFolderAddDialog.Name = "_cmFolderAddDialog";
            this._cmFolderAddDialog.Size = new System.Drawing.Size(132, 22);
            this._cmFolderAddDialog.Text = "Add dialog";
            this._cmFolderAddDialog.Click += new System.EventHandler(this.CmFolderAddDialogClick);
            // 
            // _cmFolderAddFolder
            // 
            this._cmFolderAddFolder.Image = global::DialogDesigner.Properties.Resources.folder;
            this._cmFolderAddFolder.Name = "_cmFolderAddFolder";
            this._cmFolderAddFolder.Size = new System.Drawing.Size(132, 22);
            this._cmFolderAddFolder.Text = "Add folder";
            this._cmFolderAddFolder.Click += new System.EventHandler(this.CmFolderAddFolderClick);
            // 
            // _cmFolderDelete
            // 
            this._cmFolderDelete.Image = global::DialogDesigner.Properties.Resources.folder_delete;
            this._cmFolderDelete.Name = "_cmFolderDelete";
            this._cmFolderDelete.Size = new System.Drawing.Size(132, 22);
            this._cmFolderDelete.Text = "Delete";
            this._cmFolderDelete.Click += new System.EventHandler(this.CmFolderDeleteClick);
            // 
            // _bindingDialogLink
            // 
            this._bindingDialogLink.DataSource = typeof(DialogLogic.DialogGraphLink);
            // 
            // _bindingDialog
            // 
            this._bindingDialog.DataSource = typeof(DialogDesigner.DialogObject);
            // 
            // FormDialogEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(540, 430);
            this.Controls.Add(this._toolStrip);
            this.Controls.Add(this._splitContainerH);
            this.Name = "FormDialogEditor";
            this.Text = "Dialog editor";
            this.Load += new System.EventHandler(this.FormDialogEditorLoad);
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.FormDialogEditorClosing);
            this._splitContainerV.Panel1.ResumeLayout(false);
            this._splitContainerV.Panel2.ResumeLayout(false);
            this._splitContainerV.ResumeLayout(false);
            this._cmDialog.ResumeLayout(false);
            this._splitContainerH.Panel1.ResumeLayout(false);
            this._splitContainerH.Panel2.ResumeLayout(false);
            this._splitContainerH.ResumeLayout(false);
            this._toolStrip.ResumeLayout(false);
            this._toolStrip.PerformLayout();
            this._cmFolder.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this._bindingDialogLink)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._bindingDialog)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.SplitContainer _splitContainerV;
        private System.Windows.Forms.SplitContainer _splitContainerH;
        private System.Windows.Forms.ToolStrip _toolStrip;



        private System.Windows.Forms.BindingSource _bindingDialog;
        private System.Windows.Forms.BindingSource _bindingDialogLink;
        private System.Windows.Forms.ToolStripButton _toolButtonNew;
        private System.Windows.Forms.ToolStripButton _toolButtonOpen;
        private System.Windows.Forms.ToolStripButton _toolButtonSave;
        private System.Windows.Forms.ToolStripButton _toolButtonSaveAs;
        private System.Windows.Forms.ContextMenuStrip _cmDialog;
        private ControlDialogTree _controlTree;
        private ControlDialogNodeEditor _controlDialogNode;
        private System.Windows.Forms.ToolStripMenuItem _cmDialogDelete;
        private System.Windows.Forms.ToolStripMenuItem _cmDialogProperties;
        private System.Windows.Forms.ToolStripMenuItem _cmDialogPlay;
        private System.Windows.Forms.TreeView _treeDialogs;
        private System.Windows.Forms.ContextMenuStrip _cmFolder;
        private System.Windows.Forms.ToolStripMenuItem _cmFolderMap;
        private System.Windows.Forms.ToolStripMenuItem _cmFolderAddDialog;
        private System.Windows.Forms.ToolStripMenuItem _cmFolderAddFolder;
        private System.Windows.Forms.ToolStripMenuItem _cmFolderDelete;
        private System.Windows.Forms.ToolStripButton _toolButtonCharacterEditor;


    }
}

