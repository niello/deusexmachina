using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Project;

namespace CreatorIDE.Package
{
    [ComVisible(true), Guid(GuidString)]
    public class LevelNodeProperties: NodeProperties
    {
        private const string GuidString = "B443E815-E31B-4991-98EC-F4F3FE874060";

        public LevelNodeProperties(LevelNode node):
            base(node)
        {}
    }
}
