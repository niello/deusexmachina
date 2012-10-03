using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using CreatorIDE.EngineAPI;

namespace CreatorIDE
{
	public partial class MainWnd : Form
	{
        [StructLayout(LayoutKind.Sequential)]
        public struct Message
        {
            public IntPtr hWnd;
            public uint Msg;
            public IntPtr wParam;
            public IntPtr lParam;
            public uint Time;
            public System.Drawing.Point Point;
        }

        [DllImport("User32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool PeekMessage(out Message Msg, IntPtr hWnd, uint filterMin, uint filterMax, uint flags);
        
        //!!!not here!
		public static Dictionary<string, Category> EntityCats = new Dictionary<string, Category>();
		public static Dictionary<string, Entity> Entities = new Dictionary<string, Entity>();
		public static Dictionary<string, EntityTemplate> EntityTpls = new Dictionary<string, EntityTemplate>();
        
		string CurrLevelID;
		string CurrProject;
		Entity CurrEntity;
		EntityTemplate CurrTemplate;
		bool CreateNewEntity;
		bool CreateNewTpl;
		bool DontFocusOnEntity = false;
		TreeNode DraggedTpl = null;

        private event MouseButtonCallback MouseCb;
       
        public MainWnd()
		{
			InitializeComponent();
            bCreateEntity.Visible = false;
            bApplyPropChanges.Visible = false;
            bSaveAsTemplate.Visible = false;
            bDeleteEntity.Visible = false;
		}

        private void MainWnd_Load(object sender, EventArgs e)
		{
			if (LoadProject("..\\..\\..\\..\\InsanePoet"))
			{
                Text = "Creator IDE (" + EngineAPI.Engine.GetDllName() + " v" + EngineAPI.Engine.GetDllVersion() +
					") - [" + CurrProject + "]";

                Application.Idle += TickWhileIdle;
			}
			else Text = "Creator IDE - Ошибка инициализации!";
		}

		private void MainWnd_FormClosed(object sender, System.Windows.Forms.FormClosedEventArgs e)
		{
            Application.Idle -= TickWhileIdle;
			EngineAPI.Engine.Release();
		}
  
        void TickWhileIdle(object sender, EventArgs e)
        {
            Message Msg;
            while (!PeekMessage(out Msg, IntPtr.Zero, 0, 0, 0))
                FrameTick(sender, e);
        }

		private void FrameTick(object sender, EventArgs e)
		{
			if (!EngineAPI.Engine.Advance())
			{
                Application.Idle -= TickWhileIdle;
				Close();
				return;
			}
			lEntityUnderMouse.Text = "Объект под курсором: " + EngineAPI.Entities.GetUIDUnderMouse();
        }

		#region Main menu event callbacks

		private void mmRestoreDB_Click(object sender, EventArgs e)
		{
			OnStartLevelLoading();
			EngineAPI.Levels.RestoreDB(CurrLevelID); //???!!!what if restore DB when no level loaded!?
            if (CurrLevelID != null && CurrLevelID.Length > 0) OnLevelLoaded();
		}

		private void mmSaveDB_Click(object sender, EventArgs e)
		{
            if (CurrEntity != null) CurrEntity.Save();
			EngineAPI.Levels.SaveDB();
		}

		private void mmExit_Click(object sender, EventArgs e)
		{
            Application.Idle -= TickWhileIdle;
			Close();
		}

		#endregion

		private void lbLevels_DoubleClick(object sender, EventArgs e)
		{
		    if (lbLevels.SelectedItem == null) return;
		    OnStartLevelLoading();
		    CurrLevelID = ((LevelRecord) lbLevels.SelectedItem).ID;
		    Levels.LoadLevel(CurrLevelID);
		    OnLevelLoaded();
		}
        
		private void EngineViewPanel_MouseCallback(int x, int y, int Button, EMouseAction Action)
		{
            if (Button == 0)
            {
                if (Action == EngineAPI.EMouseAction.Down)
                {
                    DraggedTpl = null;

                    if (bSelectByMouse.Checked)
                    {
                        string UID = EngineAPI.Entities.GetUIDUnderMouse();
                        if (Entities.ContainsKey(UID))
                        {
                            tcLevelsObjects.SelectTab(tpEntities);
							if (CurrEntity == null || UID != CurrEntity.Uid) DontFocusOnEntity = true;
                            tvEntities.SelectedNode = Entities[UID].UiNode; // Causes tvEntities_AfterSelect
                        }
                    }
                }
                else if (Action == EngineAPI.EMouseAction.Up && DraggedTpl != null)
                {
                    string TplName = DraggedTpl.Name;
                    string UIDBase = CurrLevelID + "_" + TplName + "_";
                    string UID = UIDBase + EngineAPI.Entities.GetNextFreeUIDOnLevel(UIDBase).ToString();
                    string CatName = DraggedTpl.Parent.Name;
                    Category Cat = EntityCats[CatName];
                    Entity NewEnt = new Entity(UID, Cat, false);
                    if (NewEnt.CreateFromTemplate(TplName, CurrLevelID, true))
                    {
						AddEntity(NewEnt);
                        tvEntities.SelectedNode = NewEnt.UiNode;
                    }
                    else MessageBox.Show("Не удалось создать новый объект!");
                    
                    DraggedTpl = null;
                }
            }
		}

		bool LoadProject(string ProjectRoot)
		{
			if (EngineAPI.Engine.Init(EngineViewPanel.Handle, ProjectRoot) == 0)
			{
				CurrProject = ProjectRoot;

                MouseCb = new EngineAPI.MouseButtonCallback(EngineViewPanel_MouseCallback);
                EngineAPI.Engine.SetMouseButtonCallback(MouseCb);

				int LvlCount = EngineAPI.Levels.GetCount();
                for (int i = 0; i < LvlCount; i++)
                    lbLevels.Items.Add(EngineAPI.Levels.GetIDName(i));

                if (EngineAPI.Categories.ParseAttrDescs("home:Data/AttrDescs.hrd"))
                {
                    int AttrDescCount = EngineAPI.Categories.GetAttrDescCount();
                    AttrDesc.DescList = new Dictionary<string, AttrDesc>(AttrDescCount);
                    for (int i = 0; i < AttrDescCount; i++)
                    {
                        AttrDesc Desc;
                        string Name = EngineAPI.Categories.GetAttrDesc(i, out Desc);
                        if (Desc.ResourceExt.Length > 0)
                            Desc.ResourceDir = CurrProject.Replace('\\', '/') + "/Content" + Desc.ResourceDir;
                        AttrDesc.DescList.Add(Name, Desc);
                    }
                }

				EntityCats.Clear();
				EntityTpls.Clear();
				tvCatsTpls.Nodes.Clear();
				tvEntities.Nodes.Clear();

				int CatCount = EngineAPI.Categories.GetCount();
                for (int i = 0; i < CatCount; i++)
                {
                    Category Cat = AddCategory(i);
					if (Cat != null)
					{
						int TplCount = EngineAPI.Categories.GetTemplateCount(Cat.Name);
						for (int j = 0; j < TplCount; j++)
						{
							EntityTemplate Tpl =
								new EntityTemplate(EngineAPI.Categories.GetTemplateID(Cat.Name, j), Cat);
							AddEntityTemplate(Tpl);
						}
					}
                }

				return true;
			}

			return false;
		}

        public Category AddCategory(int Idx)
        {
			int InstAttrCount = EngineAPI.Categories.GetInstAttrCount(Idx);
			if (InstAttrCount < 2) return null;
			Category Cat = new Category();
            Cat.Name = EngineAPI.Categories.GetName(Idx);
			for (int j = 0; j < InstAttrCount; j++)
                Cat.AttrIDs.Add(EngineAPI.Categories.GetAttrID(Idx, j));
            EntityCats.Add(Cat.Name, Cat);
            Cat.TplNode = tvCatsTpls.Nodes.Add(Cat.Name, Cat.Name);
            Cat.TplNode.Tag = Cat;
            Cat.InstNode = tvEntities.Nodes.Add(Cat.Name);
            Cat.InstNode.Tag = Cat;
            return Cat;
        }

		private void AddEntity(Entity Ent)
		{
			Ent.EntityRenamed += OnEntityRenamed;
			Entities.Add(Ent.Uid, Ent);
			Ent.UiNode = EntityCats[Ent.Category].InstNode.Nodes.Add(Ent.Uid);
			Ent.UiNode.Tag = Ent;
		}

		private void DeleteEntity(Entity Ent)
		{
			Ent.UiNode.Remove();
			Ent.UiNode = null;
			Entities.Remove(Ent.Uid);
		}

		private void AddEntityTemplate(EntityTemplate Tpl)
		{
			EntityTpls.Add(Tpl.Uid, Tpl);
			Tpl.UiNode = EntityCats[Tpl.Category].TplNode.Nodes.Add(Tpl.Uid, Tpl.Uid);
			Tpl.UiNode.Tag = Tpl;
		}

		private void DeleteEntityTemplate(EntityTemplate Tpl)
		{
			Tpl.UiNode.Remove();
			Tpl.UiNode = null;
			EntityTpls.Remove(Tpl.Uid);
		}

		private void OnStartLevelLoading()
		{
			Entities.Clear();
			//!!!clear node in entity destructor!
			foreach (TreeNode n in tvEntities.Nodes) n.Nodes.Clear();
		}

		private void OnLevelLoaded()
		{
			int EntCount = EngineAPI.Entities.GetCount();
			for (int i = 0; i < EntCount; i++)
			{
				string CatName = EngineAPI.Entities.GetCategoryByIndex(i);
				if (EntityCats.ContainsKey(CatName))
				{
					Entity NewEnt = new Entity(EngineAPI.Entities.GetUIDByIndex(i), EntityCats[CatName], true);
					AddEntity(NewEnt);
				}
			}
		}

        private void tvEntities_BeforeLabelEdit(object sender, NodeLabelEditEventArgs e)
        {
            if (e.Node.Level != 1) e.CancelEdit = true;
        }

        private void tvEntities_AfterLabelEdit(object sender, NodeLabelEditEventArgs e)
        {
            Entity Ent = e.Node.Tag as Entity;
            if (Ent == null || !Ent.Rename(e.Label)) e.CancelEdit = true;
        }

        void OnEntityRenamed(Entity Ent, string OldUID)
        {
            Entities.Remove(OldUID);
            Entities.Add(Ent.Uid, Ent);
            if (Ent == PropGrid.SelectedObject) PropGrid.Refresh();
        }

        private void bTfmMode_CheckedChanged(object sender, EventArgs e)
        {
            Transform.SetEnabled(bTfmMode.Checked); //, (CurrEntity == null) ? "" : CurrEntity.GUID);
        }

        private void bNewCategory_Click(object sender, EventArgs e)
        {
            NewCategoryWnd Wnd = new NewCategoryWnd();
            if (Wnd.ShowDialog() == DialogResult.OK)
            {
                int Idx = EngineAPI.Categories.CreateNew(Wnd.CatName, Wnd.CppClass, Wnd.TplTable,
                                                         Wnd.InstTable, Wnd.Properties);
                if (Idx > -1) AddCategory(Idx);
            }
        }

        private void mmExport_Click(object sender, EventArgs e)
        {
            string CmdLineArgs =
                "-proj " + CurrProject + "\\Content -build " + CurrProject +
                "\\Build -staticdb static.db3 -gamedb game.db3 -nowait";
            tOutput.Text = CommandLine.Run("..\\BBuilder\\BBuilder.exe", CmdLineArgs, false);
            MessageBox.Show("Экспорт завершён");
        }

		private void bNewLocation_Click(object sender, EventArgs e)
		{
			NewLocationWnd Wnd = new NewLocationWnd();
			if (Wnd.ShowDialog() == DialogResult.OK)
				if (EngineAPI.Levels.CreateNew(Wnd.ID, Wnd.LevelName, Wnd.Center, Wnd.Extents, Wnd.NavMesh))
					lbLevels.Items.Add(Wnd.ID);
		}

        private void bPauseWorld_Click(object sender, EventArgs e)
        {
            EngineAPI.World.TogglePause();
        }

		private void bLimitToGround_CheckedChanged(object sender, EventArgs e)
		{
			EngineAPI.Transform.SetGroundRespectMode(bLimitToGround.Checked, bSnapToGround.Checked);
        }

        private void tvEntities_ItemDrag(object sender, ItemDragEventArgs e)
        {
            //
        }

        private void EngineViewPanel_PreviewKeyDown(object sender, PreviewKeyDownEventArgs e)
        {
            //
        }

        private void tvCatsTpls_ItemDrag(object sender, ItemDragEventArgs e)
        {
            DraggedTpl = e.Item as TreeNode;
            if (DraggedTpl.Level != 1) DraggedTpl = null;
        }

        private void tvCatsTpls_MouseUp(object sender, MouseEventArgs e)
        {
            DraggedTpl = null;
        }

        private void MainWnd_MouseUp(object sender, MouseEventArgs e)
        {
            DraggedTpl = null;
        }

        private void tcLevelsObjects_SelectedIndexChanged(object sender, EventArgs e)
        {
            pEntButtons.Visible = (tcLevelsObjects.SelectedTab == tpEntities);
            pTplButtons.Visible = (tcLevelsObjects.SelectedTab == tpEntityCats);

			if (tcLevelsObjects.SelectedTab == tpEntityCats)
			{
				CurrTemplate = null;
				OnSelectTplTreeNode(tvCatsTpls.SelectedNode);
			}
			else if (tcLevelsObjects.SelectedTab == tpEntities)
			{
                DontFocusOnEntity = true;
                CurrEntity = null;
				OnSelectEntityTreeNode(tvEntities.SelectedNode);
			}
			else PropGrid.SelectedObject = null;
        }

		private void tvCatsTpls_AfterSelect(object sender, TreeViewEventArgs e)
		{
			if (tcLevelsObjects.SelectedTab == tpEntityCats) OnSelectTplTreeNode(e.Node);
		}

        private void tvEntities_AfterSelect(object sender, TreeViewEventArgs e)
        {
			if (tcLevelsObjects.SelectedTab == tpEntities) OnSelectEntityTreeNode(e.Node);
        }

		private void OnSelectTplTreeNode(TreeNode Node)
		{
			if (Node == null)
			{
				tvCatsTpls.SelectedNode = tvCatsTpls.Nodes[0];
				return;
			}

			if (Node.Level == 1)
			{
				// Select entity template
				EntityTemplate Tpl = Node.Tag as EntityTemplate;
				if (Tpl != CurrTemplate)
				{
					//???if (CurrTemplate != null && !CreateNewTpl) CurrTemplate.Save();
					CurrTemplate = Tpl;
					if (CurrTemplate != null) CurrTemplate.Update();
					PropGrid.SelectedObject = CurrTemplate;
				}
				CreateNewTpl = false;
				lCatName.Text = Tpl.Category;
			}
			else if (Node.Level == 0)
			{
				// Select category in template tree
				Category Cat = Node.Tag as Category;
				CurrTemplate = new EntityTemplate("", Cat);
				CurrTemplate.Update();
				PropGrid.SelectedObject = CurrTemplate;
				CreateNewTpl = true;
				lCatName.Text = Cat.Name;
			}

			//???need 2 different buttons?!

			bCreateTemplate.Visible = CurrTemplate != null && CreateNewTpl;
			bApplyTplChanges.Visible = CurrTemplate != null && !CreateNewTpl;
			bDeleteTemplate.Visible = CurrTemplate != null && !CreateNewTpl;
			
			tvCatsTpls.Focus();
		}

		private void OnSelectEntityTreeNode(TreeNode Node)
		{
			if (Node == null)
			{
				tvEntities.SelectedNode = tvEntities.Nodes[0];
				return;
			}

			if (Node.Level == 1)
			{
				// Select entity in entity tree
				Entity Ent = Node.Tag as Entity;
				if (Ent != CurrEntity)
				{
					if (CurrEntity != null && !CreateNewEntity) CurrEntity.Save();

					CurrEntity = Ent;
					if (CurrEntity != null)
					{
						CurrEntity.Update();
						EngineAPI.Transform.SetCurrentEntity(CurrEntity.Uid);
						if (DontFocusOnEntity) DontFocusOnEntity = false;
						else EditorCamera.SetFocusEntity(Ent.Uid);
					}
					else EngineAPI.Transform.SetCurrentEntity(null);

					PropGrid.SelectedObject = CurrEntity;
				}
				CreateNewEntity = false;
				lCatName.Text = Ent.Category;
			}
			else if (Node.Level == 0)
			{
				// Select category in entity tree
				Category Cat = Node.Tag as Category;
				CurrEntity = new Entity("", Cat, false);
				CurrEntity.Update();
				PropGrid.SelectedObject = CurrEntity;
				CreateNewEntity = true;
				lCatName.Text = Cat.Name;
                if (DontFocusOnEntity) DontFocusOnEntity = false;
			}

			bCreateEntity.Visible = CurrEntity != null && CreateNewEntity;
			bApplyPropChanges.Visible = CurrEntity != null && !CreateNewEntity;
			bSaveAsTemplate.Visible = CurrEntity != null;
			bDeleteEntity.Visible = CurrEntity != null && !CreateNewEntity;
			
			tvEntities.Focus();
		}

        private void bCreateEntity_Click(object sender, EventArgs e)
        {
			if (CurrEntity != null && CurrEntity.Create(CurrLevelID))
            {
				AddEntity(CurrEntity);
                tvEntities.SelectedNode = CurrEntity.UiNode;
				EngineAPI.EditorCamera.SetFocusEntity(CurrEntity.Uid);
            }
            else MessageBox.Show("Не удалось создать новый объект!");
        }

        private void bApplyPropChanges_Click(object sender, EventArgs e)
        {
            if (CurrEntity != null) CurrEntity.Save();
        }

        private void bSaveAsTemplate_Click(object sender, EventArgs e)
        {
            if (CurrEntity != null)
            {
                GetStringWnd Wnd = new GetStringWnd();
                Wnd.UserString = CurrEntity.GuidPropValue;
                if (Wnd.ShowDialog() != DialogResult.OK) return;
                if (CurrEntity.SaveAsTemplate(Wnd.UserString)) //???need UID param?
                {
					Category Cat = EntityCats[CurrEntity.Category];
					EntityTemplate Tpl =
						new EntityTemplate(Wnd.UserString, Cat);
					AddEntityTemplate(Tpl);
					tvCatsTpls.SelectedNode = Tpl.UiNode;
					tcLevelsObjects.SelectedTab = tpEntityCats;
				}
				else MessageBox.Show("Не удалось создать новый шаблон!");
            }
        }

        private void bDeleteEntity_Click(object sender, EventArgs e)
        {
            if (CurrEntity != null)
            {
                //if (bTfmMode.Checked) bTfmMode.Checked = false;
                PropGrid.SelectedObject = null;
				EngineAPI.Entities.Delete(CurrEntity.Uid);
				DeleteEntity(CurrEntity);
            }
        }

        private void bCreateTemplate_Click(object sender, EventArgs e)
        {
			if (CurrTemplate != null && CurrTemplate.Save())
			{
				AddEntityTemplate(CurrTemplate);
				tvCatsTpls.SelectedNode = CurrTemplate.UiNode;
			}
			else MessageBox.Show("Не удалось создать новый шаблон!");
		}

		private void bApplyTplChanges_Click(object sender, EventArgs e)
		{
			if (CurrTemplate != null) CurrTemplate.Save();
		}

        private void bDeleteTemplate_Click(object sender, EventArgs e)
        {
			if (CurrTemplate != null)
			{
				PropGrid.SelectedObject = null;
				EngineAPI.Entities.DeleteTemplate(CurrTemplate.Uid, CurrTemplate.Category);
				DeleteEntityTemplate(CurrTemplate);
			}
		}

        private void bBuildNavMesh_Click(object sender, EventArgs e)
        {
            EngineAPI.Levels.BuildNavMesh("ECCY", 0.29f, 1.74f, 0.6f);
        }
	}
}
