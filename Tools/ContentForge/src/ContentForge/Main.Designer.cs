namespace ContentForge
{
    partial class Main
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
			this.tcMain = new System.Windows.Forms.TabControl();
			this.tpModel = new System.Windows.Forms.TabPage();
			this.cbModelInvertTexV = new System.Windows.Forms.CheckBox();
			this.cbModelUniformScale = new System.Windows.Forms.CheckBox();
			this.cbModelNormals = new System.Windows.Forms.CheckBox();
			this.tfRotZ = new System.Windows.Forms.NumericUpDown();
			this.tfRotY = new System.Windows.Forms.NumericUpDown();
			this.tfRotX = new System.Windows.Forms.NumericUpDown();
			this.label16 = new System.Windows.Forms.Label();
			this.label17 = new System.Windows.Forms.Label();
			this.label18 = new System.Windows.Forms.Label();
			this.tfPosZ = new System.Windows.Forms.NumericUpDown();
			this.tfPosY = new System.Windows.Forms.NumericUpDown();
			this.tfPosX = new System.Windows.Forms.NumericUpDown();
			this.tfSZ = new System.Windows.Forms.NumericUpDown();
			this.tfSY = new System.Windows.Forms.NumericUpDown();
			this.tfSX = new System.Windows.Forms.NumericUpDown();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.rbModelTangentsNoSplit = new System.Windows.Forms.RadioButton();
			this.rbModelTangentsSplit = new System.Windows.Forms.RadioButton();
			this.rbModelTangentsNo = new System.Windows.Forms.RadioButton();
			this.cbModelEdges = new System.Windows.Forms.CheckBox();
			this.cbModelClean = new System.Windows.Forms.CheckBox();
			this.bOpenModelSrc = new System.Windows.Forms.Button();
			this.tModelName = new System.Windows.Forms.TextBox();
			this.label15 = new System.Windows.Forms.Label();
			this.bCreateModel = new System.Windows.Forms.Button();
			this.tModelResName = new System.Windows.Forms.TextBox();
			this.label13 = new System.Windows.Forms.Label();
			this.tpTerrain = new System.Windows.Forms.TabPage();
			this.cbPhysHRD = new System.Windows.Forms.CheckBox();
			this.label12 = new System.Windows.Forms.Label();
			this.nChuScale = new System.Windows.Forms.NumericUpDown();
			this.label11 = new System.Windows.Forms.Label();
			this.nTqtDepth = new System.Windows.Forms.NumericUpDown();
			this.label10 = new System.Windows.Forms.Label();
			this.label9 = new System.Windows.Forms.Label();
			this.nChuError = new System.Windows.Forms.NumericUpDown();
			this.label8 = new System.Windows.Forms.Label();
			this.nChuDepth = new System.Windows.Forms.NumericUpDown();
			this.cbAlphaDXT5 = new System.Windows.Forms.CheckBox();
			this.tTexA = new System.Windows.Forms.TextBox();
			this.label7 = new System.Windows.Forms.Label();
			this.tTexB = new System.Windows.Forms.TextBox();
			this.label6 = new System.Windows.Forms.Label();
			this.tTexG = new System.Windows.Forms.TextBox();
			this.label5 = new System.Windows.Forms.Label();
			this.tTexR = new System.Windows.Forms.TextBox();
			this.label4 = new System.Windows.Forms.Label();
			this.bOpenAlpha = new System.Windows.Forms.Button();
			this.tAlphaFName = new System.Windows.Forms.TextBox();
			this.label3 = new System.Windows.Forms.Label();
			this.bCreateTerrain = new System.Windows.Forms.Button();
			this.bOpenBT = new System.Windows.Forms.Button();
			this.tTerrainResName = new System.Windows.Forms.TextBox();
			this.tBTFName = new System.Windows.Forms.TextBox();
			this.label2 = new System.Windows.Forms.Label();
			this.label1 = new System.Windows.Forms.Label();
			this.tpSettings = new System.Windows.Forms.TabPage();
			this.tProjDir = new System.Windows.Forms.TextBox();
			this.label14 = new System.Windows.Forms.Label();
			this.OD = new System.Windows.Forms.OpenFileDialog();
			this.tOutput = new System.Windows.Forms.TextBox();
			this.cbModelGenerateShadow = new System.Windows.Forms.CheckBox();
			this.tcMain.SuspendLayout();
			this.tpModel.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.tfRotZ)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.tfRotY)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.tfRotX)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.tfPosZ)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.tfPosY)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.tfPosX)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.tfSZ)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.tfSY)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.tfSX)).BeginInit();
			this.groupBox1.SuspendLayout();
			this.tpTerrain.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.nChuScale)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.nTqtDepth)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.nChuError)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.nChuDepth)).BeginInit();
			this.tpSettings.SuspendLayout();
			this.SuspendLayout();
			// 
			// tcMain
			// 
			this.tcMain.Controls.Add(this.tpModel);
			this.tcMain.Controls.Add(this.tpTerrain);
			this.tcMain.Controls.Add(this.tpSettings);
			this.tcMain.Dock = System.Windows.Forms.DockStyle.Top;
			this.tcMain.Location = new System.Drawing.Point(0, 0);
			this.tcMain.Name = "tcMain";
			this.tcMain.SelectedIndex = 0;
			this.tcMain.Size = new System.Drawing.Size(685, 311);
			this.tcMain.TabIndex = 0;
			// 
			// tpModel
			// 
			this.tpModel.Controls.Add(this.cbModelGenerateShadow);
			this.tpModel.Controls.Add(this.cbModelInvertTexV);
			this.tpModel.Controls.Add(this.cbModelUniformScale);
			this.tpModel.Controls.Add(this.cbModelNormals);
			this.tpModel.Controls.Add(this.tfRotZ);
			this.tpModel.Controls.Add(this.tfRotY);
			this.tpModel.Controls.Add(this.tfRotX);
			this.tpModel.Controls.Add(this.label16);
			this.tpModel.Controls.Add(this.label17);
			this.tpModel.Controls.Add(this.label18);
			this.tpModel.Controls.Add(this.tfPosZ);
			this.tpModel.Controls.Add(this.tfPosY);
			this.tpModel.Controls.Add(this.tfPosX);
			this.tpModel.Controls.Add(this.tfSZ);
			this.tpModel.Controls.Add(this.tfSY);
			this.tpModel.Controls.Add(this.tfSX);
			this.tpModel.Controls.Add(this.groupBox1);
			this.tpModel.Controls.Add(this.cbModelEdges);
			this.tpModel.Controls.Add(this.cbModelClean);
			this.tpModel.Controls.Add(this.bOpenModelSrc);
			this.tpModel.Controls.Add(this.tModelName);
			this.tpModel.Controls.Add(this.label15);
			this.tpModel.Controls.Add(this.bCreateModel);
			this.tpModel.Controls.Add(this.tModelResName);
			this.tpModel.Controls.Add(this.label13);
			this.tpModel.Location = new System.Drawing.Point(4, 22);
			this.tpModel.Name = "tpModel";
			this.tpModel.Padding = new System.Windows.Forms.Padding(3);
			this.tpModel.Size = new System.Drawing.Size(677, 285);
			this.tpModel.TabIndex = 0;
			this.tpModel.Text = "Модель";
			this.tpModel.UseVisualStyleBackColor = true;
			// 
			// cbModelInvertTexV
			// 
			this.cbModelInvertTexV.AutoSize = true;
			this.cbModelInvertTexV.Location = new System.Drawing.Point(11, 137);
			this.cbModelInvertTexV.Name = "cbModelInvertTexV";
			this.cbModelInvertTexV.Size = new System.Drawing.Size(203, 17);
			this.cbModelInvertTexV.TabIndex = 48;
			this.cbModelInvertTexV.Text = "Инвертировать V-texcoord (Blender)";
			this.cbModelInvertTexV.UseVisualStyleBackColor = true;
			// 
			// cbModelUniformScale
			// 
			this.cbModelUniformScale.AutoSize = true;
			this.cbModelUniformScale.Checked = true;
			this.cbModelUniformScale.CheckState = System.Windows.Forms.CheckState.Checked;
			this.cbModelUniformScale.Location = new System.Drawing.Point(252, 117);
			this.cbModelUniformScale.Name = "cbModelUniformScale";
			this.cbModelUniformScale.Size = new System.Drawing.Size(314, 17);
			this.cbModelUniformScale.TabIndex = 47;
			this.cbModelUniformScale.Text = "Равномерное масштабирование (значение берётся из х)";
			this.cbModelUniformScale.UseVisualStyleBackColor = true;
			// 
			// cbModelNormals
			// 
			this.cbModelNormals.AutoSize = true;
			this.cbModelNormals.Checked = true;
			this.cbModelNormals.CheckState = System.Windows.Forms.CheckState.Checked;
			this.cbModelNormals.Location = new System.Drawing.Point(11, 97);
			this.cbModelNormals.Name = "cbModelNormals";
			this.cbModelNormals.Size = new System.Drawing.Size(208, 17);
			this.cbModelNormals.TabIndex = 46;
			this.cbModelNormals.Text = "Генерировать нормали, если их нет";
			this.cbModelNormals.UseVisualStyleBackColor = true;
			// 
			// tfRotZ
			// 
			this.tfRotZ.DecimalPlaces = 5;
			this.tfRotZ.Location = new System.Drawing.Point(384, 157);
			this.tfRotZ.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
			this.tfRotZ.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
			this.tfRotZ.Name = "tfRotZ";
			this.tfRotZ.Size = new System.Drawing.Size(60, 20);
			this.tfRotZ.TabIndex = 45;
			// 
			// tfRotY
			// 
			this.tfRotY.DecimalPlaces = 5;
			this.tfRotY.Location = new System.Drawing.Point(318, 157);
			this.tfRotY.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
			this.tfRotY.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
			this.tfRotY.Name = "tfRotY";
			this.tfRotY.Size = new System.Drawing.Size(60, 20);
			this.tfRotY.TabIndex = 44;
			// 
			// tfRotX
			// 
			this.tfRotX.DecimalPlaces = 5;
			this.tfRotX.Location = new System.Drawing.Point(252, 157);
			this.tfRotX.Maximum = new decimal(new int[] {
            1410065408,
            2,
            0,
            0});
			this.tfRotX.Minimum = new decimal(new int[] {
            1410065408,
            2,
            0,
            -2147483648});
			this.tfRotX.Name = "tfRotX";
			this.tfRotX.Size = new System.Drawing.Size(60, 20);
			this.tfRotX.TabIndex = 43;
			// 
			// label16
			// 
			this.label16.AutoSize = true;
			this.label16.Location = new System.Drawing.Point(252, 180);
			this.label16.Name = "label16";
			this.label16.Size = new System.Drawing.Size(126, 13);
			this.label16.TabIndex = 42;
			this.label16.Text = "Позиция (метры x, y, z):";
			// 
			// label17
			// 
			this.label17.AutoSize = true;
			this.label17.Location = new System.Drawing.Point(252, 141);
			this.label17.Name = "label17";
			this.label17.Size = new System.Drawing.Size(164, 13);
			this.label17.TabIndex = 41;
			this.label17.Text = "Вращение (град. вокруг x, y, z):";
			// 
			// label18
			// 
			this.label18.AutoSize = true;
			this.label18.Location = new System.Drawing.Point(252, 78);
			this.label18.Name = "label18";
			this.label18.Size = new System.Drawing.Size(92, 13);
			this.label18.TabIndex = 40;
			this.label18.Text = "Масштаб (x, y, z):";
			// 
			// tfPosZ
			// 
			this.tfPosZ.DecimalPlaces = 5;
			this.tfPosZ.Location = new System.Drawing.Point(384, 196);
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
			this.tfPosZ.TabIndex = 39;
			// 
			// tfPosY
			// 
			this.tfPosY.DecimalPlaces = 5;
			this.tfPosY.Location = new System.Drawing.Point(318, 196);
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
			this.tfPosY.TabIndex = 38;
			// 
			// tfPosX
			// 
			this.tfPosX.DecimalPlaces = 5;
			this.tfPosX.Location = new System.Drawing.Point(252, 196);
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
			this.tfPosX.TabIndex = 37;
			// 
			// tfSZ
			// 
			this.tfSZ.DecimalPlaces = 5;
			this.tfSZ.Location = new System.Drawing.Point(384, 94);
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
			this.tfSZ.TabIndex = 36;
			this.tfSZ.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
			// 
			// tfSY
			// 
			this.tfSY.DecimalPlaces = 5;
			this.tfSY.Location = new System.Drawing.Point(318, 94);
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
			this.tfSY.TabIndex = 35;
			this.tfSY.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
			// 
			// tfSX
			// 
			this.tfSX.DecimalPlaces = 5;
			this.tfSX.Location = new System.Drawing.Point(252, 94);
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
			this.tfSX.TabIndex = 34;
			this.tfSX.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
			// 
			// groupBox1
			// 
			this.groupBox1.Controls.Add(this.rbModelTangentsNoSplit);
			this.groupBox1.Controls.Add(this.rbModelTangentsSplit);
			this.groupBox1.Controls.Add(this.rbModelTangentsNo);
			this.groupBox1.Location = new System.Drawing.Point(11, 180);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(200, 92);
			this.groupBox1.TabIndex = 14;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Tangents";
			// 
			// rbModelTangentsNoSplit
			// 
			this.rbModelTangentsNoSplit.AutoSize = true;
			this.rbModelTangentsNoSplit.Location = new System.Drawing.Point(7, 67);
			this.rbModelTangentsNoSplit.Name = "rbModelTangentsNoSplit";
			this.rbModelTangentsNoSplit.Size = new System.Drawing.Size(142, 17);
			this.rbModelTangentsNoSplit.TabIndex = 2;
			this.rbModelTangentsNoSplit.Text = "Без разбиения вершин";
			this.rbModelTangentsNoSplit.UseVisualStyleBackColor = true;
			// 
			// rbModelTangentsSplit
			// 
			this.rbModelTangentsSplit.AutoSize = true;
			this.rbModelTangentsSplit.Checked = true;
			this.rbModelTangentsSplit.Location = new System.Drawing.Point(7, 44);
			this.rbModelTangentsSplit.Name = "rbModelTangentsSplit";
			this.rbModelTangentsSplit.Size = new System.Drawing.Size(138, 17);
			this.rbModelTangentsSplit.TabIndex = 1;
			this.rbModelTangentsSplit.TabStop = true;
			this.rbModelTangentsSplit.Text = "С разбиением вершин";
			this.rbModelTangentsSplit.UseVisualStyleBackColor = true;
			// 
			// rbModelTangentsNo
			// 
			this.rbModelTangentsNo.AutoSize = true;
			this.rbModelTangentsNo.Location = new System.Drawing.Point(7, 20);
			this.rbModelTangentsNo.Name = "rbModelTangentsNo";
			this.rbModelTangentsNo.Size = new System.Drawing.Size(112, 17);
			this.rbModelTangentsNo.TabIndex = 0;
			this.rbModelTangentsNo.Text = "Не генерировать";
			this.rbModelTangentsNo.UseVisualStyleBackColor = true;
			// 
			// cbModelEdges
			// 
			this.cbModelEdges.AutoSize = true;
			this.cbModelEdges.Location = new System.Drawing.Point(11, 117);
			this.cbModelEdges.Name = "cbModelEdges";
			this.cbModelEdges.Size = new System.Drawing.Size(207, 17);
			this.cbModelEdges.TabIndex = 13;
			this.cbModelEdges.Text = "Генерировать список рёбер (edges)";
			this.cbModelEdges.UseVisualStyleBackColor = true;
			// 
			// cbModelClean
			// 
			this.cbModelClean.AutoSize = true;
			this.cbModelClean.Checked = true;
			this.cbModelClean.CheckState = System.Windows.Forms.CheckState.Checked;
			this.cbModelClean.Location = new System.Drawing.Point(11, 78);
			this.cbModelClean.Name = "cbModelClean";
			this.cbModelClean.Size = new System.Drawing.Size(159, 17);
			this.cbModelClean.TabIndex = 12;
			this.cbModelClean.Text = "Удалить лишние вершины";
			this.cbModelClean.UseVisualStyleBackColor = true;
			// 
			// bOpenModelSrc
			// 
			this.bOpenModelSrc.Location = new System.Drawing.Point(593, 33);
			this.bOpenModelSrc.Name = "bOpenModelSrc";
			this.bOpenModelSrc.Size = new System.Drawing.Size(75, 23);
			this.bOpenModelSrc.TabIndex = 11;
			this.bOpenModelSrc.Text = "Открыть...";
			this.bOpenModelSrc.UseVisualStyleBackColor = true;
			this.bOpenModelSrc.Click += new System.EventHandler(this.bOpenModelSrc_Click);
			// 
			// tModelName
			// 
			this.tModelName.Location = new System.Drawing.Point(121, 35);
			this.tModelName.Name = "tModelName";
			this.tModelName.Size = new System.Drawing.Size(466, 20);
			this.tModelName.TabIndex = 10;
			this.tModelName.Text = "Models/vgt/apple/apple.obj";
			// 
			// label15
			// 
			this.label15.AutoSize = true;
			this.label15.Location = new System.Drawing.Point(8, 38);
			this.label15.Name = "label15";
			this.label15.Size = new System.Drawing.Size(49, 13);
			this.label15.TabIndex = 9;
			this.label15.Text = "Модель:";
			// 
			// bCreateModel
			// 
			this.bCreateModel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
			this.bCreateModel.Location = new System.Drawing.Point(593, 7);
			this.bCreateModel.Name = "bCreateModel";
			this.bCreateModel.Size = new System.Drawing.Size(75, 23);
			this.bCreateModel.TabIndex = 8;
			this.bCreateModel.Text = "Создать";
			this.bCreateModel.UseVisualStyleBackColor = true;
			this.bCreateModel.Click += new System.EventHandler(this.bCreateModel_Click);
			// 
			// tModelResName
			// 
			this.tModelResName.Location = new System.Drawing.Point(121, 9);
			this.tModelResName.Name = "tModelResName";
			this.tModelResName.Size = new System.Drawing.Size(158, 20);
			this.tModelResName.TabIndex = 7;
			this.tModelResName.Text = "arch/ECCY_Obelisk";
			// 
			// label13
			// 
			this.label13.AutoSize = true;
			this.label13.Location = new System.Drawing.Point(8, 12);
			this.label13.Name = "label13";
			this.label13.Size = new System.Drawing.Size(76, 13);
			this.label13.TabIndex = 6;
			this.label13.Text = "Имя ресурса:";
			// 
			// tpTerrain
			// 
			this.tpTerrain.Controls.Add(this.cbPhysHRD);
			this.tpTerrain.Controls.Add(this.label12);
			this.tpTerrain.Controls.Add(this.nChuScale);
			this.tpTerrain.Controls.Add(this.label11);
			this.tpTerrain.Controls.Add(this.nTqtDepth);
			this.tpTerrain.Controls.Add(this.label10);
			this.tpTerrain.Controls.Add(this.label9);
			this.tpTerrain.Controls.Add(this.nChuError);
			this.tpTerrain.Controls.Add(this.label8);
			this.tpTerrain.Controls.Add(this.nChuDepth);
			this.tpTerrain.Controls.Add(this.cbAlphaDXT5);
			this.tpTerrain.Controls.Add(this.tTexA);
			this.tpTerrain.Controls.Add(this.label7);
			this.tpTerrain.Controls.Add(this.tTexB);
			this.tpTerrain.Controls.Add(this.label6);
			this.tpTerrain.Controls.Add(this.tTexG);
			this.tpTerrain.Controls.Add(this.label5);
			this.tpTerrain.Controls.Add(this.tTexR);
			this.tpTerrain.Controls.Add(this.label4);
			this.tpTerrain.Controls.Add(this.bOpenAlpha);
			this.tpTerrain.Controls.Add(this.tAlphaFName);
			this.tpTerrain.Controls.Add(this.label3);
			this.tpTerrain.Controls.Add(this.bCreateTerrain);
			this.tpTerrain.Controls.Add(this.bOpenBT);
			this.tpTerrain.Controls.Add(this.tTerrainResName);
			this.tpTerrain.Controls.Add(this.tBTFName);
			this.tpTerrain.Controls.Add(this.label2);
			this.tpTerrain.Controls.Add(this.label1);
			this.tpTerrain.Location = new System.Drawing.Point(4, 22);
			this.tpTerrain.Name = "tpTerrain";
			this.tpTerrain.Padding = new System.Windows.Forms.Padding(3);
			this.tpTerrain.Size = new System.Drawing.Size(677, 266);
			this.tpTerrain.TabIndex = 1;
			this.tpTerrain.Text = "Ландшафт";
			this.tpTerrain.UseVisualStyleBackColor = true;
			// 
			// cbPhysHRD
			// 
			this.cbPhysHRD.AutoSize = true;
			this.cbPhysHRD.Checked = true;
			this.cbPhysHRD.CheckState = System.Windows.Forms.CheckState.Checked;
			this.cbPhysHRD.Location = new System.Drawing.Point(298, 219);
			this.cbPhysHRD.Name = "cbPhysHRD";
			this.cbPhysHRD.Size = new System.Drawing.Size(152, 17);
			this.cbPhysHRD.TabIndex = 29;
			this.cbPhysHRD.Text = " Экспортировать физику";
			this.cbPhysHRD.UseVisualStyleBackColor = true;
			// 
			// label12
			// 
			this.label12.AutoSize = true;
			this.label12.Location = new System.Drawing.Point(295, 239);
			this.label12.Name = "label12";
			this.label12.Size = new System.Drawing.Size(246, 13);
			this.label12.TabIndex = 28;
			this.label12.Text = "TQT2 base tile size = 256 by default, make setting!";
			// 
			// nChuScale
			// 
			this.nChuScale.DecimalPlaces = 3;
			this.nChuScale.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
			this.nChuScale.Location = new System.Drawing.Point(488, 142);
			this.nChuScale.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            196608});
			this.nChuScale.Name = "nChuScale";
			this.nChuScale.Size = new System.Drawing.Size(87, 20);
			this.nChuScale.TabIndex = 27;
			this.nChuScale.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
			// 
			// label11
			// 
			this.label11.AutoSize = true;
			this.label11.Location = new System.Drawing.Point(295, 170);
			this.label11.Name = "label11";
			this.label11.Size = new System.Drawing.Size(139, 13);
			this.label11.TabIndex = 26;
			this.label11.Text = "Глубина разбиения TQT2:";
			// 
			// nTqtDepth
			// 
			this.nTqtDepth.Location = new System.Drawing.Point(488, 168);
			this.nTqtDepth.Maximum = new decimal(new int[] {
            12,
            0,
            0,
            0});
			this.nTqtDepth.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
			this.nTqtDepth.Name = "nTqtDepth";
			this.nTqtDepth.Size = new System.Drawing.Size(87, 20);
			this.nTqtDepth.TabIndex = 25;
			this.nTqtDepth.Value = new decimal(new int[] {
            2,
            0,
            0,
            0});
			// 
			// label10
			// 
			this.label10.AutoSize = true;
			this.label10.Location = new System.Drawing.Point(295, 144);
			this.label10.Name = "label10";
			this.label10.Size = new System.Drawing.Size(178, 13);
			this.label10.TabIndex = 24;
			this.label10.Text = "Вертикальное масштабирование:";
			// 
			// label9
			// 
			this.label9.AutoSize = true;
			this.label9.Location = new System.Drawing.Point(295, 117);
			this.label9.Name = "label9";
			this.label9.Size = new System.Drawing.Size(123, 13);
			this.label9.TabIndex = 23;
			this.label9.Text = "Макс. ошибка (метры):";
			// 
			// nChuError
			// 
			this.nChuError.DecimalPlaces = 3;
			this.nChuError.Increment = new decimal(new int[] {
            5,
            0,
            0,
            131072});
			this.nChuError.Location = new System.Drawing.Point(488, 115);
			this.nChuError.Maximum = new decimal(new int[] {
            10,
            0,
            0,
            0});
			this.nChuError.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            196608});
			this.nChuError.Name = "nChuError";
			this.nChuError.Size = new System.Drawing.Size(87, 20);
			this.nChuError.TabIndex = 22;
			this.nChuError.Value = new decimal(new int[] {
            5,
            0,
            0,
            131072});
			// 
			// label8
			// 
			this.label8.AutoSize = true;
			this.label8.Location = new System.Drawing.Point(295, 91);
			this.label8.Name = "label8";
			this.label8.Size = new System.Drawing.Size(134, 13);
			this.label8.TabIndex = 21;
			this.label8.Text = "Глубина разбиения CHU:";
			// 
			// nChuDepth
			// 
			this.nChuDepth.Location = new System.Drawing.Point(488, 89);
			this.nChuDepth.Maximum = new decimal(new int[] {
            10,
            0,
            0,
            0});
			this.nChuDepth.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
			this.nChuDepth.Name = "nChuDepth";
			this.nChuDepth.Size = new System.Drawing.Size(87, 20);
			this.nChuDepth.TabIndex = 20;
			this.nChuDepth.Value = new decimal(new int[] {
            3,
            0,
            0,
            0});
			// 
			// cbAlphaDXT5
			// 
			this.cbAlphaDXT5.AutoSize = true;
			this.cbAlphaDXT5.Checked = true;
			this.cbAlphaDXT5.CheckState = System.Windows.Forms.CheckState.Checked;
			this.cbAlphaDXT5.Location = new System.Drawing.Point(298, 196);
			this.cbAlphaDXT5.Name = "cbAlphaDXT5";
			this.cbAlphaDXT5.Size = new System.Drawing.Size(224, 17);
			this.cbAlphaDXT5.TabIndex = 19;
			this.cbAlphaDXT5.Text = "Сжать альфа-карту сплаттинга в DXT5";
			this.cbAlphaDXT5.UseVisualStyleBackColor = true;
			// 
			// tTexA
			// 
			this.tTexA.Location = new System.Drawing.Point(121, 166);
			this.tTexA.Name = "tTexA";
			this.tTexA.Size = new System.Drawing.Size(158, 20);
			this.tTexA.TabIndex = 18;
			this.tTexA.Text = "ground/rock.dds";
			// 
			// label7
			// 
			this.label7.AutoSize = true;
			this.label7.Location = new System.Drawing.Point(8, 169);
			this.label7.Name = "label7";
			this.label7.Size = new System.Drawing.Size(106, 13);
			this.label7.TabIndex = 17;
			this.label7.Text = "Текстура канала A:";
			// 
			// tTexB
			// 
			this.tTexB.Location = new System.Drawing.Point(121, 140);
			this.tTexB.Name = "tTexB";
			this.tTexB.Size = new System.Drawing.Size(158, 20);
			this.tTexB.TabIndex = 16;
			this.tTexB.Text = "ground/sand.dds";
			// 
			// label6
			// 
			this.label6.AutoSize = true;
			this.label6.Location = new System.Drawing.Point(8, 143);
			this.label6.Name = "label6";
			this.label6.Size = new System.Drawing.Size(106, 13);
			this.label6.TabIndex = 15;
			this.label6.Text = "Текстура канала B:";
			// 
			// tTexG
			// 
			this.tTexG.Location = new System.Drawing.Point(121, 114);
			this.tTexG.Name = "tTexG";
			this.tTexG.Size = new System.Drawing.Size(158, 20);
			this.tTexG.TabIndex = 14;
			this.tTexG.Text = "ground/brown.dds";
			// 
			// label5
			// 
			this.label5.AutoSize = true;
			this.label5.Location = new System.Drawing.Point(8, 117);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(107, 13);
			this.label5.TabIndex = 13;
			this.label5.Text = "Текстура канала G:";
			// 
			// tTexR
			// 
			this.tTexR.Location = new System.Drawing.Point(121, 88);
			this.tTexR.Name = "tTexR";
			this.tTexR.Size = new System.Drawing.Size(158, 20);
			this.tTexR.TabIndex = 12;
			this.tTexR.Text = "ground/agrar.dds";
			// 
			// label4
			// 
			this.label4.AutoSize = true;
			this.label4.Location = new System.Drawing.Point(8, 91);
			this.label4.Name = "label4";
			this.label4.Size = new System.Drawing.Size(107, 13);
			this.label4.TabIndex = 11;
			this.label4.Text = "Текстура канала R:";
			// 
			// bOpenAlpha
			// 
			this.bOpenAlpha.Location = new System.Drawing.Point(593, 60);
			this.bOpenAlpha.Name = "bOpenAlpha";
			this.bOpenAlpha.Size = new System.Drawing.Size(75, 23);
			this.bOpenAlpha.TabIndex = 10;
			this.bOpenAlpha.Text = "Открыть...";
			this.bOpenAlpha.UseVisualStyleBackColor = true;
			this.bOpenAlpha.Click += new System.EventHandler(this.bOpenAlpha_Click);
			// 
			// tAlphaFName
			// 
			this.tAlphaFName.Location = new System.Drawing.Point(121, 62);
			this.tAlphaFName.Name = "tAlphaFName";
			this.tAlphaFName.Size = new System.Drawing.Size(466, 20);
			this.tAlphaFName.TabIndex = 9;
			this.tAlphaFName.Text = "F:\\Game Content\\Terrain & Sky\\L3DT Projects\\Alpha\\AlphaMap.tga";
			// 
			// label3
			// 
			this.label3.AutoSize = true;
			this.label3.Location = new System.Drawing.Point(8, 65);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(100, 13);
			this.label3.TabIndex = 8;
			this.label3.Text = "Карта сплаттинга:";
			// 
			// bCreateTerrain
			// 
			this.bCreateTerrain.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
			this.bCreateTerrain.Location = new System.Drawing.Point(593, 7);
			this.bCreateTerrain.Name = "bCreateTerrain";
			this.bCreateTerrain.Size = new System.Drawing.Size(75, 23);
			this.bCreateTerrain.TabIndex = 5;
			this.bCreateTerrain.Text = "Создать";
			this.bCreateTerrain.UseVisualStyleBackColor = true;
			this.bCreateTerrain.Click += new System.EventHandler(this.bCreateTerrain_Click);
			// 
			// bOpenBT
			// 
			this.bOpenBT.Location = new System.Drawing.Point(593, 34);
			this.bOpenBT.Name = "bOpenBT";
			this.bOpenBT.Size = new System.Drawing.Size(75, 23);
			this.bOpenBT.TabIndex = 4;
			this.bOpenBT.Text = "Открыть...";
			this.bOpenBT.UseVisualStyleBackColor = true;
			this.bOpenBT.Click += new System.EventHandler(this.bOpenBT_Click);
			// 
			// tTerrainResName
			// 
			this.tTerrainResName.Location = new System.Drawing.Point(121, 9);
			this.tTerrainResName.Name = "tTerrainResName";
			this.tTerrainResName.Size = new System.Drawing.Size(158, 20);
			this.tTerrainResName.TabIndex = 3;
			this.tTerrainResName.Text = "terrain/test";
			// 
			// tBTFName
			// 
			this.tBTFName.Location = new System.Drawing.Point(121, 36);
			this.tBTFName.Name = "tBTFName";
			this.tBTFName.Size = new System.Drawing.Size(466, 20);
			this.tBTFName.TabIndex = 2;
			this.tBTFName.Text = "F:\\Game Content\\Terrain & Sky\\L3DT Projects\\1_HF.bt";
			// 
			// label2
			// 
			this.label2.AutoSize = true;
			this.label2.Location = new System.Drawing.Point(8, 12);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(76, 13);
			this.label2.TabIndex = 1;
			this.label2.Text = "Имя ресурса:";
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(8, 39);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(74, 13);
			this.label1.TabIndex = 0;
			this.label1.Text = "Карта высот:";
			// 
			// tpSettings
			// 
			this.tpSettings.Controls.Add(this.tProjDir);
			this.tpSettings.Controls.Add(this.label14);
			this.tpSettings.Location = new System.Drawing.Point(4, 22);
			this.tpSettings.Name = "tpSettings";
			this.tpSettings.Size = new System.Drawing.Size(677, 266);
			this.tpSettings.TabIndex = 2;
			this.tpSettings.Text = "Настройки";
			this.tpSettings.UseVisualStyleBackColor = true;
			// 
			// tProjDir
			// 
			this.tProjDir.Location = new System.Drawing.Point(131, 10);
			this.tProjDir.Name = "tProjDir";
			this.tProjDir.Size = new System.Drawing.Size(538, 20);
			this.tProjDir.TabIndex = 1;
			// 
			// label14
			// 
			this.label14.AutoSize = true;
			this.label14.Location = new System.Drawing.Point(9, 13);
			this.label14.Name = "label14";
			this.label14.Size = new System.Drawing.Size(116, 13);
			this.label14.TabIndex = 0;
			this.label14.Text = "Директория проекта:";
			// 
			// tOutput
			// 
			this.tOutput.Dock = System.Windows.Forms.DockStyle.Bottom;
			this.tOutput.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
			this.tOutput.Location = new System.Drawing.Point(0, 317);
			this.tOutput.Multiline = true;
			this.tOutput.Name = "tOutput";
			this.tOutput.ReadOnly = true;
			this.tOutput.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
			this.tOutput.Size = new System.Drawing.Size(685, 180);
			this.tOutput.TabIndex = 7;
			// 
			// cbModelGenerateShadow
			// 
			this.cbModelGenerateShadow.AutoSize = true;
			this.cbModelGenerateShadow.Location = new System.Drawing.Point(11, 157);
			this.cbModelGenerateShadow.Name = "cbModelGenerateShadow";
			this.cbModelGenerateShadow.Size = new System.Drawing.Size(123, 17);
			this.cbModelGenerateShadow.TabIndex = 49;
			this.cbModelGenerateShadow.Text = "Генерировать тень";
			this.cbModelGenerateShadow.UseVisualStyleBackColor = true;
			this.cbModelGenerateShadow.CheckedChanged += new System.EventHandler(this.cbGenerateShadow_CheckedChanged);
			// 
			// Main
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(685, 497);
			this.Controls.Add(this.tOutput);
			this.Controls.Add(this.tcMain);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.Fixed3D;
			this.Name = "Main";
			this.Text = "ContentForge for Nebula2";
			this.tcMain.ResumeLayout(false);
			this.tpModel.ResumeLayout(false);
			this.tpModel.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.tfRotZ)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.tfRotY)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.tfRotX)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.tfPosZ)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.tfPosY)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.tfPosX)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.tfSZ)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.tfSY)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.tfSX)).EndInit();
			this.groupBox1.ResumeLayout(false);
			this.groupBox1.PerformLayout();
			this.tpTerrain.ResumeLayout(false);
			this.tpTerrain.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.nChuScale)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.nTqtDepth)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.nChuError)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.nChuDepth)).EndInit();
			this.tpSettings.ResumeLayout(false);
			this.tpSettings.PerformLayout();
			this.ResumeLayout(false);
			this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TabControl tcMain;
        private System.Windows.Forms.TabPage tpModel;
        private System.Windows.Forms.TabPage tpTerrain;
        private System.Windows.Forms.TextBox tTerrainResName;
        private System.Windows.Forms.TextBox tBTFName;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.OpenFileDialog OD;
        private System.Windows.Forms.Button bOpenBT;
		private System.Windows.Forms.Button bCreateTerrain;
        private System.Windows.Forms.Button bOpenAlpha;
        private System.Windows.Forms.TextBox tAlphaFName;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox tTexR;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox tTexA;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.TextBox tTexB;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.TextBox tTexG;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.CheckBox cbAlphaDXT5;
        private System.Windows.Forms.NumericUpDown nChuError;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.NumericUpDown nChuDepth;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Label label11;
        private System.Windows.Forms.NumericUpDown nTqtDepth;
        private System.Windows.Forms.NumericUpDown nChuScale;
		private System.Windows.Forms.Label label12;
        private System.Windows.Forms.TabPage tpSettings;
        private System.Windows.Forms.TextBox tProjDir;
        private System.Windows.Forms.Label label14;
        private System.Windows.Forms.CheckBox cbPhysHRD;
		private System.Windows.Forms.TextBox tOutput;
		private System.Windows.Forms.Button bCreateModel;
		private System.Windows.Forms.TextBox tModelResName;
		private System.Windows.Forms.Label label13;
		private System.Windows.Forms.Button bOpenModelSrc;
		private System.Windows.Forms.TextBox tModelName;
		private System.Windows.Forms.Label label15;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.RadioButton rbModelTangentsNoSplit;
		private System.Windows.Forms.RadioButton rbModelTangentsSplit;
		private System.Windows.Forms.RadioButton rbModelTangentsNo;
		private System.Windows.Forms.CheckBox cbModelEdges;
		private System.Windows.Forms.CheckBox cbModelClean;
		private System.Windows.Forms.NumericUpDown tfRotZ;
		private System.Windows.Forms.NumericUpDown tfRotY;
		private System.Windows.Forms.NumericUpDown tfRotX;
		private System.Windows.Forms.Label label16;
		private System.Windows.Forms.Label label17;
		private System.Windows.Forms.Label label18;
		private System.Windows.Forms.NumericUpDown tfPosZ;
		private System.Windows.Forms.NumericUpDown tfPosY;
		private System.Windows.Forms.NumericUpDown tfPosX;
		private System.Windows.Forms.NumericUpDown tfSZ;
		private System.Windows.Forms.NumericUpDown tfSY;
		private System.Windows.Forms.NumericUpDown tfSX;
        private System.Windows.Forms.CheckBox cbModelNormals;
        private System.Windows.Forms.CheckBox cbModelUniformScale;
        private System.Windows.Forms.CheckBox cbModelInvertTexV;
		private System.Windows.Forms.CheckBox cbModelGenerateShadow;
    }
}

