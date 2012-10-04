using System.Windows.Forms;
using DialogLogic;

namespace DialogDesigner
{
    partial class ControlDialogNodeEditor
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
            System.Windows.Forms.Label label1;
            System.Windows.Forms.Label label3;
            this._comboCharacter = new System.Windows.Forms.ComboBox();
            this._bindingDialogNode = new System.Windows.Forms.BindingSource(this.components);
            this._bindingCharacters = new System.Windows.Forms.BindingSource(this.components);
            this._bindingDialog = new System.Windows.Forms.BindingSource(this.components);
            this._richTextPhrase = new System.Windows.Forms.RichTextBox();
            this.label2 = new System.Windows.Forms.Label();
            this._textCondition = new System.Windows.Forms.TextBox();
            this._bindingLink = new System.Windows.Forms.BindingSource(this.components);
            this._buttonOpenScriptFile = new System.Windows.Forms.Button();
            this._textAction = new System.Windows.Forms.TextBox();
            label1 = new System.Windows.Forms.Label();
            label3 = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this._bindingDialogNode)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._bindingCharacters)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._bindingDialog)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._bindingLink)).BeginInit();
            this.SuspendLayout();
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new System.Drawing.Point(3, 6);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(53, 13);
            label1.TabIndex = 0;
            label1.Text = "Character";
            // 
            // label3
            // 
            label3.AutoSize = true;
            label3.Location = new System.Drawing.Point(3, 75);
            label3.Name = "label3";
            label3.Size = new System.Drawing.Size(37, 13);
            label3.TabIndex = 7;
            label3.Text = "Action";
            // 
            // _comboCharacter
            // 
            this._comboCharacter.DataBindings.Add(new System.Windows.Forms.Binding("SelectedValue", this._bindingDialogNode, "Character", true));
            this._comboCharacter.DataSource = this._bindingCharacters;
            this._comboCharacter.DisplayMember = "Name";
            this._comboCharacter.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._comboCharacter.FormattingEnabled = true;
            this._comboCharacter.Location = new System.Drawing.Point(62, 3);
            this._comboCharacter.Name = "_comboCharacter";
            this._comboCharacter.Size = new System.Drawing.Size(154, 21);
            this._comboCharacter.TabIndex = 1;
            this._comboCharacter.ValueMember = "Name";
            // 
            // _bindingDialogNode
            // 
            this._bindingDialogNode.DataSource = typeof(DialogLogic.DialogGraphNodeBase);
            // 
            // _bindingCharacters
            // 
            this._bindingCharacters.DataMember = "DialogCharacters";
            this._bindingCharacters.DataSource = this._bindingDialog;
            // 
            // _bindingDialog
            // 
            this._bindingDialog.DataSource = typeof(DialogLogic.DialogInfo);
            // 
            // _richTextPhrase
            // 
            this._richTextPhrase.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._richTextPhrase.DataBindings.Add(new System.Windows.Forms.Binding("Text", this._bindingDialogNode, "Phrase", true, System.Windows.Forms.DataSourceUpdateMode.OnPropertyChanged));
            this._richTextPhrase.Location = new System.Drawing.Point(0, 98);
            this._richTextPhrase.Name = "_richTextPhrase";
            this._richTextPhrase.Size = new System.Drawing.Size(489, 111);
            this._richTextPhrase.TabIndex = 2;
            this._richTextPhrase.Text = "";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(3, 41);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(51, 13);
            this.label2.TabIndex = 4;
            this.label2.Text = "Condition";
            // 
            // _textCondition
            // 
            this._textCondition.DataBindings.Add(new System.Windows.Forms.Binding("Text", this._bindingLink, "Condition", true));
            this._textCondition.Location = new System.Drawing.Point(62, 38);
            this._textCondition.Name = "_textCondition";
            this._textCondition.Size = new System.Drawing.Size(233, 20);
            this._textCondition.TabIndex = 5;
            // 
            // _bindingLink
            // 
            this._bindingLink.DataSource = typeof(DialogLogic.DialogGraphLink);
            // 
            // _buttonOpenScriptFile
            // 
            this._buttonOpenScriptFile.Location = new System.Drawing.Point(301, 70);
            this._buttonOpenScriptFile.Name = "_buttonOpenScriptFile";
            this._buttonOpenScriptFile.Size = new System.Drawing.Size(85, 23);
            this._buttonOpenScriptFile.TabIndex = 6;
            this._buttonOpenScriptFile.Text = "Open script file";
            this._buttonOpenScriptFile.UseVisualStyleBackColor = true;
            this._buttonOpenScriptFile.Click += new System.EventHandler(this.ButtonOpenScriptFileClick);
            // 
            // _textAction
            // 
            this._textAction.DataBindings.Add(new System.Windows.Forms.Binding("Text", this._bindingLink, "Action", true));
            this._textAction.Location = new System.Drawing.Point(62, 72);
            this._textAction.Name = "_textAction";
            this._textAction.Size = new System.Drawing.Size(233, 20);
            this._textAction.TabIndex = 8;
            // 
            // ControlDialogNodeEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._textAction);
            this.Controls.Add(label3);
            this.Controls.Add(this._buttonOpenScriptFile);
            this.Controls.Add(this._textCondition);
            this.Controls.Add(this.label2);
            this.Controls.Add(this._richTextPhrase);
            this.Controls.Add(this._comboCharacter);
            this.Controls.Add(label1);
            this.Name = "ControlDialogNodeEditor";
            this.Size = new System.Drawing.Size(489, 209);
            ((System.ComponentModel.ISupportInitialize)(this._bindingDialogNode)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._bindingCharacters)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._bindingDialog)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._bindingLink)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ComboBox _comboCharacter;
        private System.Windows.Forms.RichTextBox _richTextPhrase;
        private System.Windows.Forms.BindingSource _bindingDialog;
        private System.Windows.Forms.BindingSource _bindingDialogNode;
        private System.Windows.Forms.BindingSource _bindingCharacters;
        private Label label2;
        private TextBox _textCondition;
        private Button _buttonOpenScriptFile;
        private TextBox _textAction;
        private BindingSource _bindingLink;

    }
}
