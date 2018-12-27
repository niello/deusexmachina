namespace DialogDesigner
{
    partial class ControlDialogTree
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

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this._treeDialog = new System.Windows.Forms.TreeView();
            this._cmTree = new System.Windows.Forms.ContextMenuStrip(this.components);
            this._cmTreeAdd = new System.Windows.Forms.ToolStripMenuItem();
            this._cmTreeAddEmpty = new System.Windows.Forms.ToolStripMenuItem();
            this._cmTreeAddAnswer = new System.Windows.Forms.ToolStripMenuItem();
            this._cmTreeAddLinkToEnd = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripMenuItem1 = new System.Windows.Forms.ToolStripSeparator();
            this._cmTreeReplace = new System.Windows.Forms.ToolStripMenuItem();
            this._cmTreeCopyAsLink = new System.Windows.Forms.ToolStripMenuItem();
            this._cmTreeCut = new System.Windows.Forms.ToolStripMenuItem();
            this._cmTreePaste = new System.Windows.Forms.ToolStripMenuItem();
            this._cmTreeDelete = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripMenuItem2 = new System.Windows.Forms.ToolStripSeparator();
            this._cmTreePlay = new System.Windows.Forms.ToolStripMenuItem();
            this._cmTree.SuspendLayout();
            this.SuspendLayout();
            // 
            // _treeDialog
            // 
            this._treeDialog.ContextMenuStrip = this._cmTree;
            this._treeDialog.Dock = System.Windows.Forms.DockStyle.Fill;
            this._treeDialog.HideSelection = false;
            this._treeDialog.Location = new System.Drawing.Point(0, 0);
            this._treeDialog.Name = "_treeDialog";
            this._treeDialog.Size = new System.Drawing.Size(211, 210);
            this._treeDialog.TabIndex = 0;
            this._treeDialog.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.TreeDialogAfterSelect);
            this._treeDialog.NodeMouseClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.TreeDialogNodeMouseClick);
            this._treeDialog.BeforeSelect += new System.Windows.Forms.TreeViewCancelEventHandler(this.TreeDialogBeforeSelect);
            // 
            // _cmTree
            // 
            this._cmTree.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._cmTreeAdd,
            this._cmTreeAddEmpty,
            this._cmTreeAddAnswer,
            this._cmTreeAddLinkToEnd,
            this.toolStripMenuItem1,
            this._cmTreeReplace,
            this._cmTreeCopyAsLink,
            this._cmTreeCut,
            this._cmTreePaste,
            this._cmTreeDelete,
            this.toolStripMenuItem2,
            this._cmTreePlay});
            this._cmTree.Name = "_cmTree";
            this._cmTree.Size = new System.Drawing.Size(167, 236);
            this._cmTree.Opening += new System.ComponentModel.CancelEventHandler(this.CmTreeOpening);
            // 
            // _cmTreeAdd
            // 
            this._cmTreeAdd.Image = global::DialogDesigner.Properties.Resources.element;
            this._cmTreeAdd.Name = "_cmTreeAdd";
            this._cmTreeAdd.Size = new System.Drawing.Size(166, 22);
            this._cmTreeAdd.Text = "Add phrase";
            this._cmTreeAdd.Click += new System.EventHandler(this.CmTreeAddClick);
            // 
            // _cmTreeAddEmpty
            // 
            this._cmTreeAddEmpty.Image = global::DialogDesigner.Properties.Resources.element_gray;
            this._cmTreeAddEmpty.Name = "_cmTreeAddEmpty";
            this._cmTreeAddEmpty.Size = new System.Drawing.Size(166, 22);
            this._cmTreeAddEmpty.Text = "Add empty node";
            this._cmTreeAddEmpty.Click += new System.EventHandler(this.CmTreeAddEmptyClick);
            // 
            // _cmTreeAddAnswer
            // 
            this._cmTreeAddAnswer.Image = global::DialogDesigner.Properties.Resources.elements1;
            this._cmTreeAddAnswer.Name = "_cmTreeAddAnswer";
            this._cmTreeAddAnswer.Size = new System.Drawing.Size(166, 22);
            this._cmTreeAddAnswer.Text = "Add answer node";
            this._cmTreeAddAnswer.Click += new System.EventHandler(this.CmTreeAddAnswerClick);
            // 
            // _cmTreeAddLinkToEnd
            // 
            this._cmTreeAddLinkToEnd.Image = global::DialogDesigner.Properties.Resources.element_stop;
            this._cmTreeAddLinkToEnd.Name = "_cmTreeAddLinkToEnd";
            this._cmTreeAddLinkToEnd.Size = new System.Drawing.Size(166, 22);
            this._cmTreeAddLinkToEnd.Text = "Add link to \"End\"";
            this._cmTreeAddLinkToEnd.Click += new System.EventHandler(this.CmTreeAddLinkToEndClick);
            // 
            // toolStripMenuItem1
            // 
            this.toolStripMenuItem1.Name = "toolStripMenuItem1";
            this.toolStripMenuItem1.Size = new System.Drawing.Size(163, 6);
            // 
            // _cmTreeReplace
            // 
            this._cmTreeReplace.Image = global::DialogDesigner.Properties.Resources.element_replace;
            this._cmTreeReplace.Name = "_cmTreeReplace";
            this._cmTreeReplace.Size = new System.Drawing.Size(166, 22);
            this._cmTreeReplace.Text = "Replace";
            this._cmTreeReplace.Click += new System.EventHandler(this.CmTreeReplaceClick);
            // 
            // _cmTreeCopyAsLink
            // 
            this._cmTreeCopyAsLink.Image = global::DialogDesigner.Properties.Resources.element_copy;
            this._cmTreeCopyAsLink.Name = "_cmTreeCopyAsLink";
            this._cmTreeCopyAsLink.Size = new System.Drawing.Size(166, 22);
            this._cmTreeCopyAsLink.Text = "Copy";
            this._cmTreeCopyAsLink.Click += new System.EventHandler(this.CmTreeCopyAsLinkClick);
            // 
            // _cmTreeCut
            // 
            this._cmTreeCut.Image = global::DialogDesigner.Properties.Resources.element_cut;
            this._cmTreeCut.Name = "_cmTreeCut";
            this._cmTreeCut.Size = new System.Drawing.Size(166, 22);
            this._cmTreeCut.Text = "Cut";
            this._cmTreeCut.Click += new System.EventHandler(this.CmTreeCutClick);
            // 
            // _cmTreePaste
            // 
            this._cmTreePaste.Image = global::DialogDesigner.Properties.Resources.element_down;
            this._cmTreePaste.Name = "_cmTreePaste";
            this._cmTreePaste.Size = new System.Drawing.Size(166, 22);
            this._cmTreePaste.Text = "Paste";
            this._cmTreePaste.Click += new System.EventHandler(this.CmTreePasteClick);
            // 
            // _cmTreeDelete
            // 
            this._cmTreeDelete.Image = global::DialogDesigner.Properties.Resources.element_delete;
            this._cmTreeDelete.Name = "_cmTreeDelete";
            this._cmTreeDelete.Size = new System.Drawing.Size(166, 22);
            this._cmTreeDelete.Text = "Delete";
            this._cmTreeDelete.Click += new System.EventHandler(this.CmTreeDeleteClick);
            // 
            // toolStripMenuItem2
            // 
            this.toolStripMenuItem2.Name = "toolStripMenuItem2";
            this.toolStripMenuItem2.Size = new System.Drawing.Size(163, 6);
            // 
            // _cmTreePlay
            // 
            this._cmTreePlay.Image = global::DialogDesigner.Properties.Resources.media_play;
            this._cmTreePlay.Name = "_cmTreePlay";
            this._cmTreePlay.Size = new System.Drawing.Size(166, 22);
            this._cmTreePlay.Text = "Play";
            this._cmTreePlay.Click += new System.EventHandler(this.CmTreePlayClick);
            // 
            // ControlDialogTree
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._treeDialog);
            this.Name = "ControlDialogTree";
            this.Size = new System.Drawing.Size(211, 210);
            this._cmTree.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TreeView _treeDialog;
        private System.Windows.Forms.ContextMenuStrip _cmTree;
        private System.Windows.Forms.ToolStripMenuItem _cmTreeAdd;
        private System.Windows.Forms.ToolStripMenuItem _cmTreeAddEmpty;
        private System.Windows.Forms.ToolStripMenuItem _cmTreeCopyAsLink;
        private System.Windows.Forms.ToolStripMenuItem _cmTreeCut;
        private System.Windows.Forms.ToolStripMenuItem _cmTreePaste;
        private System.Windows.Forms.ToolStripSeparator toolStripMenuItem1;
        private System.Windows.Forms.ToolStripMenuItem _cmTreeDelete;
        private System.Windows.Forms.ToolStripSeparator toolStripMenuItem2;
        private System.Windows.Forms.ToolStripMenuItem _cmTreePlay;
        private System.Windows.Forms.ToolStripMenuItem _cmTreeAddAnswer;
        private System.Windows.Forms.ToolStripMenuItem _cmTreeReplace;
        private System.Windows.Forms.ToolStripMenuItem _cmTreeAddLinkToEnd;
    }
}
