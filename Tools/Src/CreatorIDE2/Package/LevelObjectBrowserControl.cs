using System;
using System.Collections.Generic;
using System.Windows.Forms;
using CreatorIDE.Engine;
using Microsoft.VisualStudio.Shell;

namespace CreatorIDE.Package
{
    public partial class LevelObjectBrowserControl: UserControl, ILevelEditorTool
    {
        private LevelNode _level;

        public event EventHandler SelectionChanged;

        public ILevelEditorToolPane Pane { get; internal set; }

        public SelectionContainer Selection { get; private set; }

        public LevelObjectBrowserControl()
        {
            InitializeComponent();
        }

        public void Initialize(LevelNode level)
        {
            _level = level;
            LoadCategories();
        }

        private void LoadCategories()
        {
            CideEngine engine;
            if (!TryGetEngine(out engine))
                return;

            int catCount = engine.GetCategoryCount();
            var categories = new Dictionary<string, TreeNode>();
            for (int i = 0; i < catCount; i++)
            {
                var category = engine.GetCategory(i);
                var categoryNode = _treeView.Nodes.Add(category.Name);
                categoryNode.Tag = category;
                categories.Add(category.Name, categoryNode);
            }

            int entCount = engine.GetEntityCount();
            for (int i = 0; i < entCount; i++)
            {
                var category = engine.GetEntityCategory(i);
                TreeNode categoryNode;
                if (category == null || !categories.TryGetValue(category, out categoryNode))
                    continue;

                var uid = engine.GetEntityUID(i);
                var entityNode = categoryNode.Nodes.Add(uid);
                entityNode.Tag = uid;
            }
        }

        private void OnTreeNodeDoubleClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            string uid;
            if (string.IsNullOrEmpty(uid = e.Node.Tag as string))
                return;

            CideEngine engine;
            if (!TryGetEngine(out engine))
                return;

            engine.SetFocusEntity(uid);
        }

        private void OnSelectionChanged()
        {
            var h = SelectionChanged;
            if (h != null)
                h(this, EventArgs.Empty);
        }

        private void OnTreeAfterSelect(object sender, TreeViewEventArgs e)
        {
            object selectedObject;
            CideEngine engine;
            if(!TryGetEngine(out engine))
                selectedObject = null;
            else if (e.Node.Tag is Category)
            {
                var category = (Category) e.Node.Tag;
                selectedObject = new CideEntity(engine, category);
            }
            else if(e.Node.Tag is string)
            {
                var entityID = (string) e.Node.Tag;
                var category = (Category) e.Node.Parent.Tag;
                selectedObject = new CideEntity(engine, category, entityID, true);
            }
            else
                selectedObject = null;

            if (selectedObject == null)
                Selection = null;
            else
            {
                Selection = new SelectionContainer(true, false);
                var list = new List<object> {selectedObject};
                Selection.SelectableObjects = Selection.SelectedObjects = list;
            }
            OnSelectionChanged();
        }

        /// <summary>
        /// Get engine and check if it is initialized
        /// </summary>
        /// <param name="engine">Engine</param>
        /// <returns>True if engine is initialized, otherwise false</returns>
        private bool TryGetEngine(out CideEngine engine)
        {
            engine = _level == null ? null : _level.ProjectMgr.Package.Engine;
            return engine != null && engine.IsInitialized;
        }
    }
}
