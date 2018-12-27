namespace DialogDesigner
{
    partial class FormCharactersProperties
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
            this._propertyGrid = new System.Windows.Forms.PropertyGrid();
            this._comboBoxCharacters = new System.Windows.Forms.ComboBox();
            this._buttonOk = new System.Windows.Forms.Button();
            this._buttonAddProperty = new System.Windows.Forms.Button();
            this._buttonAddCharacter = new System.Windows.Forms.Button();
            label1 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new System.Drawing.Point(9, 15);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(53, 13);
            label1.TabIndex = 5;
            label1.Text = "Character";
            // 
            // _propertyGrid
            // 
            this._propertyGrid.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._propertyGrid.Location = new System.Drawing.Point(12, 39);
            this._propertyGrid.Name = "_propertyGrid";
            this._propertyGrid.Size = new System.Drawing.Size(409, 248);
            this._propertyGrid.TabIndex = 0;
            // 
            // _comboBoxCharacters
            // 
            this._comboBoxCharacters.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._comboBoxCharacters.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.Suggest;
            this._comboBoxCharacters.FormattingEnabled = true;
            this._comboBoxCharacters.Location = new System.Drawing.Point(68, 12);
            this._comboBoxCharacters.Name = "_comboBoxCharacters";
            this._comboBoxCharacters.Size = new System.Drawing.Size(251, 21);
            this._comboBoxCharacters.TabIndex = 1;
            this._comboBoxCharacters.SelectedIndexChanged += new System.EventHandler(this.ComboBoxCharactersSelectedIndexChanged);
            // 
            // _buttonOk
            // 
            this._buttonOk.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._buttonOk.Location = new System.Drawing.Point(346, 330);
            this._buttonOk.Name = "_buttonOk";
            this._buttonOk.Size = new System.Drawing.Size(75, 23);
            this._buttonOk.TabIndex = 2;
            this._buttonOk.Text = "OK";
            this._buttonOk.UseVisualStyleBackColor = true;
            this._buttonOk.Click += new System.EventHandler(this.ButtonOkClick);
            // 
            // _buttonAddProperty
            // 
            this._buttonAddProperty.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._buttonAddProperty.Location = new System.Drawing.Point(325, 293);
            this._buttonAddProperty.Name = "_buttonAddProperty";
            this._buttonAddProperty.Size = new System.Drawing.Size(96, 23);
            this._buttonAddProperty.TabIndex = 3;
            this._buttonAddProperty.Text = "Add property";
            this._buttonAddProperty.UseVisualStyleBackColor = true;
            this._buttonAddProperty.Click += new System.EventHandler(ButtonAddPropertyClick);
            // 
            // _buttonAddCharacter
            // 
            this._buttonAddCharacter.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._buttonAddCharacter.Location = new System.Drawing.Point(325, 10);
            this._buttonAddCharacter.Name = "_buttonAddCharacter";
            this._buttonAddCharacter.Size = new System.Drawing.Size(96, 23);
            this._buttonAddCharacter.TabIndex = 4;
            this._buttonAddCharacter.Text = "Add character";
            this._buttonAddCharacter.UseVisualStyleBackColor = true;
            this._buttonAddCharacter.Click += new System.EventHandler(this.ButtonAddCharacterClick);
            // 
            // FormCharactersProperties
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(433, 365);
            this.Controls.Add(label1);
            this.Controls.Add(this._buttonAddCharacter);
            this.Controls.Add(this._buttonAddProperty);
            this.Controls.Add(this._buttonOk);
            this.Controls.Add(this._comboBoxCharacters);
            this.Controls.Add(this._propertyGrid);
            this.Name = "FormCharactersProperties";
            this.Text = "FormCharactersProperties";
            this.Load += new System.EventHandler(this.FormCharactersPropertiesLoad);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.PropertyGrid _propertyGrid;
        private System.Windows.Forms.ComboBox _comboBoxCharacters;
        private System.Windows.Forms.Button _buttonOk;
        private System.Windows.Forms.Button _buttonAddProperty;
        private System.Windows.Forms.Button _buttonAddCharacter;
    }
}