namespace CreatorIDE.Package
{
    internal interface ICideHierarchyNode
    {
        CideBuildAction BuildAction { get; }
        CideBuildAction EffectiveBuildAction { get; }

        void OnBuildActionChanged();
    }
}
