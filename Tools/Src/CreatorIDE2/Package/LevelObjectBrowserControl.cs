using System.Collections.Generic;
using System.Windows.Forms;

namespace CreatorIDE.Package
{
    public partial class LevelObjectBrowserControl: UserControl, ILevelEditorTool
    {
        private LevelNode _level;

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
            var engine = _level == null ? null : _level.ProjectMgr.Package.Engine;
            if (engine == null || !engine.IsInitialized)
                return;

            int catCount = engine.GetCategoryCount();
            var categories = new Dictionary<string, TreeNode>();
            for (int i = 0; i < catCount; i++)
            {
                int instAttrCount = engine.GetCategoryInstantAttrCount(i);
                if (instAttrCount < 2)
                    continue;

                var name = engine.GetCategoryName(i);
                var categoryNode = new TreeNode(name);

                categories.Add(name, categoryNode);
                
                _treeView.Nodes.Add(categoryNode);
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

            var engine = _level == null ? null : _level.ProjectMgr.Package.Engine;
            if (engine == null || !engine.IsInitialized)
                return;

            engine.SetFocusEntity(uid);
        }
    }
}
