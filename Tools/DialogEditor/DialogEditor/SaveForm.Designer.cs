namespace DialogDesigner
{
    partial class SaveForm
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
            System.Windows.Forms.Label label1;
            System.Windows.Forms.Button _buttonCanel;
            System.Windows.Forms.Button _buttonNo;
            System.Windows.Forms.Button _buttonYes;
            this._tree = new System.Windows.Forms.TreeView();
            label1 = new System.Windows.Forms.Label();
            _buttonCanel = new System.Windows.Forms.Button();
            _buttonNo = new System.Windows.Forms.Button();
            _buttonYes = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new System.Drawing.Point(12, 9);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(371, 13);
            label1.TabIndex = 0;
            label1.Text = "The following dialogs has been changed. Do you want to save this changes?";
            // 
            // _buttonCanel
            // 
            _buttonCanel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            _buttonCanel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            _buttonCanel.Location = new System.Drawing.Point(316, 327);
            _buttonCanel.Name = "_buttonCanel";
            _buttonCanel.Size = new System.Drawing.Size(75, 23);
            _buttonCanel.TabIndex = 4;
            _buttonCanel.Text = "Cancel";
            _buttonCanel.UseVisualStyleBackColor = true;
            // 
            // _buttonNo
            // 
            _buttonNo.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            _buttonNo.Location = new System.Drawing.Point(235, 327);
            _buttonNo.Name = "_buttonNo";
            _buttonNo.Size = new System.Drawing.Size(75, 23);
            _buttonNo.TabIndex = 3;
            _buttonNo.Text = "No";
            _buttonNo.UseVisualStyleBackColor = true;
            _buttonNo.Click += new System.EventHandler(this.ButtonNoClick);
            // 
            // _buttonYes
            // 
            _buttonYes.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            _buttonYes.Location = new System.Drawing.Point(154, 327);
            _buttonYes.Name = "_buttonYes";
            _buttonYes.Size = new System.Drawing.Size(75, 23);
            _buttonYes.TabIndex = 2;
            _buttonYes.Text = "Yes";
            _buttonYes.UseVisualStyleBackColor = true;
            _buttonYes.Click += new System.EventHandler(this.ButtonYesClick);
            // 
            // _tree
            // 
            this._tree.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._tree.CheckBoxes = true;
            this._tree.Location = new System.Drawing.Point(15, 34);
            this._tree.Name = "_tree";
            this._tree.Size = new System.Drawing.Size(376, 287);
            this._tree.TabIndex = 1;
            this._tree.AfterCheck += new System.Windows.Forms.TreeViewEventHandler(this.TreeAfterCheck);
            // 
            // SaveForm
            // 
            this.AcceptButton = _buttonYes;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = _buttonCanel;
            this.ClientSize = new System.Drawing.Size(403, 362);
            this.Controls.Add(_buttonYes);
            this.Controls.Add(_buttonNo);
            this.Controls.Add(_buttonCanel);
            this.Controls.Add(this._tree);
            this.Controls.Add(label1);
            this.MinimizeBox = false;
            this.MinimumSize = new System.Drawing.Size(400, 400);
            this.Name = "SaveForm";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Save changes";
            this.Load += new System.EventHandler(this.SaveFormLoad);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TreeView _tree;
    }
}