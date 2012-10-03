namespace CreatorIDE
{
    partial class Matrix44EditorCtl
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
            this.bApply = new System.Windows.Forms.Button();
            this.tcModes = new System.Windows.Forms.TabControl();
            this.tpTF = new System.Windows.Forms.TabPage();
            this.label3 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.tfPosZ = new System.Windows.Forms.NumericUpDown();
            this.tfPosY = new System.Windows.Forms.NumericUpDown();
            this.tfPosX = new System.Windows.Forms.NumericUpDown();
            this.tfSZ = new System.Windows.Forms.NumericUpDown();
            this.tfSY = new System.Windows.Forms.NumericUpDown();
            this.tfSX = new System.Windows.Forms.NumericUpDown();
            this.tpMatrix = new System.Windows.Forms.TabPage();
            this.m33 = new System.Windows.Forms.NumericUpDown();
            this.m32 = new System.Windows.Forms.NumericUpDown();
            this.m31 = new System.Windows.Forms.NumericUpDown();
            this.m30 = new System.Windows.Forms.NumericUpDown();
            this.m23 = new System.Windows.Forms.NumericUpDown();
            this.m22 = new System.Windows.Forms.NumericUpDown();
            this.m21 = new System.Windows.Forms.NumericUpDown();
            this.m20 = new System.Windows.Forms.NumericUpDown();
            this.m13 = new System.Windows.Forms.NumericUpDown();
            this.m12 = new System.Windows.Forms.NumericUpDown();
            this.m11 = new System.Windows.Forms.NumericUpDown();
            this.m10 = new System.Windows.Forms.NumericUpDown();
            this.m03 = new System.Windows.Forms.NumericUpDown();
            this.m02 = new System.Windows.Forms.NumericUpDown();
            this.m01 = new System.Windows.Forms.NumericUpDown();
            this.m00 = new System.Windows.Forms.NumericUpDown();
            this.bCancel = new System.Windows.Forms.Button();
            this.tfAngle = new System.Windows.Forms.NumericUpDown();
            this.tfAxisZ = new System.Windows.Forms.NumericUpDown();
            this.tfAxisY = new System.Windows.Forms.NumericUpDown();
            this.tfAxisX = new System.Windows.Forms.NumericUpDown();
            this.tcModes.SuspendLayout();
            this.tpTF.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.tfPosZ)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfPosY)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfPosX)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfSZ)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfSY)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfSX)).BeginInit();
            this.tpMatrix.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.m33)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.m32)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.m31)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.m30)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.m23)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.m22)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.m21)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.m20)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.m13)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.m12)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.m11)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.m10)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.m03)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.m02)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.m01)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.m00)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfAngle)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfAxisZ)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfAxisY)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfAxisX)).BeginInit();
            this.SuspendLayout();
            // 
            // bApply
            // 
            this.bApply.Location = new System.Drawing.Point(3, 153);
            this.bApply.Name = "bApply";
            this.bApply.Size = new System.Drawing.Size(129, 23);
            this.bApply.TabIndex = 0;
            this.bApply.Text = "Применить";
            this.bApply.UseVisualStyleBackColor = true;
            this.bApply.Click += new System.EventHandler(this.bApply_Click);
            // 
            // tcModes
            // 
            this.tcModes.Controls.Add(this.tpTF);
            this.tcModes.Controls.Add(this.tpMatrix);
            this.tcModes.Dock = System.Windows.Forms.DockStyle.Top;
            this.tcModes.Location = new System.Drawing.Point(0, 0);
            this.tcModes.Name = "tcModes";
            this.tcModes.SelectedIndex = 0;
            this.tcModes.Size = new System.Drawing.Size(275, 151);
            this.tcModes.TabIndex = 1;
            this.tcModes.SelectedIndexChanged += new System.EventHandler(this.tcModes_SelectedIndexChanged);
            // 
            // tpTF
            // 
            this.tpTF.Controls.Add(this.tfAngle);
            this.tpTF.Controls.Add(this.tfAxisZ);
            this.tpTF.Controls.Add(this.tfAxisY);
            this.tpTF.Controls.Add(this.tfAxisX);
            this.tpTF.Controls.Add(this.label3);
            this.tpTF.Controls.Add(this.label5);
            this.tpTF.Controls.Add(this.label2);
            this.tpTF.Controls.Add(this.label1);
            this.tpTF.Controls.Add(this.tfPosZ);
            this.tpTF.Controls.Add(this.tfPosY);
            this.tpTF.Controls.Add(this.tfPosX);
            this.tpTF.Controls.Add(this.tfSZ);
            this.tpTF.Controls.Add(this.tfSY);
            this.tpTF.Controls.Add(this.tfSX);
            this.tpTF.Location = new System.Drawing.Point(4, 22);
            this.tpTF.Name = "tpTF";
            this.tpTF.Padding = new System.Windows.Forms.Padding(3);
            this.tpTF.Size = new System.Drawing.Size(267, 125);
            this.tpTF.TabIndex = 0;
            this.tpTF.Text = "Трансформация";
            this.tpTF.UseVisualStyleBackColor = true;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(199, 42);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(70, 13);
            this.label3.TabIndex = 30;
            this.label3.Text = "Угол (град.):";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(6, 81);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(126, 13);
            this.label5.TabIndex = 29;
            this.label5.Text = "Позиция (метры x, y, z):";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(6, 42);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(120, 13);
            this.label2.TabIndex = 26;
            this.label2.Text = "Ось вращения (x, y, z):";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(6, 3);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(92, 13);
            this.label1.TabIndex = 25;
            this.label1.Text = "Масштаб (x, y, z):";
            // 
            // tfPosZ
            // 
            this.tfPosZ.DecimalPlaces = 5;
            this.tfPosZ.Location = new System.Drawing.Point(138, 97);
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
            this.tfPosZ.TabIndex = 21;
            this.tfPosZ.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // tfPosY
            // 
            this.tfPosY.DecimalPlaces = 5;
            this.tfPosY.Location = new System.Drawing.Point(72, 97);
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
            this.tfPosY.TabIndex = 20;
            this.tfPosY.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // tfPosX
            // 
            this.tfPosX.DecimalPlaces = 5;
            this.tfPosX.Location = new System.Drawing.Point(6, 97);
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
            this.tfPosX.TabIndex = 19;
            this.tfPosX.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // tfSZ
            // 
            this.tfSZ.DecimalPlaces = 5;
            this.tfSZ.Location = new System.Drawing.Point(138, 19);
            this.tfSZ.Maximum = new decimal(new int[] {
            1000000,
            0,
            0,
            0});
            this.tfSZ.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            458752});
            this.tfSZ.Name = "tfSZ";
            this.tfSZ.Size = new System.Drawing.Size(60, 20);
            this.tfSZ.TabIndex = 18;
            this.tfSZ.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.tfSZ.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // tfSY
            // 
            this.tfSY.DecimalPlaces = 5;
            this.tfSY.Location = new System.Drawing.Point(72, 19);
            this.tfSY.Maximum = new decimal(new int[] {
            1000000,
            0,
            0,
            0});
            this.tfSY.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            458752});
            this.tfSY.Name = "tfSY";
            this.tfSY.Size = new System.Drawing.Size(60, 20);
            this.tfSY.TabIndex = 17;
            this.tfSY.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.tfSY.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // tfSX
            // 
            this.tfSX.DecimalPlaces = 5;
            this.tfSX.Location = new System.Drawing.Point(6, 19);
            this.tfSX.Maximum = new decimal(new int[] {
            1000000,
            0,
            0,
            0});
            this.tfSX.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            458752});
            this.tfSX.Name = "tfSX";
            this.tfSX.Size = new System.Drawing.Size(60, 20);
            this.tfSX.TabIndex = 16;
            this.tfSX.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.tfSX.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // tpMatrix
            // 
            this.tpMatrix.Controls.Add(this.m33);
            this.tpMatrix.Controls.Add(this.m32);
            this.tpMatrix.Controls.Add(this.m31);
            this.tpMatrix.Controls.Add(this.m30);
            this.tpMatrix.Controls.Add(this.m23);
            this.tpMatrix.Controls.Add(this.m22);
            this.tpMatrix.Controls.Add(this.m21);
            this.tpMatrix.Controls.Add(this.m20);
            this.tpMatrix.Controls.Add(this.m13);
            this.tpMatrix.Controls.Add(this.m12);
            this.tpMatrix.Controls.Add(this.m11);
            this.tpMatrix.Controls.Add(this.m10);
            this.tpMatrix.Controls.Add(this.m03);
            this.tpMatrix.Controls.Add(this.m02);
            this.tpMatrix.Controls.Add(this.m01);
            this.tpMatrix.Controls.Add(this.m00);
            this.tpMatrix.Location = new System.Drawing.Point(4, 22);
            this.tpMatrix.Name = "tpMatrix";
            this.tpMatrix.Padding = new System.Windows.Forms.Padding(3);
            this.tpMatrix.Size = new System.Drawing.Size(267, 125);
            this.tpMatrix.TabIndex = 1;
            this.tpMatrix.Text = "Матрица";
            this.tpMatrix.UseVisualStyleBackColor = true;
            // 
            // m33
            // 
            this.m33.DecimalPlaces = 5;
            this.m33.Location = new System.Drawing.Point(204, 92);
            this.m33.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.m33.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.m33.Name = "m33";
            this.m33.Size = new System.Drawing.Size(60, 20);
            this.m33.TabIndex = 16;
            this.m33.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // m32
            // 
            this.m32.DecimalPlaces = 5;
            this.m32.Location = new System.Drawing.Point(138, 92);
            this.m32.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.m32.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.m32.Name = "m32";
            this.m32.Size = new System.Drawing.Size(60, 20);
            this.m32.TabIndex = 15;
            this.m32.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // m31
            // 
            this.m31.DecimalPlaces = 5;
            this.m31.Location = new System.Drawing.Point(72, 92);
            this.m31.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.m31.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.m31.Name = "m31";
            this.m31.Size = new System.Drawing.Size(60, 20);
            this.m31.TabIndex = 14;
            this.m31.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // m30
            // 
            this.m30.DecimalPlaces = 5;
            this.m30.Location = new System.Drawing.Point(6, 92);
            this.m30.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.m30.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.m30.Name = "m30";
            this.m30.Size = new System.Drawing.Size(60, 20);
            this.m30.TabIndex = 13;
            this.m30.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // m23
            // 
            this.m23.DecimalPlaces = 5;
            this.m23.Location = new System.Drawing.Point(204, 66);
            this.m23.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.m23.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.m23.Name = "m23";
            this.m23.Size = new System.Drawing.Size(60, 20);
            this.m23.TabIndex = 12;
            this.m23.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // m22
            // 
            this.m22.DecimalPlaces = 5;
            this.m22.Location = new System.Drawing.Point(138, 66);
            this.m22.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.m22.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.m22.Name = "m22";
            this.m22.Size = new System.Drawing.Size(60, 20);
            this.m22.TabIndex = 11;
            this.m22.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // m21
            // 
            this.m21.DecimalPlaces = 5;
            this.m21.Location = new System.Drawing.Point(72, 66);
            this.m21.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.m21.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.m21.Name = "m21";
            this.m21.Size = new System.Drawing.Size(60, 20);
            this.m21.TabIndex = 10;
            this.m21.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // m20
            // 
            this.m20.DecimalPlaces = 5;
            this.m20.Location = new System.Drawing.Point(6, 66);
            this.m20.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.m20.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.m20.Name = "m20";
            this.m20.Size = new System.Drawing.Size(60, 20);
            this.m20.TabIndex = 9;
            this.m20.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // m13
            // 
            this.m13.DecimalPlaces = 5;
            this.m13.Location = new System.Drawing.Point(204, 40);
            this.m13.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.m13.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.m13.Name = "m13";
            this.m13.Size = new System.Drawing.Size(60, 20);
            this.m13.TabIndex = 8;
            this.m13.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // m12
            // 
            this.m12.DecimalPlaces = 5;
            this.m12.Location = new System.Drawing.Point(138, 40);
            this.m12.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.m12.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.m12.Name = "m12";
            this.m12.Size = new System.Drawing.Size(60, 20);
            this.m12.TabIndex = 7;
            this.m12.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // m11
            // 
            this.m11.DecimalPlaces = 5;
            this.m11.Location = new System.Drawing.Point(72, 40);
            this.m11.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.m11.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.m11.Name = "m11";
            this.m11.Size = new System.Drawing.Size(60, 20);
            this.m11.TabIndex = 6;
            this.m11.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // m10
            // 
            this.m10.DecimalPlaces = 5;
            this.m10.Location = new System.Drawing.Point(6, 40);
            this.m10.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.m10.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.m10.Name = "m10";
            this.m10.Size = new System.Drawing.Size(60, 20);
            this.m10.TabIndex = 5;
            this.m10.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // m03
            // 
            this.m03.DecimalPlaces = 5;
            this.m03.Location = new System.Drawing.Point(204, 14);
            this.m03.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.m03.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.m03.Name = "m03";
            this.m03.Size = new System.Drawing.Size(60, 20);
            this.m03.TabIndex = 4;
            this.m03.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // m02
            // 
            this.m02.DecimalPlaces = 5;
            this.m02.Location = new System.Drawing.Point(138, 14);
            this.m02.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.m02.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.m02.Name = "m02";
            this.m02.Size = new System.Drawing.Size(60, 20);
            this.m02.TabIndex = 3;
            this.m02.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // m01
            // 
            this.m01.DecimalPlaces = 5;
            this.m01.Location = new System.Drawing.Point(72, 14);
            this.m01.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.m01.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.m01.Name = "m01";
            this.m01.Size = new System.Drawing.Size(60, 20);
            this.m01.TabIndex = 2;
            this.m01.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // m00
            // 
            this.m00.DecimalPlaces = 5;
            this.m00.Location = new System.Drawing.Point(6, 14);
            this.m00.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.m00.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.m00.Name = "m00";
            this.m00.Size = new System.Drawing.Size(60, 20);
            this.m00.TabIndex = 1;
            this.m00.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // bCancel
            // 
            this.bCancel.Location = new System.Drawing.Point(146, 153);
            this.bCancel.Name = "bCancel";
            this.bCancel.Size = new System.Drawing.Size(126, 23);
            this.bCancel.TabIndex = 2;
            this.bCancel.Text = "Отмена";
            this.bCancel.UseVisualStyleBackColor = true;
            this.bCancel.Click += new System.EventHandler(this.bCancel_Click);
            // 
            // tfAngle
            // 
            this.tfAngle.DecimalPlaces = 5;
            this.tfAngle.Location = new System.Drawing.Point(204, 58);
            this.tfAngle.Maximum = new decimal(new int[] {
            360,
            0,
            0,
            0});
            this.tfAngle.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            -2147483648});
            this.tfAngle.Name = "tfAngle";
            this.tfAngle.Size = new System.Drawing.Size(60, 20);
            this.tfAngle.TabIndex = 34;
            this.tfAngle.ValueChanged += new System.EventHandler(this.DegreeNumUpDown_ValueChanged);
            // 
            // tfAxisZ
            // 
            this.tfAxisZ.DecimalPlaces = 5;
            this.tfAxisZ.Location = new System.Drawing.Point(138, 58);
            this.tfAxisZ.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.tfAxisZ.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.tfAxisZ.Name = "tfAxisZ";
            this.tfAxisZ.Size = new System.Drawing.Size(60, 20);
            this.tfAxisZ.TabIndex = 33;
            this.tfAxisZ.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // tfAxisY
            // 
            this.tfAxisY.DecimalPlaces = 5;
            this.tfAxisY.Location = new System.Drawing.Point(72, 58);
            this.tfAxisY.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.tfAxisY.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.tfAxisY.Name = "tfAxisY";
            this.tfAxisY.Size = new System.Drawing.Size(60, 20);
            this.tfAxisY.TabIndex = 32;
            this.tfAxisY.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // tfAxisX
            // 
            this.tfAxisX.DecimalPlaces = 5;
            this.tfAxisX.Location = new System.Drawing.Point(6, 58);
            this.tfAxisX.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
            this.tfAxisX.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
            this.tfAxisX.Name = "tfAxisX";
            this.tfAxisX.Size = new System.Drawing.Size(60, 20);
            this.tfAxisX.TabIndex = 31;
            this.tfAxisX.ValueChanged += new System.EventHandler(this.NumUpDown_ValueChanged);
            // 
            // Matrix44EditorCtl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.bCancel);
            this.Controls.Add(this.tcModes);
            this.Controls.Add(this.bApply);
            this.Name = "Matrix44EditorCtl";
            this.Size = new System.Drawing.Size(275, 179);
            this.tcModes.ResumeLayout(false);
            this.tpTF.ResumeLayout(false);
            this.tpTF.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.tfPosZ)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfPosY)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfPosX)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfSZ)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfSY)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfSX)).EndInit();
            this.tpMatrix.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.m33)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.m32)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.m31)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.m30)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.m23)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.m22)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.m21)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.m20)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.m13)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.m12)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.m11)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.m10)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.m03)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.m02)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.m01)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.m00)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfAngle)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfAxisZ)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfAxisY)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.tfAxisX)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button bApply;
        private System.Windows.Forms.TabControl tcModes;
        private System.Windows.Forms.TabPage tpTF;
        private System.Windows.Forms.TabPage tpMatrix;
        private System.Windows.Forms.Button bCancel;
        private System.Windows.Forms.NumericUpDown m33;
        private System.Windows.Forms.NumericUpDown m32;
        private System.Windows.Forms.NumericUpDown m31;
        private System.Windows.Forms.NumericUpDown m30;
        private System.Windows.Forms.NumericUpDown m23;
        private System.Windows.Forms.NumericUpDown m22;
        private System.Windows.Forms.NumericUpDown m21;
        private System.Windows.Forms.NumericUpDown m20;
        private System.Windows.Forms.NumericUpDown m13;
        private System.Windows.Forms.NumericUpDown m12;
        private System.Windows.Forms.NumericUpDown m11;
        private System.Windows.Forms.NumericUpDown m10;
        private System.Windows.Forms.NumericUpDown m03;
        private System.Windows.Forms.NumericUpDown m02;
        private System.Windows.Forms.NumericUpDown m01;
        private System.Windows.Forms.NumericUpDown m00;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.NumericUpDown tfPosZ;
        private System.Windows.Forms.NumericUpDown tfPosY;
        private System.Windows.Forms.NumericUpDown tfPosX;
        private System.Windows.Forms.NumericUpDown tfSZ;
        private System.Windows.Forms.NumericUpDown tfSY;
        private System.Windows.Forms.NumericUpDown tfSX;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.NumericUpDown tfAngle;
        private System.Windows.Forms.NumericUpDown tfAxisZ;
        private System.Windows.Forms.NumericUpDown tfAxisY;
        private System.Windows.Forms.NumericUpDown tfAxisX;
    }
}
