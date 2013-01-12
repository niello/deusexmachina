namespace CreatorIDE
{
	partial class MainWnd
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainWnd));
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.mmProject = new System.Windows.Forms.ToolStripMenuItem();
            this.mmRestoreDB = new System.Windows.Forms.ToolStripMenuItem();
            this.mmSaveDB = new System.Windows.Forms.ToolStripMenuItem();
            this.mmExport = new System.Windows.Forms.ToolStripMenuItem();
            this.mmExit = new System.Windows.Forms.ToolStripMenuItem();
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.toolStrip1 = new System.Windows.Forms.ToolStrip();
            this.bSelectByMouse = new System.Windows.Forms.ToolStripButton();
            this.bTfmMode = new System.Windows.Forms.ToolStripButton();
            this.bNewCategory = new System.Windows.Forms.ToolStripButton();
            this.bNewLocation = new System.Windows.Forms.ToolStripButton();
            this.bPauseWorld = new System.Windows.Forms.ToolStripButton();
            this.bNotAbove = new System.Windows.Forms.ToolStripButton();
            this.bNotBelow = new System.Windows.Forms.ToolStripButton();
            this.bBuildNavMesh = new System.Windows.Forms.ToolStripButton();
            this.bNavRegions = new System.Windows.Forms.ToolStripButton();
            this.bNavOffmesh = new System.Windows.Forms.ToolStripButton();
            this.splitContainer2 = new System.Windows.Forms.SplitContainer();
            this.splitContainer3 = new System.Windows.Forms.SplitContainer();
            this.lEntityUnderMouse = new System.Windows.Forms.Label();
            this.EngineViewPanel = new System.Windows.Forms.Panel();
            this.tOutput = new System.Windows.Forms.TextBox();
            this.lCatName = new System.Windows.Forms.Label();
            this.PropGrid = new System.Windows.Forms.PropertyGrid();
            this.pEntButtons = new System.Windows.Forms.Panel();
            this.bSaveAsTemplate = new System.Windows.Forms.Button();
            this.bCreateEntity = new System.Windows.Forms.Button();
            this.bDeleteEntity = new System.Windows.Forms.Button();
            this.bApplyPropChanges = new System.Windows.Forms.Button();
            this.pTplButtons = new System.Windows.Forms.Panel();
            this.bApplyTplChanges = new System.Windows.Forms.Button();
            this.bCreateTemplate = new System.Windows.Forms.Button();
            this.bDeleteTemplate = new System.Windows.Forms.Button();
            this.button4 = new System.Windows.Forms.Button();
            this.tcLevelsObjects = new System.Windows.Forms.TabControl();
            this.tpLevels = new System.Windows.Forms.TabPage();
            this.lbLevels = new System.Windows.Forms.ListBox();
            this.tpEntityCats = new System.Windows.Forms.TabPage();
            this.tvCatsTpls = new System.Windows.Forms.TreeView();
            this.tpEntities = new System.Windows.Forms.TabPage();
            this.tvEntities = new System.Windows.Forms.TreeView();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.menuStrip1.SuspendLayout();
            this.toolStrip1.SuspendLayout();
            this.splitContainer2.Panel1.SuspendLayout();
            this.splitContainer2.Panel2.SuspendLayout();
            this.splitContainer2.SuspendLayout();
            this.splitContainer3.Panel1.SuspendLayout();
            this.splitContainer3.Panel2.SuspendLayout();
            this.splitContainer3.SuspendLayout();
            this.pEntButtons.SuspendLayout();
            this.pTplButtons.SuspendLayout();
            this.tcLevelsObjects.SuspendLayout();
            this.tpLevels.SuspendLayout();
            this.tpEntityCats.SuspendLayout();
            this.tpEntities.SuspendLayout();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.SuspendLayout();
            // 
            // menuStrip1
            // 
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mmProject});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Size = new System.Drawing.Size(1226, 24);
            this.menuStrip1.TabIndex = 2;
            this.menuStrip1.Text = "menuStrip1";
            // 
            // mmProject
            // 
            this.mmProject.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mmRestoreDB,
            this.mmSaveDB,
            this.mmExport,
            this.mmExit});
            this.mmProject.Name = "mmProject";
            this.mmProject.Size = new System.Drawing.Size(59, 20);
            this.mmProject.Text = "Проект";
            // 
            // mmRestoreDB
            // 
            this.mmRestoreDB.Name = "mmRestoreDB";
            this.mmRestoreDB.Size = new System.Drawing.Size(176, 22);
            this.mmRestoreDB.Text = "Восстановить базу";
            this.mmRestoreDB.Click += new System.EventHandler(this.mmRestoreDB_Click);
            // 
            // mmSaveDB
            // 
            this.mmSaveDB.Name = "mmSaveDB";
            this.mmSaveDB.Size = new System.Drawing.Size(176, 22);
            this.mmSaveDB.Text = "Сохранить базу";
            this.mmSaveDB.Click += new System.EventHandler(this.mmSaveDB_Click);
            // 
            // mmExport
            // 
            this.mmExport.Name = "mmExport";
            this.mmExport.Size = new System.Drawing.Size(176, 22);
            this.mmExport.Text = "Экспортировать";
            this.mmExport.Click += new System.EventHandler(this.mmExport_Click);
            // 
            // mmExit
            // 
            this.mmExit.Name = "mmExit";
            this.mmExit.Size = new System.Drawing.Size(176, 22);
            this.mmExit.Text = "Выход";
            this.mmExit.Click += new System.EventHandler(this.mmExit_Click);
            // 
            // statusStrip1
            // 
            this.statusStrip1.Location = new System.Drawing.Point(0, 720);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(1226, 22);
            this.statusStrip1.TabIndex = 3;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // toolStrip1
            // 
            this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.bSelectByMouse,
            this.bTfmMode,
            this.bNewCategory,
            this.bNewLocation,
            this.bPauseWorld,
            this.bNotAbove,
            this.bNotBelow,
            this.bBuildNavMesh,
            this.bNavRegions,
            this.bNavOffmesh});
            this.toolStrip1.Location = new System.Drawing.Point(0, 24);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.Size = new System.Drawing.Size(1226, 25);
            this.toolStrip1.TabIndex = 6;
            this.toolStrip1.Text = "toolStrip1";
            // 
            // bSelectByMouse
            // 
            this.bSelectByMouse.Checked = true;
            this.bSelectByMouse.CheckOnClick = true;
            this.bSelectByMouse.CheckState = System.Windows.Forms.CheckState.Checked;
            this.bSelectByMouse.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.bSelectByMouse.Image = ((System.Drawing.Image)(resources.GetObject("bSelectByMouse.Image")));
            this.bSelectByMouse.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.bSelectByMouse.Name = "bSelectByMouse";
            this.bSelectByMouse.Size = new System.Drawing.Size(23, 22);
            this.bSelectByMouse.Text = "Выбор мышью";
            // 
            // bTfmMode
            // 
            this.bTfmMode.CheckOnClick = true;
            this.bTfmMode.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.bTfmMode.Image = ((System.Drawing.Image)(resources.GetObject("bTfmMode.Image")));
            this.bTfmMode.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.bTfmMode.Name = "bTfmMode";
            this.bTfmMode.Size = new System.Drawing.Size(23, 22);
            this.bTfmMode.Text = "Режим трансформации";
            this.bTfmMode.CheckedChanged += new System.EventHandler(this.bTfmMode_CheckedChanged);
            // 
            // bNewCategory
            // 
            this.bNewCategory.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.bNewCategory.Image = ((System.Drawing.Image)(resources.GetObject("bNewCategory.Image")));
            this.bNewCategory.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.bNewCategory.Name = "bNewCategory";
            this.bNewCategory.Size = new System.Drawing.Size(23, 22);
            this.bNewCategory.Text = "Новая категория";
            this.bNewCategory.Click += new System.EventHandler(this.bNewCategory_Click);
            // 
            // bNewLocation
            // 
            this.bNewLocation.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.bNewLocation.Image = ((System.Drawing.Image)(resources.GetObject("bNewLocation.Image")));
            this.bNewLocation.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.bNewLocation.Name = "bNewLocation";
            this.bNewLocation.RightToLeftAutoMirrorImage = true;
            this.bNewLocation.Size = new System.Drawing.Size(23, 22);
            this.bNewLocation.Text = "Новая локация";
            this.bNewLocation.Click += new System.EventHandler(this.bNewLocation_Click);
            // 
            // bPauseWorld
            // 
            this.bPauseWorld.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.bPauseWorld.Image = ((System.Drawing.Image)(resources.GetObject("bPauseWorld.Image")));
            this.bPauseWorld.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.bPauseWorld.Name = "bPauseWorld";
            this.bPauseWorld.Size = new System.Drawing.Size(23, 22);
            this.bPauseWorld.Text = "Вкл/выкл паузу";
            this.bPauseWorld.Click += new System.EventHandler(this.bPauseWorld_Click);
            // 
            // bNotAbove
            // 
            this.bNotAbove.CheckOnClick = true;
            this.bNotAbove.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.bNotAbove.Image = ((System.Drawing.Image)(resources.GetObject("bNotAbove.Image")));
            this.bNotAbove.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.bNotAbove.Name = "bNotAbove";
            this.bNotAbove.Size = new System.Drawing.Size(23, 22);
            this.bNotAbove.Text = "Не выше земли";
            this.bNotAbove.CheckedChanged += new System.EventHandler(this.bGroundConstrBtns_CheckedChanged);
            // 
            // bNotBelow
            // 
            this.bNotBelow.CheckOnClick = true;
            this.bNotBelow.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.bNotBelow.Image = ((System.Drawing.Image)(resources.GetObject("bNotBelow.Image")));
            this.bNotBelow.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.bNotBelow.Name = "bNotBelow";
            this.bNotBelow.Size = new System.Drawing.Size(23, 22);
            this.bNotBelow.Text = "Не ниже земли";
            this.bNotBelow.CheckedChanged += new System.EventHandler(this.bGroundConstrBtns_CheckedChanged);
            // 
            // bBuildNavMesh
            // 
            this.bBuildNavMesh.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.bBuildNavMesh.Image = ((System.Drawing.Image)(resources.GetObject("bBuildNavMesh.Image")));
            this.bBuildNavMesh.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.bBuildNavMesh.Name = "bBuildNavMesh";
            this.bBuildNavMesh.Size = new System.Drawing.Size(23, 22);
            this.bBuildNavMesh.Text = "Построить навигационную сетку";
            this.bBuildNavMesh.Click += new System.EventHandler(this.bBuildNavMesh_Click);
            // 
            // bNavRegions
            // 
            this.bNavRegions.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.bNavRegions.Image = ((System.Drawing.Image)(resources.GetObject("bNavRegions.Image")));
            this.bNavRegions.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.bNavRegions.Name = "bNavRegions";
            this.bNavRegions.Size = new System.Drawing.Size(23, 22);
            this.bNavRegions.Text = "Навигация - области";
            this.bNavRegions.Click += new System.EventHandler(this.bNavRegions_Click);
            // 
            // bNavOffmesh
            // 
            this.bNavOffmesh.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.bNavOffmesh.Image = ((System.Drawing.Image)(resources.GetObject("bNavOffmesh.Image")));
            this.bNavOffmesh.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.bNavOffmesh.Name = "bNavOffmesh";
            this.bNavOffmesh.Size = new System.Drawing.Size(23, 22);
            this.bNavOffmesh.Text = "Навигация - связи (offmesh)";
            this.bNavOffmesh.Click += new System.EventHandler(this.bNavOffmesh_Click);
            // 
            // splitContainer2
            // 
            this.splitContainer2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer2.Location = new System.Drawing.Point(0, 0);
            this.splitContainer2.Name = "splitContainer2";
            // 
            // splitContainer2.Panel1
            // 
            this.splitContainer2.Panel1.Controls.Add(this.splitContainer3);
            // 
            // splitContainer2.Panel2
            // 
            this.splitContainer2.Panel2.Controls.Add(this.lCatName);
            this.splitContainer2.Panel2.Controls.Add(this.PropGrid);
            this.splitContainer2.Panel2.Controls.Add(this.pEntButtons);
            this.splitContainer2.Panel2.Controls.Add(this.pTplButtons);
            this.splitContainer2.Size = new System.Drawing.Size(1035, 668);
            this.splitContainer2.SplitterDistance = 758;
            this.splitContainer2.TabIndex = 4;
            // 
            // splitContainer3
            // 
            this.splitContainer3.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer3.Location = new System.Drawing.Point(0, 0);
            this.splitContainer3.Name = "splitContainer3";
            this.splitContainer3.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer3.Panel1
            // 
            this.splitContainer3.Panel1.Controls.Add(this.lEntityUnderMouse);
            this.splitContainer3.Panel1.Controls.Add(this.EngineViewPanel);
            // 
            // splitContainer3.Panel2
            // 
            this.splitContainer3.Panel2.Controls.Add(this.tOutput);
            this.splitContainer3.Size = new System.Drawing.Size(758, 668);
            this.splitContainer3.SplitterDistance = 558;
            this.splitContainer3.TabIndex = 0;
            // 
            // lEntityUnderMouse
            // 
            this.lEntityUnderMouse.AutoSize = true;
            this.lEntityUnderMouse.Location = new System.Drawing.Point(4, 5);
            this.lEntityUnderMouse.Name = "lEntityUnderMouse";
            this.lEntityUnderMouse.Size = new System.Drawing.Size(124, 13);
            this.lEntityUnderMouse.TabIndex = 7;
            this.lEntityUnderMouse.Text = "Объект под курсором: ";
            // 
            // EngineViewPanel
            // 
            this.EngineViewPanel.AllowDrop = true;
            this.EngineViewPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.EngineViewPanel.Location = new System.Drawing.Point(4, 23);
            this.EngineViewPanel.Name = "EngineViewPanel";
            this.EngineViewPanel.Size = new System.Drawing.Size(750, 533);
            this.EngineViewPanel.TabIndex = 6;
            this.EngineViewPanel.PreviewKeyDown += new System.Windows.Forms.PreviewKeyDownEventHandler(this.EngineViewPanel_PreviewKeyDown);
            // 
            // tOutput
            // 
            this.tOutput.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tOutput.Font = new System.Drawing.Font("Courier New", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.tOutput.Location = new System.Drawing.Point(0, 0);
            this.tOutput.Multiline = true;
            this.tOutput.Name = "tOutput";
            this.tOutput.ReadOnly = true;
            this.tOutput.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.tOutput.Size = new System.Drawing.Size(758, 106);
            this.tOutput.TabIndex = 0;
            // 
            // lCatName
            // 
            this.lCatName.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.lCatName.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Underline, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.lCatName.Location = new System.Drawing.Point(111, 81);
            this.lCatName.Name = "lCatName";
            this.lCatName.Size = new System.Drawing.Size(159, 23);
            this.lCatName.TabIndex = 5;
            this.lCatName.Text = "     ";
            this.lCatName.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // PropGrid
            // 
            this.PropGrid.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.PropGrid.Location = new System.Drawing.Point(0, 82);
            this.PropGrid.Name = "PropGrid";
            this.PropGrid.PropertySort = System.Windows.Forms.PropertySort.Categorized;
            this.PropGrid.Size = new System.Drawing.Size(273, 586);
            this.PropGrid.TabIndex = 0;
            // 
            // pEntButtons
            // 
            this.pEntButtons.Controls.Add(this.bSaveAsTemplate);
            this.pEntButtons.Controls.Add(this.bCreateEntity);
            this.pEntButtons.Controls.Add(this.bDeleteEntity);
            this.pEntButtons.Controls.Add(this.bApplyPropChanges);
            this.pEntButtons.Location = new System.Drawing.Point(0, 0);
            this.pEntButtons.Name = "pEntButtons";
            this.pEntButtons.Size = new System.Drawing.Size(273, 76);
            this.pEntButtons.TabIndex = 10;
            // 
            // bSaveAsTemplate
            // 
            this.bSaveAsTemplate.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.bSaveAsTemplate.Location = new System.Drawing.Point(3, 26);
            this.bSaveAsTemplate.Name = "bSaveAsTemplate";
            this.bSaveAsTemplate.Size = new System.Drawing.Size(267, 23);
            this.bSaveAsTemplate.TabIndex = 7;
            this.bSaveAsTemplate.Text = "Сохранить шаблон";
            this.bSaveAsTemplate.UseVisualStyleBackColor = true;
            this.bSaveAsTemplate.Click += new System.EventHandler(this.bSaveAsTemplate_Click);
            // 
            // bCreateEntity
            // 
            this.bCreateEntity.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.bCreateEntity.Location = new System.Drawing.Point(3, 4);
            this.bCreateEntity.Name = "bCreateEntity";
            this.bCreateEntity.Size = new System.Drawing.Size(267, 23);
            this.bCreateEntity.TabIndex = 6;
            this.bCreateEntity.Text = "Создать";
            this.bCreateEntity.UseVisualStyleBackColor = true;
            this.bCreateEntity.Click += new System.EventHandler(this.bCreateEntity_Click);
            // 
            // bDeleteEntity
            // 
            this.bDeleteEntity.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.bDeleteEntity.Location = new System.Drawing.Point(3, 48);
            this.bDeleteEntity.Name = "bDeleteEntity";
            this.bDeleteEntity.Size = new System.Drawing.Size(267, 23);
            this.bDeleteEntity.TabIndex = 5;
            this.bDeleteEntity.Text = "Удалить";
            this.bDeleteEntity.UseVisualStyleBackColor = true;
            this.bDeleteEntity.Click += new System.EventHandler(this.bDeleteEntity_Click);
            // 
            // bApplyPropChanges
            // 
            this.bApplyPropChanges.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.bApplyPropChanges.Location = new System.Drawing.Point(3, 4);
            this.bApplyPropChanges.Name = "bApplyPropChanges";
            this.bApplyPropChanges.Size = new System.Drawing.Size(267, 23);
            this.bApplyPropChanges.TabIndex = 8;
            this.bApplyPropChanges.Text = "Применить изменения";
            this.bApplyPropChanges.UseVisualStyleBackColor = true;
            this.bApplyPropChanges.Click += new System.EventHandler(this.bApplyPropChanges_Click);
            // 
            // pTplButtons
            // 
            this.pTplButtons.Controls.Add(this.bApplyTplChanges);
            this.pTplButtons.Controls.Add(this.bCreateTemplate);
            this.pTplButtons.Controls.Add(this.bDeleteTemplate);
            this.pTplButtons.Controls.Add(this.button4);
            this.pTplButtons.Location = new System.Drawing.Point(0, 0);
            this.pTplButtons.Name = "pTplButtons";
            this.pTplButtons.Size = new System.Drawing.Size(271, 76);
            this.pTplButtons.TabIndex = 8;
            // 
            // bApplyTplChanges
            // 
            this.bApplyTplChanges.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.bApplyTplChanges.Location = new System.Drawing.Point(3, 4);
            this.bApplyTplChanges.Name = "bApplyTplChanges";
            this.bApplyTplChanges.Size = new System.Drawing.Size(267, 23);
            this.bApplyTplChanges.TabIndex = 9;
            this.bApplyTplChanges.Text = "Применить изменения";
            this.bApplyTplChanges.UseVisualStyleBackColor = true;
            this.bApplyTplChanges.Click += new System.EventHandler(this.bApplyTplChanges_Click);
            // 
            // bCreateTemplate
            // 
            this.bCreateTemplate.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.bCreateTemplate.Location = new System.Drawing.Point(3, 4);
            this.bCreateTemplate.Name = "bCreateTemplate";
            this.bCreateTemplate.Size = new System.Drawing.Size(267, 23);
            this.bCreateTemplate.TabIndex = 6;
            this.bCreateTemplate.Text = "Создать";
            this.bCreateTemplate.UseVisualStyleBackColor = true;
            this.bCreateTemplate.Click += new System.EventHandler(this.bCreateTemplate_Click);
            // 
            // bDeleteTemplate
            // 
            this.bDeleteTemplate.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.bDeleteTemplate.Location = new System.Drawing.Point(3, 26);
            this.bDeleteTemplate.Name = "bDeleteTemplate";
            this.bDeleteTemplate.Size = new System.Drawing.Size(267, 23);
            this.bDeleteTemplate.TabIndex = 5;
            this.bDeleteTemplate.Text = "Удалить";
            this.bDeleteTemplate.UseVisualStyleBackColor = true;
            this.bDeleteTemplate.Click += new System.EventHandler(this.bDeleteTemplate_Click);
            // 
            // button4
            // 
            this.button4.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.button4.Location = new System.Drawing.Point(3, 4);
            this.button4.Name = "button4";
            this.button4.Size = new System.Drawing.Size(267, 23);
            this.button4.TabIndex = 8;
            this.button4.Text = "Применить изменения";
            this.button4.UseVisualStyleBackColor = true;
            // 
            // tcLevelsObjects
            // 
            this.tcLevelsObjects.Controls.Add(this.tpLevels);
            this.tcLevelsObjects.Controls.Add(this.tpEntityCats);
            this.tcLevelsObjects.Controls.Add(this.tpEntities);
            this.tcLevelsObjects.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tcLevelsObjects.Location = new System.Drawing.Point(0, 0);
            this.tcLevelsObjects.Name = "tcLevelsObjects";
            this.tcLevelsObjects.SelectedIndex = 0;
            this.tcLevelsObjects.Size = new System.Drawing.Size(187, 668);
            this.tcLevelsObjects.TabIndex = 5;
            this.tcLevelsObjects.SelectedIndexChanged += new System.EventHandler(this.tcLevelsObjects_SelectedIndexChanged);
            // 
            // tpLevels
            // 
            this.tpLevels.Controls.Add(this.lbLevels);
            this.tpLevels.Location = new System.Drawing.Point(4, 22);
            this.tpLevels.Name = "tpLevels";
            this.tpLevels.Padding = new System.Windows.Forms.Padding(3);
            this.tpLevels.Size = new System.Drawing.Size(179, 642);
            this.tpLevels.TabIndex = 0;
            this.tpLevels.Text = "Локации";
            this.tpLevels.UseVisualStyleBackColor = true;
            // 
            // lbLevels
            // 
            this.lbLevels.Dock = System.Windows.Forms.DockStyle.Fill;
            this.lbLevels.FormattingEnabled = true;
            this.lbLevels.Location = new System.Drawing.Point(3, 3);
            this.lbLevels.Name = "lbLevels";
            this.lbLevels.Size = new System.Drawing.Size(173, 628);
            this.lbLevels.TabIndex = 0;
            this.lbLevels.DoubleClick += new System.EventHandler(this.lbLevels_DoubleClick);
            // 
            // tpEntityCats
            // 
            this.tpEntityCats.Controls.Add(this.tvCatsTpls);
            this.tpEntityCats.Location = new System.Drawing.Point(4, 22);
            this.tpEntityCats.Name = "tpEntityCats";
            this.tpEntityCats.Padding = new System.Windows.Forms.Padding(3);
            this.tpEntityCats.Size = new System.Drawing.Size(179, 642);
            this.tpEntityCats.TabIndex = 1;
            this.tpEntityCats.Text = "Шаблоны";
            this.tpEntityCats.UseVisualStyleBackColor = true;
            // 
            // tvCatsTpls
            // 
            this.tvCatsTpls.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tvCatsTpls.HideSelection = false;
            this.tvCatsTpls.Location = new System.Drawing.Point(3, 3);
            this.tvCatsTpls.Name = "tvCatsTpls";
            this.tvCatsTpls.Size = new System.Drawing.Size(173, 636);
            this.tvCatsTpls.TabIndex = 0;
            this.tvCatsTpls.MouseUp += new System.Windows.Forms.MouseEventHandler(this.tvCatsTpls_MouseUp);
            this.tvCatsTpls.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.tvCatsTpls_AfterSelect);
            this.tvCatsTpls.ItemDrag += new System.Windows.Forms.ItemDragEventHandler(this.tvCatsTpls_ItemDrag);
            // 
            // tpEntities
            // 
            this.tpEntities.Controls.Add(this.tvEntities);
            this.tpEntities.Location = new System.Drawing.Point(4, 22);
            this.tpEntities.Name = "tpEntities";
            this.tpEntities.Padding = new System.Windows.Forms.Padding(3);
            this.tpEntities.Size = new System.Drawing.Size(179, 642);
            this.tpEntities.TabIndex = 2;
            this.tpEntities.Text = "Объекты";
            this.tpEntities.UseVisualStyleBackColor = true;
            // 
            // tvEntities
            // 
            this.tvEntities.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tvEntities.HideSelection = false;
            this.tvEntities.LabelEdit = true;
            this.tvEntities.Location = new System.Drawing.Point(3, 3);
            this.tvEntities.Name = "tvEntities";
            this.tvEntities.Size = new System.Drawing.Size(173, 636);
            this.tvEntities.TabIndex = 1;
            this.tvEntities.AfterLabelEdit += new System.Windows.Forms.NodeLabelEditEventHandler(this.tvEntities_AfterLabelEdit);
            this.tvEntities.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.tvEntities_AfterSelect);
            this.tvEntities.BeforeLabelEdit += new System.Windows.Forms.NodeLabelEditEventHandler(this.tvEntities_BeforeLabelEdit);
            this.tvEntities.ItemDrag += new System.Windows.Forms.ItemDragEventHandler(this.tvEntities_ItemDrag);
            // 
            // splitContainer1
            // 
            this.splitContainer1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.splitContainer1.Location = new System.Drawing.Point(0, 52);
            this.splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.tcLevelsObjects);
            this.splitContainer1.Panel1MinSize = 120;
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.splitContainer2);
            this.splitContainer1.Panel2MinSize = 0;
            this.splitContainer1.Size = new System.Drawing.Size(1226, 668);
            this.splitContainer1.SplitterDistance = 187;
            this.splitContainer1.TabIndex = 5;
            // 
            // MainWnd
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1226, 742);
            this.Controls.Add(this.toolStrip1);
            this.Controls.Add(this.splitContainer1);
            this.Controls.Add(this.statusStrip1);
            this.Controls.Add(this.menuStrip1);
            this.MainMenuStrip = this.menuStrip1;
            this.Name = "MainWnd";
            this.Text = "MainWnd";
            this.Load += new System.EventHandler(this.MainWnd_Load);
            this.MouseUp += new System.Windows.Forms.MouseEventHandler(this.MainWnd_MouseUp);
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.MainWnd_FormClosed);
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            this.splitContainer2.Panel1.ResumeLayout(false);
            this.splitContainer2.Panel2.ResumeLayout(false);
            this.splitContainer2.ResumeLayout(false);
            this.splitContainer3.Panel1.ResumeLayout(false);
            this.splitContainer3.Panel1.PerformLayout();
            this.splitContainer3.Panel2.ResumeLayout(false);
            this.splitContainer3.Panel2.PerformLayout();
            this.splitContainer3.ResumeLayout(false);
            this.pEntButtons.ResumeLayout(false);
            this.pTplButtons.ResumeLayout(false);
            this.tcLevelsObjects.ResumeLayout(false);
            this.tpLevels.ResumeLayout(false);
            this.tpEntityCats.ResumeLayout(false);
            this.tpEntities.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

		}

		#endregion

        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem mmProject;
		private System.Windows.Forms.StatusStrip statusStrip1;
		private System.Windows.Forms.ToolStripMenuItem mmExit;
        private System.Windows.Forms.ToolStripMenuItem mmRestoreDB;
        private System.Windows.Forms.ToolStripMenuItem mmSaveDB;
        private System.Windows.Forms.ToolStrip toolStrip1;
        private System.Windows.Forms.ToolStripButton bTfmMode;
        private System.Windows.Forms.ToolStripButton bNewCategory;
        private System.Windows.Forms.ToolStripMenuItem mmExport;
		private System.Windows.Forms.ToolStripButton bNewLocation;
        private System.Windows.Forms.ToolStripButton bPauseWorld;
		private System.Windows.Forms.ToolStripButton bNotAbove;
		private System.Windows.Forms.ToolStripButton bNotBelow;
		private System.Windows.Forms.ToolStripButton bSelectByMouse;
		private System.Windows.Forms.SplitContainer splitContainer2;
		private System.Windows.Forms.Label lCatName;
		private System.Windows.Forms.PropertyGrid PropGrid;
		private System.Windows.Forms.Panel pEntButtons;
		private System.Windows.Forms.Button bSaveAsTemplate;
		private System.Windows.Forms.Button bCreateEntity;
		private System.Windows.Forms.Button bDeleteEntity;
		private System.Windows.Forms.Button bApplyPropChanges;
		private System.Windows.Forms.Panel pTplButtons;
		private System.Windows.Forms.Button bApplyTplChanges;
		private System.Windows.Forms.Button bCreateTemplate;
		private System.Windows.Forms.Button bDeleteTemplate;
		private System.Windows.Forms.Button button4;
		private System.Windows.Forms.TabControl tcLevelsObjects;
		private System.Windows.Forms.TabPage tpLevels;
		private System.Windows.Forms.ListBox lbLevels;
		private System.Windows.Forms.TabPage tpEntityCats;
		private System.Windows.Forms.TreeView tvCatsTpls;
		private System.Windows.Forms.TabPage tpEntities;
		private System.Windows.Forms.TreeView tvEntities;
		private System.Windows.Forms.SplitContainer splitContainer1;
		private System.Windows.Forms.SplitContainer splitContainer3;
		private System.Windows.Forms.Label lEntityUnderMouse;
		private System.Windows.Forms.Panel EngineViewPanel;
		private System.Windows.Forms.TextBox tOutput;
        private System.Windows.Forms.ToolStripButton bBuildNavMesh;
        private System.Windows.Forms.ToolStripButton bNavRegions;
        private System.Windows.Forms.ToolStripButton bNavOffmesh;
	}
}