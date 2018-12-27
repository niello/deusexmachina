namespace DialogDesigner
{
    partial class FormPropertyEditor
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
            System.Windows.Forms.Label label2;
            this._comboBoxPropType = new System.Windows.Forms.ComboBox();
            this._textDefaultValue = new System.Windows.Forms.TextBox();
            this._checkBoxDefaultValue = new System.Windows.Forms.CheckBox();
            this._textPropertyName = new System.Windows.Forms.TextBox();
            this._buttonCancel = new System.Windows.Forms.Button();
            this._buttonOk = new System.Windows.Forms.Button();
            label1 = new System.Windows.Forms.Label();
            label2 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new System.Drawing.Point(12, 49);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(69, 13);
            label1.TabIndex = 1;
            label1.Text = "Property type";
            // 
            // label2
            // 
            label2.AutoSize = true;
            label2.Location = new System.Drawing.Point(12, 15);
            label2.Name = "label2";
            label2.Size = new System.Drawing.Size(75, 13);
            label2.TabIndex = 7;
            label2.Text = "Property name";
            // 
            // _comboBoxPropType
            // 
            this._comboBoxPropType.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._comboBoxPropType.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._comboBoxPropType.FormattingEnabled = true;
            this._comboBoxPropType.Location = new System.Drawing.Point(110, 46);
            this._comboBoxPropType.Name = "_comboBoxPropType";
            this._comboBoxPropType.Size = new System.Drawing.Size(272, 21);
            this._comboBoxPropType.TabIndex = 0;
            this._comboBoxPropType.SelectedValueChanged += new System.EventHandler(this.ComboBoxPropTypeSelectedValueChanged);
            // 
            // _textDefaultValue
            // 
            this._textDefaultValue.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._textDefaultValue.Location = new System.Drawing.Point(110, 81);
            this._textDefaultValue.Name = "_textDefaultValue";
            this._textDefaultValue.Size = new System.Drawing.Size(272, 20);
            this._textDefaultValue.TabIndex = 4;
            // 
            // _checkBoxDefaultValue
            // 
            this._checkBoxDefaultValue.AutoSize = true;
            this._checkBoxDefaultValue.Location = new System.Drawing.Point(15, 83);
            this._checkBoxDefaultValue.Name = "_checkBoxDefaultValue";
            this._checkBoxDefaultValue.Size = new System.Drawing.Size(89, 17);
            this._checkBoxDefaultValue.TabIndex = 5;
            this._checkBoxDefaultValue.Text = "Default value";
            this._checkBoxDefaultValue.UseVisualStyleBackColor = true;
            // 
            // _textPropertyName
            // 
            this._textPropertyName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._textPropertyName.Location = new System.Drawing.Point(110, 12);
            this._textPropertyName.Name = "_textPropertyName";
            this._textPropertyName.Size = new System.Drawing.Size(272, 20);
            this._textPropertyName.TabIndex = 6;
            this._textPropertyName.TextChanged += new System.EventHandler(this.TextPropertyNameTextChanged);
            // 
            // _buttonCancel
            // 
            this._buttonCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._buttonCancel.Location = new System.Drawing.Point(307, 119);
            this._buttonCancel.Name = "_buttonCancel";
            this._buttonCancel.Size = new System.Drawing.Size(75, 23);
            this._buttonCancel.TabIndex = 8;
            this._buttonCancel.Text = "Cancel";
            this._buttonCancel.UseVisualStyleBackColor = true;
            this._buttonCancel.Click += new System.EventHandler(this.ButtonCancelClick);
            // 
            // _buttonOk
            // 
            this._buttonOk.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._buttonOk.Location = new System.Drawing.Point(226, 119);
            this._buttonOk.Name = "_buttonOk";
            this._buttonOk.Size = new System.Drawing.Size(75, 23);
            this._buttonOk.TabIndex = 9;
            this._buttonOk.Text = "OK";
            this._buttonOk.UseVisualStyleBackColor = true;
            this._buttonOk.Click += new System.EventHandler(this.ButtonOkClick);
            // 
            // FormPropertyEditor
            // 
            this.AcceptButton = this._buttonOk;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this._buttonCancel;
            this.ClientSize = new System.Drawing.Size(394, 154);
            this.Controls.Add(this._buttonOk);
            this.Controls.Add(this._buttonCancel);
            this.Controls.Add(label2);
            this.Controls.Add(this._textPropertyName);
            this.Controls.Add(this._checkBoxDefaultValue);
            this.Controls.Add(this._textDefaultValue);
            this.Controls.Add(label1);
            this.Controls.Add(this._comboBoxPropType);
            this.Name = "FormPropertyEditor";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "FormPropertyEditor";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ComboBox _comboBoxPropType;
        private System.Windows.Forms.TextBox _textDefaultValue;
        private System.Windows.Forms.CheckBox _checkBoxDefaultValue;
        private System.Windows.Forms.TextBox _textPropertyName;
        private System.Windows.Forms.Button _buttonCancel;
        private System.Windows.Forms.Button _buttonOk;
    }
}