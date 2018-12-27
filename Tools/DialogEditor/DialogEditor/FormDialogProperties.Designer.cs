namespace DialogDesigner
{
    partial class FormDialogProperties
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
            System.Windows.Forms.Label label1;
            System.Windows.Forms.Label label2;
            System.Windows.Forms.Label label3;
            this._comboCharacters = new System.Windows.Forms.ComboBox();
            this._buttonAdd = new System.Windows.Forms.Button();
            this._buttonCancel = new System.Windows.Forms.Button();
            this._textDialogName = new System.Windows.Forms.TextBox();
            this._buttonOk = new System.Windows.Forms.Button();
            this._listCharacters = new System.Windows.Forms.ListView();
            this._cmViewCharacter = new System.Windows.Forms.ContextMenuStrip(this.components);
            this._cmCharacterItemDelete = new System.Windows.Forms.ToolStripMenuItem();
            this._textScriptFile = new System.Windows.Forms.TextBox();
            this._buttonScriptBrowse = new System.Windows.Forms.Button();
            label1 = new System.Windows.Forms.Label();
            label2 = new System.Windows.Forms.Label();
            label3 = new System.Windows.Forms.Label();
            this._cmViewCharacter.SuspendLayout();
            this.SuspendLayout();
            // 
            // _comboCharacters
            // 
            this._comboCharacters.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._comboCharacters.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.Suggest;
            this._comboCharacters.FormattingEnabled = true;
            this._comboCharacters.Location = new System.Drawing.Point(16, 366);
            this._comboCharacters.Name = "_comboCharacters";
            this._comboCharacters.Size = new System.Drawing.Size(383, 21);
            this._comboCharacters.TabIndex = 0;
            // 
            // _buttonAdd
            // 
            this._buttonAdd.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._buttonAdd.Location = new System.Drawing.Point(405, 364);
            this._buttonAdd.Name = "_buttonAdd";
            this._buttonAdd.Size = new System.Drawing.Size(75, 23);
            this._buttonAdd.TabIndex = 2;
            this._buttonAdd.Text = "Add";
            this._buttonAdd.UseVisualStyleBackColor = true;
            this._buttonAdd.Click += new System.EventHandler(this.ButtonAddClick);
            // 
            // _buttonCancel
            // 
            this._buttonCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._buttonCancel.Location = new System.Drawing.Point(405, 438);
            this._buttonCancel.Name = "_buttonCancel";
            this._buttonCancel.Size = new System.Drawing.Size(75, 23);
            this._buttonCancel.TabIndex = 3;
            this._buttonCancel.Text = "Cancel";
            this._buttonCancel.UseVisualStyleBackColor = true;
            this._buttonCancel.Click += new System.EventHandler(this.ButtonCancelClick);
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new System.Drawing.Point(13, 13);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(66, 13);
            label1.TabIndex = 4;
            label1.Text = "Dialog name";
            // 
            // _textDialogName
            // 
            this._textDialogName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._textDialogName.Location = new System.Drawing.Point(85, 10);
            this._textDialogName.Name = "_textDialogName";
            this._textDialogName.ReadOnly = true;
            this._textDialogName.Size = new System.Drawing.Size(395, 20);
            this._textDialogName.TabIndex = 5;
            // 
            // label2
            // 
            label2.AutoSize = true;
            label2.Location = new System.Drawing.Point(13, 39);
            label2.Name = "label2";
            label2.Size = new System.Drawing.Size(58, 13);
            label2.TabIndex = 6;
            label2.Text = "Characters";
            // 
            // _buttonOk
            // 
            this._buttonOk.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._buttonOk.Location = new System.Drawing.Point(324, 438);
            this._buttonOk.Name = "_buttonOk";
            this._buttonOk.Size = new System.Drawing.Size(75, 23);
            this._buttonOk.TabIndex = 7;
            this._buttonOk.Text = "OK";
            this._buttonOk.UseVisualStyleBackColor = true;
            this._buttonOk.Click += new System.EventHandler(this.ButtonOkClick);
            // 
            // _listCharacters
            // 
            this._listCharacters.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._listCharacters.ContextMenuStrip = this._cmViewCharacter;
            this._listCharacters.GridLines = true;
            this._listCharacters.Location = new System.Drawing.Point(16, 55);
            this._listCharacters.Name = "_listCharacters";
            this._listCharacters.Size = new System.Drawing.Size(464, 295);
            this._listCharacters.TabIndex = 8;
            this._listCharacters.UseCompatibleStateImageBehavior = false;
            this._listCharacters.View = System.Windows.Forms.View.List;
            this._listCharacters.MouseClick += new System.Windows.Forms.MouseEventHandler(this.ListCharactersMouseClick);
            // 
            // _cmViewCharacter
            // 
            this._cmViewCharacter.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._cmCharacterItemDelete});
            this._cmViewCharacter.Name = "_cmViewCharacter";
            this._cmViewCharacter.Size = new System.Drawing.Size(108, 26);
            // 
            // _cmCharacterItemDelete
            // 
            this._cmCharacterItemDelete.Name = "_cmCharacterItemDelete";
            this._cmCharacterItemDelete.Size = new System.Drawing.Size(107, 22);
            this._cmCharacterItemDelete.Text = "Delete";
            this._cmCharacterItemDelete.Click += new System.EventHandler(this.CmCharacterItemDeleteClick);
            // 
            // label3
            // 
            label3.AutoSize = true;
            label3.Location = new System.Drawing.Point(13, 406);
            label3.Name = "label3";
            label3.Size = new System.Drawing.Size(50, 13);
            label3.TabIndex = 9;
            label3.Text = "Script file";
            // 
            // _textScriptFile
            // 
            this._textScriptFile.Location = new System.Drawing.Point(69, 403);
            this._textScriptFile.Name = "_textScriptFile";
            this._textScriptFile.ReadOnly = true;
            this._textScriptFile.Size = new System.Drawing.Size(330, 20);
            this._textScriptFile.TabIndex = 10;
            // 
            // _buttonScriptBrowse
            // 
            this._buttonScriptBrowse.Location = new System.Drawing.Point(405, 401);
            this._buttonScriptBrowse.Name = "_buttonScriptBrowse";
            this._buttonScriptBrowse.Size = new System.Drawing.Size(75, 23);
            this._buttonScriptBrowse.TabIndex = 11;
            this._buttonScriptBrowse.Text = "Browse...";
            this._buttonScriptBrowse.UseVisualStyleBackColor = true;
            this._buttonScriptBrowse.Click += new System.EventHandler(this.ButtonScriptBrowseClick);
            // 
            // FormDialogProperties
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(492, 473);
            this.Controls.Add(this._buttonScriptBrowse);
            this.Controls.Add(this._textScriptFile);
            this.Controls.Add(label3);
            this.Controls.Add(this._listCharacters);
            this.Controls.Add(this._buttonOk);
            this.Controls.Add(label2);
            this.Controls.Add(this._textDialogName);
            this.Controls.Add(label1);
            this.Controls.Add(this._buttonCancel);
            this.Controls.Add(this._buttonAdd);
            this.Controls.Add(this._comboCharacters);
            this.Name = "FormDialogProperties";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Dialog properties";
            this._cmViewCharacter.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ComboBox _comboCharacters;
        private System.Windows.Forms.Button _buttonAdd;
        private System.Windows.Forms.Button _buttonCancel;
        private System.Windows.Forms.TextBox _textDialogName;
        private System.Windows.Forms.Button _buttonOk;
        private System.Windows.Forms.ListView _listCharacters;
        private System.Windows.Forms.ContextMenuStrip _cmViewCharacter;
        private System.Windows.Forms.ToolStripMenuItem _cmCharacterItemDelete;
        private System.Windows.Forms.TextBox _textScriptFile;
        private System.Windows.Forms.Button _buttonScriptBrowse;
    }
}