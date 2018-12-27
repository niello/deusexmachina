namespace CreatorIDE
{
	partial class NewLocationWnd
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
			this.label1 = new System.Windows.Forms.Label();
			this.tID = new System.Windows.Forms.TextBox();
			this.label2 = new System.Windows.Forms.Label();
			this.tName = new System.Windows.Forms.TextBox();
			this.bCancel = new System.Windows.Forms.Button();
			this.bCreate = new System.Windows.Forms.Button();
			this.label5 = new System.Windows.Forms.Label();
			this.tfPosZ = new System.Windows.Forms.NumericUpDown();
			this.tfPosY = new System.Windows.Forms.NumericUpDown();
			this.tfPosX = new System.Windows.Forms.NumericUpDown();
			this.label3 = new System.Windows.Forms.Label();
			this.tfSizeZ = new System.Windows.Forms.NumericUpDown();
			this.tfSizeY = new System.Windows.Forms.NumericUpDown();
			this.tfSizeX = new System.Windows.Forms.NumericUpDown();
			this.label4 = new System.Windows.Forms.Label();
			this.tNavMesh = new System.Windows.Forms.TextBox();
			((System.ComponentModel.ISupportInitialize)(this.tfPosZ)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.tfPosY)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.tfPosX)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.tfSizeZ)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.tfSizeY)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.tfSizeX)).BeginInit();
			this.SuspendLayout();
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(12, 9);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(21, 13);
			this.label1.TabIndex = 5;
			this.label1.Text = "ID:";
			// 
			// tID
			// 
			this.tID.Location = new System.Drawing.Point(11, 28);
			this.tID.Name = "tID";
			this.tID.Size = new System.Drawing.Size(208, 20);
			this.tID.TabIndex = 4;
			// 
			// label2
			// 
			this.label2.AutoSize = true;
			this.label2.Location = new System.Drawing.Point(12, 51);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(60, 13);
			this.label2.TabIndex = 7;
			this.label2.Text = "Название:";
			// 
			// tName
			// 
			this.tName.Location = new System.Drawing.Point(11, 70);
			this.tName.Name = "tName";
			this.tName.Size = new System.Drawing.Size(208, 20);
			this.tName.TabIndex = 6;
			// 
			// bCancel
			// 
			this.bCancel.Location = new System.Drawing.Point(144, 220);
			this.bCancel.Name = "bCancel";
			this.bCancel.Size = new System.Drawing.Size(75, 23);
			this.bCancel.TabIndex = 13;
			this.bCancel.Text = "Отмена";
			this.bCancel.UseVisualStyleBackColor = true;
			this.bCancel.Click += new System.EventHandler(this.bCancel_Click);
			// 
			// bCreate
			// 
			this.bCreate.Location = new System.Drawing.Point(11, 220);
			this.bCreate.Name = "bCreate";
			this.bCreate.Size = new System.Drawing.Size(75, 23);
			this.bCreate.TabIndex = 12;
			this.bCreate.Text = "Создать";
			this.bCreate.UseVisualStyleBackColor = true;
			this.bCreate.Click += new System.EventHandler(this.bCreate_Click);
			// 
			// label5
			// 
			this.label5.AutoSize = true;
			this.label5.Location = new System.Drawing.Point(11, 97);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(113, 13);
			this.label5.TabIndex = 33;
			this.label5.Text = "Центр (метры x, y, z):";
			// 
			// tfPosZ
			// 
			this.tfPosZ.DecimalPlaces = 5;
			this.tfPosZ.Location = new System.Drawing.Point(143, 113);
			this.tfPosZ.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
			this.tfPosZ.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
			this.tfPosZ.Name = "tfPosZ";
			this.tfPosZ.Size = new System.Drawing.Size(60, 20);
			this.tfPosZ.TabIndex = 32;
			// 
			// tfPosY
			// 
			this.tfPosY.DecimalPlaces = 5;
			this.tfPosY.Location = new System.Drawing.Point(77, 113);
			this.tfPosY.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
			this.tfPosY.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
			this.tfPosY.Name = "tfPosY";
			this.tfPosY.Size = new System.Drawing.Size(60, 20);
			this.tfPosY.TabIndex = 31;
			// 
			// tfPosX
			// 
			this.tfPosX.DecimalPlaces = 5;
			this.tfPosX.Location = new System.Drawing.Point(11, 113);
			this.tfPosX.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
			this.tfPosX.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
			this.tfPosX.Name = "tfPosX";
			this.tfPosX.Size = new System.Drawing.Size(60, 20);
			this.tfPosX.TabIndex = 30;
			// 
			// label3
			// 
			this.label3.AutoSize = true;
			this.label3.Location = new System.Drawing.Point(12, 136);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(129, 13);
			this.label3.TabIndex = 37;
			this.label3.Text = "Размеры (метры x, y, z):";
			// 
			// tfSizeZ
			// 
			this.tfSizeZ.DecimalPlaces = 5;
			this.tfSizeZ.Location = new System.Drawing.Point(144, 152);
			this.tfSizeZ.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
			this.tfSizeZ.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
			this.tfSizeZ.Name = "tfSizeZ";
			this.tfSizeZ.Size = new System.Drawing.Size(60, 20);
			this.tfSizeZ.TabIndex = 36;
			this.tfSizeZ.Value = new decimal(new int[] {
            1000,
            0,
            0,
            0});
			// 
			// tfSizeY
			// 
			this.tfSizeY.DecimalPlaces = 5;
			this.tfSizeY.Location = new System.Drawing.Point(78, 152);
			this.tfSizeY.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
			this.tfSizeY.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
			this.tfSizeY.Name = "tfSizeY";
			this.tfSizeY.Size = new System.Drawing.Size(60, 20);
			this.tfSizeY.TabIndex = 35;
			this.tfSizeY.Value = new decimal(new int[] {
            200,
            0,
            0,
            0});
			// 
			// tfSizeX
			// 
			this.tfSizeX.DecimalPlaces = 5;
			this.tfSizeX.Location = new System.Drawing.Point(12, 152);
			this.tfSizeX.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
			this.tfSizeX.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
			this.tfSizeX.Name = "tfSizeX";
			this.tfSizeX.Size = new System.Drawing.Size(60, 20);
			this.tfSizeX.TabIndex = 34;
			this.tfSizeX.Value = new decimal(new int[] {
            1000,
            0,
            0,
            0});
			// 
			// label4
			// 
			this.label4.AutoSize = true;
			this.label4.Location = new System.Drawing.Point(12, 175);
			this.label4.Name = "label4";
			this.label4.Size = new System.Drawing.Size(158, 13);
			this.label4.TabIndex = 39;
			this.label4.Text = "Ресурс навигационной сетки:";
			// 
			// tNavMesh
			// 
			this.tNavMesh.Location = new System.Drawing.Point(11, 194);
			this.tNavMesh.Name = "tNavMesh";
			this.tNavMesh.Size = new System.Drawing.Size(208, 20);
			this.tNavMesh.TabIndex = 38;
			// 
			// NewLocationWnd
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(230, 250);
			this.Controls.Add(this.label4);
			this.Controls.Add(this.tNavMesh);
			this.Controls.Add(this.label3);
			this.Controls.Add(this.tfSizeZ);
			this.Controls.Add(this.tfSizeY);
			this.Controls.Add(this.tfSizeX);
			this.Controls.Add(this.label5);
			this.Controls.Add(this.tfPosZ);
			this.Controls.Add(this.tfPosY);
			this.Controls.Add(this.tfPosX);
			this.Controls.Add(this.bCancel);
			this.Controls.Add(this.bCreate);
			this.Controls.Add(this.label2);
			this.Controls.Add(this.tName);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.tID);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
			this.Name = "NewLocationWnd";
			this.Text = "Новая локация";
			((System.ComponentModel.ISupportInitialize)(this.tfPosZ)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.tfPosY)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.tfPosX)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.tfSizeZ)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.tfSizeY)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.tfSizeX)).EndInit();
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.TextBox tID;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.TextBox tName;
		private System.Windows.Forms.Button bCancel;
		private System.Windows.Forms.Button bCreate;
		private System.Windows.Forms.Label label5;
		private System.Windows.Forms.NumericUpDown tfPosZ;
		private System.Windows.Forms.NumericUpDown tfPosY;
		private System.Windows.Forms.NumericUpDown tfPosX;
		private System.Windows.Forms.Label label3;
		private System.Windows.Forms.NumericUpDown tfSizeZ;
		private System.Windows.Forms.NumericUpDown tfSizeY;
		private System.Windows.Forms.NumericUpDown tfSizeX;
		private System.Windows.Forms.Label label4;
		private System.Windows.Forms.TextBox tNavMesh;
	}
}