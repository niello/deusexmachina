namespace CreatorIDE
{
    partial class NewCategoryWnd
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
			this.lbProperties = new System.Windows.Forms.CheckedListBox();
			this.cbCppClass = new System.Windows.Forms.ComboBox();
			this.tName = new System.Windows.Forms.TextBox();
			this.label1 = new System.Windows.Forms.Label();
			this.label2 = new System.Windows.Forms.Label();
			this.label3 = new System.Windows.Forms.Label();
			this.label4 = new System.Windows.Forms.Label();
			this.tTplTable = new System.Windows.Forms.TextBox();
			this.label5 = new System.Windows.Forms.Label();
			this.tInstTable = new System.Windows.Forms.TextBox();
			this.bCreate = new System.Windows.Forms.Button();
			this.bCancel = new System.Windows.Forms.Button();
			this.SuspendLayout();
			// 
			// lbProperties
			// 
			this.lbProperties.FormattingEnabled = true;
			this.lbProperties.Location = new System.Drawing.Point(12, 66);
			this.lbProperties.Name = "lbProperties";
			this.lbProperties.Size = new System.Drawing.Size(260, 184);
			this.lbProperties.TabIndex = 0;
			// 
			// cbCppClass
			// 
			this.cbCppClass.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.cbCppClass.FormattingEnabled = true;
			this.cbCppClass.Location = new System.Drawing.Point(151, 24);
			this.cbCppClass.Name = "cbCppClass";
			this.cbCppClass.Size = new System.Drawing.Size(121, 21);
			this.cbCppClass.TabIndex = 1;
			// 
			// tName
			// 
			this.tName.Location = new System.Drawing.Point(12, 24);
			this.tName.Name = "tName";
			this.tName.Size = new System.Drawing.Size(133, 20);
			this.tName.TabIndex = 2;
			this.tName.TextChanged += new System.EventHandler(this.tName_TextChanged);
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(13, 5);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(87, 13);
			this.label1.TabIndex = 3;
			this.label1.Text = "Имя категории:";
			// 
			// label2
			// 
			this.label2.AutoSize = true;
			this.label2.Location = new System.Drawing.Point(148, 5);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(72, 13);
			this.label2.TabIndex = 4;
			this.label2.Text = "Класс в С++:";
			// 
			// label3
			// 
			this.label3.AutoSize = true;
			this.label3.Location = new System.Drawing.Point(12, 47);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(58, 13);
			this.label3.TabIndex = 5;
			this.label3.Text = "Свойства:";
			// 
			// label4
			// 
			this.label4.AutoSize = true;
			this.label4.Location = new System.Drawing.Point(13, 254);
			this.label4.Name = "label4";
			this.label4.Size = new System.Drawing.Size(106, 13);
			this.label4.TabIndex = 7;
			this.label4.Text = "Таблица шаблонов:";
			// 
			// tTplTable
			// 
			this.tTplTable.Location = new System.Drawing.Point(12, 273);
			this.tTplTable.Name = "tTplTable";
			this.tTplTable.Size = new System.Drawing.Size(260, 20);
			this.tTplTable.TabIndex = 6;
			// 
			// label5
			// 
			this.label5.AutoSize = true;
			this.label5.Location = new System.Drawing.Point(13, 296);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(124, 13);
			this.label5.TabIndex = 9;
			this.label5.Text = "Таблица экземпляров:";
			// 
			// tInstTable
			// 
			this.tInstTable.Location = new System.Drawing.Point(12, 315);
			this.tInstTable.Name = "tInstTable";
			this.tInstTable.Size = new System.Drawing.Size(260, 20);
			this.tInstTable.TabIndex = 8;
			// 
			// bCreate
			// 
			this.bCreate.Location = new System.Drawing.Point(12, 342);
			this.bCreate.Name = "bCreate";
			this.bCreate.Size = new System.Drawing.Size(75, 23);
			this.bCreate.TabIndex = 10;
			this.bCreate.Text = "Создать";
			this.bCreate.UseVisualStyleBackColor = true;
			this.bCreate.Click += new System.EventHandler(this.bCreate_Click);
			// 
			// bCancel
			// 
			this.bCancel.Location = new System.Drawing.Point(197, 342);
			this.bCancel.Name = "bCancel";
			this.bCancel.Size = new System.Drawing.Size(75, 23);
			this.bCancel.TabIndex = 11;
			this.bCancel.Text = "Отмена";
			this.bCancel.UseVisualStyleBackColor = true;
			this.bCancel.Click += new System.EventHandler(this.bCancel_Click);
			// 
			// NewCategoryWnd
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(284, 371);
			this.Controls.Add(this.bCancel);
			this.Controls.Add(this.bCreate);
			this.Controls.Add(this.label5);
			this.Controls.Add(this.tInstTable);
			this.Controls.Add(this.label4);
			this.Controls.Add(this.tTplTable);
			this.Controls.Add(this.label3);
			this.Controls.Add(this.label2);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.tName);
			this.Controls.Add(this.cbCppClass);
			this.Controls.Add(this.lbProperties);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
			this.Name = "NewCategoryWnd";
			this.Text = "Новая категория объектов";
			this.Shown += new System.EventHandler(this.NewCategoryWnd_Shown);
			this.ResumeLayout(false);
			this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.CheckedListBox lbProperties;
        private System.Windows.Forms.ComboBox cbCppClass;
        private System.Windows.Forms.TextBox tName;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox tTplTable;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox tInstTable;
        private System.Windows.Forms.Button bCreate;
        private System.Windows.Forms.Button bCancel;
    }
}