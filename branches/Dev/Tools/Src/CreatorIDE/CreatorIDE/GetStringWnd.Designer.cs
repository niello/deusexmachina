namespace CreatorIDE
{
    partial class GetStringWnd
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
            this._bOK = new System.Windows.Forms.Button();
            this._bCancel = new System.Windows.Forms.Button();
            this.tStr = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            // 
            // _bOK
            // 
            this._bOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._bOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            this._bOK.Location = new System.Drawing.Point(116, 39);
            this._bOK.Name = "_bOK";
            this._bOK.Size = new System.Drawing.Size(75, 23);
            this._bOK.TabIndex = 0;
            this._bOK.Text = "ОК";
            this._bOK.UseVisualStyleBackColor = true;
            this._bOK.Click += new System.EventHandler(this.OnOkButtonClick);
            // 
            // _bCancel
            // 
            this._bCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._bCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._bCancel.Location = new System.Drawing.Point(197, 39);
            this._bCancel.Name = "_bCancel";
            this._bCancel.Size = new System.Drawing.Size(75, 23);
            this._bCancel.TabIndex = 1;
            this._bCancel.Text = "Отмена";
            this._bCancel.UseVisualStyleBackColor = true;
            this._bCancel.Click += new System.EventHandler(this.OnCancelButtonClick);
            // 
            // tStr
            // 
            this.tStr.Location = new System.Drawing.Point(13, 13);
            this.tStr.MaxLength = 64;
            this.tStr.Name = "tStr";
            this.tStr.Size = new System.Drawing.Size(259, 20);
            this.tStr.TabIndex = 2;
            // 
            // GetStringWnd
            // 
            this.AcceptButton = this._bOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this._bCancel;
            this.ClientSize = new System.Drawing.Size(284, 74);
            this.Controls.Add(this.tStr);
            this.Controls.Add(this._bCancel);
            this.Controls.Add(this._bOK);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
            this.Name = "GetStringWnd";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Введите название";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button _bOK;
        private System.Windows.Forms.Button _bCancel;
        private System.Windows.Forms.TextBox tStr;
    }
}