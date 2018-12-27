namespace CreatorIDE
{
    partial class ResourceSelectorWnd
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
            this.bCancel = new System.Windows.Forms.Button();
            this.bApply = new System.Windows.Forms.Button();
            this.tvFS = new System.Windows.Forms.TreeView();
            this.bClear = new System.Windows.Forms.Button();
            this.tPreview = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            // 
            // bCancel
            // 
            this.bCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.bCancel.Location = new System.Drawing.Point(91, 350);
            this.bCancel.Name = "bCancel";
            this.bCancel.Size = new System.Drawing.Size(73, 23);
            this.bCancel.TabIndex = 7;
            this.bCancel.Text = "Отмена";
            this.bCancel.UseVisualStyleBackColor = true;
            this.bCancel.Click += new System.EventHandler(this.bCancel_Click);
            // 
            // bApply
            // 
            this.bApply.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.bApply.Enabled = false;
            this.bApply.Location = new System.Drawing.Point(12, 350);
            this.bApply.Name = "bApply";
            this.bApply.Size = new System.Drawing.Size(73, 23);
            this.bApply.TabIndex = 6;
            this.bApply.Text = "Применить";
            this.bApply.UseVisualStyleBackColor = true;
            this.bApply.Click += new System.EventHandler(this.bApply_Click);
            // 
            // tvFS
            // 
            this.tvFS.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)));
            this.tvFS.HideSelection = false;
            this.tvFS.Location = new System.Drawing.Point(12, 12);
            this.tvFS.Name = "tvFS";
            this.tvFS.PathSeparator = "/";
            this.tvFS.Size = new System.Drawing.Size(231, 332);
            this.tvFS.TabIndex = 5;
            this.tvFS.NodeMouseDoubleClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.tvFS_NodeMouseDoubleClick);
            this.tvFS.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.tvFS_AfterSelect);
            // 
            // bClear
            // 
            this.bClear.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.bClear.Location = new System.Drawing.Point(170, 350);
            this.bClear.Name = "bClear";
            this.bClear.Size = new System.Drawing.Size(73, 23);
            this.bClear.TabIndex = 8;
            this.bClear.Text = "Очистить";
            this.bClear.UseVisualStyleBackColor = true;
            this.bClear.Click += new System.EventHandler(this.bClear_Click);
            // 
            // tPreview
            // 
            this.tPreview.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.tPreview.Font = new System.Drawing.Font("Courier New", 10F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.tPreview.Location = new System.Drawing.Point(249, 12);
            this.tPreview.Multiline = true;
            this.tPreview.Name = "tPreview";
            this.tPreview.ReadOnly = true;
            this.tPreview.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.tPreview.Size = new System.Drawing.Size(673, 361);
            this.tPreview.TabIndex = 9;
            // 
            // ResourceSelectorWnd
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(934, 377);
            this.Controls.Add(this.tPreview);
            this.Controls.Add(this.bClear);
            this.Controls.Add(this.bCancel);
            this.Controls.Add(this.bApply);
            this.Controls.Add(this.tvFS);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.SizableToolWindow;
            this.Name = "ResourceSelectorWnd";
            this.Text = "Выберите ресурс";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button bCancel;
        private System.Windows.Forms.Button bApply;
        private System.Windows.Forms.TreeView tvFS;
        private System.Windows.Forms.Button bClear;
        private System.Windows.Forms.TextBox tPreview;
    }
}