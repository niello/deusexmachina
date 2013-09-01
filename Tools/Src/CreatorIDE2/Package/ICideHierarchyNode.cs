using System;
using System.Diagnostics;
using System.IO;
using Microsoft.VisualStudio.Project;

namespace CreatorIDE.Package
{
    internal interface ICideHierarchyNode
    {
        ICideHierarchyNode Parent { get; }

        CideBuildAction BuildAction { get; }
        CideBuildAction EffectiveBuildAction { get; }

        string FullPath { get; }

        void OnBuildActionChanged();
    }

    /// <summary>
    /// The class provides basic methods for <see cref="CideProjectNode"/>, <see cref="CideFolderNode"/>, and <see cref="CideFileNode"/>.
    /// It help us to avoid of a code duplication in these classes.
    /// </summary>
    internal static class CideHierarchyNodeHelper
    {
        public static QueryStatusResult QueryCommandStatus(this ICideHierarchyNode node, CideItemNodeCommand command)
        {
            if (node == null)
                throw new ArgumentNullException("node");

            switch (command)
            {
                case CideItemNodeCommand.BrowseFolder:
                    return QueryStatusResult.Enabled | QueryStatusResult.Supported;

                default:
                    return QueryStatusResult.NotSupported;
            }
        }

        public static bool ExecuteCommand(this ICideHierarchyNode node, CideItemNodeCommand command)
        {
            if (node == null)
                throw new ArgumentNullException("node");

            switch (command)
            {
                case CideItemNodeCommand.BrowseFolder:
                    var path = node.FullPath;
                    if (!Path.IsPathRooted(path))
                        return false;

                    var arg = File.Exists(path) ? "select" : "root";

                    Process.Start("explorer.exe", string.Format("/{0},\"{1}\"", arg, path));
                    return true;

                default:
                    return false;
            }
        }

        public static CideBuildAction GetEffectiveBuildAction(this ICideHierarchyNode node)
        {
            if (node == null)
                throw new ArgumentNullException("node");

            var result = node.BuildAction;
            if (result == CideBuildAction.Inherited)
            {
                var parent = node.Parent;
                if (parent != null)
                    result = parent.EffectiveBuildAction;
            }

            if (result == CideBuildAction.Inherited)
                result = CideBuildAction.None;

            return result;
        }
    }
}
