using DialogLogic;

namespace DialogDesigner
{
    partial class ControlPlayerNode
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
            this._labelCharacter = new System.Windows.Forms.Label();
            this._bindingDialogNode = new System.Windows.Forms.BindingSource(this.components);
            this._richTextPhrase = new System.Windows.Forms.RichTextBox();
            ((System.ComponentModel.ISupportInitialize)(this._bindingDialogNode)).BeginInit();
            this.SuspendLayout();
            // 
            // _labelCharacter
            // 
            this._labelCharacter.AutoSize = true;
            this._labelCharacter.DataBindings.Add(new System.Windows.Forms.Binding("Text", this._bindingDialogNode, "Character", true));
            this._labelCharacter.Dock = System.Windows.Forms.DockStyle.Top;
            this._labelCharacter.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this._labelCharacter.Location = new System.Drawing.Point(0, 0);
            this._labelCharacter.Name = "_labelCharacter";
            this._labelCharacter.Size = new System.Drawing.Size(88, 16);
            this._labelCharacter.TabIndex = 0;
            this._labelCharacter.Text = "%character%";
            // 
            // _bindingDialogNode
            // 
            this._bindingDialogNode.DataSource = typeof(DialogLogic.DialogGraphPhraseNodeBase);
            // 
            // _richTextPhrase
            // 
            this._richTextPhrase.DataBindings.Add(new System.Windows.Forms.Binding("Text", this._bindingDialogNode, "Phrase", true));
            this._richTextPhrase.Dock = System.Windows.Forms.DockStyle.Fill;
            this._richTextPhrase.Location = new System.Drawing.Point(0, 16);
            this._richTextPhrase.Name = "_richTextPhrase";
            this._richTextPhrase.ReadOnly = true;
            this._richTextPhrase.Size = new System.Drawing.Size(334, 209);
            this._richTextPhrase.TabIndex = 1;
            this._richTextPhrase.Text = "";
            this._richTextPhrase.MouseEnter += new System.EventHandler(this.RichTextPhraseMouseEnter);
            this._richTextPhrase.MouseLeave += new System.EventHandler(this.RichTextPhraseMouseLeave);
            this._richTextPhrase.Click += new System.EventHandler(this.RichTextPhraseClick);
            // 
            // ControlPlayerNode
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._richTextPhrase);
            this.Controls.Add(this._labelCharacter);
            this.Name = "ControlPlayerNode";
            this.Size = new System.Drawing.Size(334, 225);
            ((System.ComponentModel.ISupportInitialize)(this._bindingDialogNode)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label _labelCharacter;
        private System.Windows.Forms.BindingSource _bindingDialogNode;
        private System.Windows.Forms.RichTextBox _richTextPhrase;
    }
}
