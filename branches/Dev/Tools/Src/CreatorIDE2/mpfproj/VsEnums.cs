using System;
using Microsoft.VisualStudio.Shell.Interop;

namespace Microsoft.VisualStudio.Project
{
    [Flags]
    public enum VsEditorCreateDocWinFlags
    {
        None = 0,
        UserCanceled = __VSEDITORCREATEDOCWIN.ECDW_UserCanceled,
        AltDocData = __VSCREATEDOCWIN.CDW_fAltDocData & __VSEDITORCREATEDOCWIN.ECDW_CDWFLAGS_MASK,
        CreateNewWindow = __VSCREATEDOCWIN.CDW_fCreateNewWindow & __VSEDITORCREATEDOCWIN.ECDW_CDWFLAGS_MASK,
        Dockable = __VSCREATEDOCWIN.CDW_fDockable & __VSEDITORCREATEDOCWIN.ECDW_CDWFLAGS_MASK,
    }

    [Flags]
    public enum VsCreateEditorFlags : uint
    {
        CloneFile = __VSCREATEEDITORFLAGS.CEF_CLONEFILE,
        OpenAsNew = __VSCREATEEDITORFLAGS.CEF_OPENASNEW,
        OpenFile = __VSCREATEEDITORFLAGS.CEF_OPENFILE,
        Silent = __VSCREATEEDITORFLAGS.CEF_SILENT,
        OpenSpecific = __VSCREATEEDITORFLAGS2.CEF_OPENSPECIFIC,
    }

    [Flags]
    public enum VsCreateProjFlags : uint
    {
        CloneFile = __VSCREATEPROJFLAGS.CPF_CLONEFILE,
        NonLocalStore = __VSCREATEPROJFLAGS.CPF_NONLOCALSTORE,
        NotInSlnExplr = __VSCREATEPROJFLAGS.CPF_NOTINSLNEXPLR,
        OpenDirectory = __VSCREATEPROJFLAGS.CPF_OPENDIRECTORY,
        OpenFile = __VSCREATEPROJFLAGS.CPF_OPENFILE,
        Overwrite = __VSCREATEPROJFLAGS.CPF_OVERWRITE,
        Silent = __VSCREATEPROJFLAGS.CPF_SILENT,
        DefferedSave = __VSCREATEPROJFLAGS2.CPF_DEFERREDSAVE,
        OpenAsync = __VSCREATEPROJFLAGS2.CPF_OPEN_ASYNCHRONOUSLY,
        OpenStandalone = __VSCREATEPROJFLAGS2.CPF_OPEN_STANDALONE,
        SkipSolutionAccessCheck = __VSCREATEPROJFLAGS3.CPF_SKIP_SOLUTION_ACCESS_CHECK
    }

    public enum VsConfigPropID
    {
       First= __VSCFGPROPID.VSCFGPROPID_FIRST,
       IntrinsicExtenderCATID = __VSCFGPROPID.VSCFGPROPID_IntrinsicExtenderCATID,
       Last = __VSCFGPROPID.VSCFGPROPID_LAST,
       Reserved1 = __VSCFGPROPID.VSCFGPROPID_Reserved1,
       Reserved2 = __VSCFGPROPID.VSCFGPROPID_Reserved2,
       SupportsConfigAdd = __VSCFGPROPID.VSCFGPROPID_SupportsCfgAdd,
       SupportsConfigDelete = __VSCFGPROPID.VSCFGPROPID_SupportsCfgDelete,
       SupportsConfigRename = __VSCFGPROPID.VSCFGPROPID_SupportsCfgRename,
       SupportsPlatformAdd = __VSCFGPROPID.VSCFGPROPID_SupportsPlatformAdd,
       SupportsPlatformDelete = __VSCFGPROPID.VSCFGPROPID_SupportsPlatformDelete,
       SupportsPrivateConfigs = __VSCFGPROPID.VSCFGPROPID_SupportsPrivateCfgs,
    }

    public enum VsItemType : uint
    {
        Nil = VSConstants.VSITEMID_NIL,
        Root = VSConstants.VSITEMID_ROOT,
        Selection = VSConstants.VSITEMID_SELECTION,
        Other = Nil ^ Root ^ Selection
    }

    [Flags]
    public enum VsAddItemFlags: uint
    {
        AllowMultiSelect = __VSADDITEMFLAGS.VSADDITEM_AllowMultiSelect,
        HideNameField = __VSADDITEMFLAGS.VSADDITEM_HideNameField,
        SuggestTemplateName = __VSADDITEMFLAGS.VSADDITEM_SuggestTemplateName,
        ShowLocationField = __VSADDITEMFLAGS.VSADDITEM_ShowLocationField,
        ShowDontShowAgain = __VSADDITEMFLAGS.VSADDITEM_ShowDontShowAgain,
        AllowStickyFilter = __VSADDITEMFLAGS.VSADDITEM_AllowStickyFilter,
        AddNewItems = __VSADDITEMFLAGS.VSADDITEM_AddNewItems,
        AddExistingItems = __VSADDITEMFLAGS.VSADDITEM_AddExistingItems,
        ProjectHandlesLinks = __VSADDITEMFLAGS.VSADDITEM_ProjectHandlesLinks,
        NewDirectoryForItem = __VSADDITEMFLAGS.VSADDITEM_NewDirectoryForItem,
        AllowHiddenTreeView = __VSADDITEMFLAGS.VSADDITEM_AllowHiddenTreeView,
        NoOpenButtonDropDown = __VSADDITEMFLAGS.VSADDITEM_NoOpenButtonDropDown,
        AllowSingleTreeRoot = __VSADDITEMFLAGS.VSADDITEM_AllowSingleTreeRoot,
        ExpandSingleTreeRoot = __VSADDITEMFLAGS.VSADDITEM_ExpandSingleTreeRoot,
        ShowProjectTypesOnly = __VSADDITEMFLAGS.VSADDITEM_ShowProjectTypesOnly,
        AllowOnlyFileSysLocs = __VSADDITEMFLAGS.VSADDITEM_AllowOnlyFileSysLocs,
        NoUserTemplateFeatures = __VSADDITEMFLAGS2.VSADDITEM_NoUserTemplateFeatures,
        ShowOpenButtonDropDown = __VSADDITEMFLAGS2.VSADDITEM_ShowOpenButtonDropDown,
    }

    public enum VsHPropID
    {
        DefaultEnableDeployProjectCfg = __VSHPROPID.VSHPROPID_DefaultEnableDeployProjectCfg,
        First = __VSHPROPID.VSHPROPID_FIRST,
        DefaultEnableBuildProjectCfg = __VSHPROPID.VSHPROPID_DefaultEnableBuildProjectCfg,
        HasEnumerationSideEffects = __VSHPROPID.VSHPROPID_HasEnumerationSideEffects,
        DesignerFunctionVisibility = __VSHPROPID.VSHPROPID_DesignerFunctionVisibility,
        DesignerVariableNaming = __VSHPROPID.VSHPROPID_DesignerVariableNaming,
        ProjectIDGuid = __VSHPROPID.VSHPROPID_ProjectIDGuid,
        ShowOnlyItemCaption = __VSHPROPID.VSHPROPID_ShowOnlyItemCaption,
        IsNewUnsavedItem = __VSHPROPID.VSHPROPID_IsNewUnsavedItem,
        AllowEditInRunMode = __VSHPROPID.VSHPROPID_AllowEditInRunMode,
        ShowProjInSolutionPage = __VSHPROPID.VSHPROPID_ShowProjInSolutionPage,
        PreferredLanguageSID = __VSHPROPID.VSHPROPID_PreferredLanguageSID,
        CanBuildFromMemory = __VSHPROPID.VSHPROPID_CanBuildFromMemory,
        IsFindInFilesForegroundOnly = __VSHPROPID.VSHPROPID_IsFindInFilesForegroundOnly,
        IsNonSearchable = __VSHPROPID.VSHPROPID_IsNonSearchable,
        DefaultNamespace = __VSHPROPID.VSHPROPID_DefaultNamespace,
        OverlayIconIndex = __VSHPROPID.VSHPROPID_OverlayIconIndex,
        ItemSubType = __VSHPROPID.VSHPROPID_ItemSubType,
        StorageType = __VSHPROPID.VSHPROPID_StorageType,
        IsNonLocalStorage = __VSHPROPID.VSHPROPID_IsNonLocalStorage,
        IsNonMemberItem = __VSHPROPID.VSHPROPID_IsNonMemberItem,
        IsHiddenItem = __VSHPROPID.VSHPROPID_IsHiddenItem,
        NextVisibleSibling = __VSHPROPID.VSHPROPID_NextVisibleSibling,
        FirstVisibleChild = __VSHPROPID.VSHPROPID_FirstVisibleChild,
        StartupServices = __VSHPROPID.VSHPROPID_StartupServices,
        OwnerKey = __VSHPROPID.VSHPROPID_OwnerKey,
        ImplantHierarchy = __VSHPROPID.VSHPROPID_ImplantHierarchy,
        [Obsolete("Use IVsGetCfgProvider.")]
        ConfigurationProvider = __VSHPROPID.VSHPROPID_ConfigurationProvider,
        Expanded = __VSHPROPID.VSHPROPID_Expanded,
        ItemDocCookie = __VSHPROPID.VSHPROPID_ItemDocCookie,
        ParentHierarchyItemID = __VSHPROPID.VSHPROPID_ParentHierarchyItemid,
        ParentHierarchy = __VSHPROPID.VSHPROPID_ParentHierarchy,
        ReloadableProjectFile = __VSHPROPID.VSHPROPID_ReloadableProjectFile,
        HandlesOwnReload = __VSHPROPID.VSHPROPID_HandlesOwnReload,
        ProjectType = __VSHPROPID.VSHPROPID_ProjectType,
        TypeName = __VSHPROPID.VSHPROPID_TypeName,
        StateIconIndex = __VSHPROPID.VSHPROPID_StateIconIndex,
        ExtSelectedItem = __VSHPROPID.VSHPROPID_ExtSelectedItem,
        ExtObject = __VSHPROPID.VSHPROPID_ExtObject,
        EditLabel = __VSHPROPID.VSHPROPID_EditLabel,
        UserContext = __VSHPROPID.VSHPROPID_UserContext,
        SortPriority = __VSHPROPID.VSHPROPID_SortPriority,
        ProjectDir = __VSHPROPID.VSHPROPID_ProjectDir,
        AltItemid = __VSHPROPID.VSHPROPID_AltItemid,
        AltHierarchy = __VSHPROPID.VSHPROPID_AltHierarchy,
        BrowseObject = __VSHPROPID.VSHPROPID_BrowseObject,
        SelContainer = __VSHPROPID.VSHPROPID_SelContainer,
        CmdUIGuid = __VSHPROPID.VSHPROPID_CmdUIGuid,
        OpenFolderIconIndex = __VSHPROPID.VSHPROPID_OpenFolderIconIndex,
        OpenFolderIconHandle = __VSHPROPID.VSHPROPID_OpenFolderIconHandle,
        IconHandle = __VSHPROPID.VSHPROPID_IconHandle,
        ProjectName = __VSHPROPID.VSHPROPID_ProjectName,
        Name = __VSHPROPID.VSHPROPID_Name,
        ExpandByDefault = __VSHPROPID.VSHPROPID_ExpandByDefault,
        Expandable = __VSHPROPID.VSHPROPID_Expandable,
        IconIndex = __VSHPROPID.VSHPROPID_IconIndex,
        IconImgList = __VSHPROPID.VSHPROPID_IconImgList,
        Caption = __VSHPROPID.VSHPROPID_Caption,
        SaveName = __VSHPROPID.VSHPROPID_SaveName,
        TypeGuid = __VSHPROPID.VSHPROPID_TypeGuid,
        Root = __VSHPROPID.VSHPROPID_Root,
        NextSibling = __VSHPROPID.VSHPROPID_NextSibling,
        FirstChild = __VSHPROPID.VSHPROPID_FirstChild,
        Last = __VSHPROPID.VSHPROPID_LAST,
        Parent = __VSHPROPID.VSHPROPID_Parent,
        Nil = __VSHPROPID.VSHPROPID_NIL,
        SupportedMyApplicationTypes = __VSHPROPID2.VSHPROPID_SupportedMyApplicationTypes,
        First2 = __VSHPROPID2.VSHPROPID_FIRST2,
        ExcludeFromExportItemTemplate = __VSHPROPID2.VSHPROPID_ExcludeFromExportItemTemplate,
        NoDefaultNestedHierSorting = __VSHPROPID2.VSHPROPID_NoDefaultNestedHierSorting,
        PriorityPropertyPagesCLSIDList = __VSHPROPID2.VSHPROPID_PriorityPropertyPagesCLSIDList,
        ProjectDesignerEditor = __VSHPROPID2.VSHPROPID_ProjectDesignerEditor,
        DisableApplicationSettings = __VSHPROPID2.VSHPROPID_DisableApplicationSettings,
        CategoryGuid = __VSHPROPID2.VSHPROPID_CategoryGuid,
        DebuggerSourcePaths = __VSHPROPID2.VSHPROPID_DebuggerSourcePaths,
        AppTitleBarTopHierarchyName = __VSHPROPID2.VSHPROPID_AppTitleBarTopHierarchyName,
        EnableDataSourceWindow = __VSHPROPID2.VSHPROPID_EnableDataSourceWindow,
        UseInnerHierarchyIconList = __VSHPROPID2.VSHPROPID_UseInnerHierarchyIconList,
        Container = __VSHPROPID2.VSHPROPID_Container,
        SuppressOutOfDateMessageOnBuild = __VSHPROPID2.VSHPROPID_SuppressOutOfDateMessageOnBuild,
        DesignerHiddenCodeGeneration = __VSHPROPID2.VSHPROPID_DesignerHiddenCodeGeneration,
        IsUpgradeRequired = __VSHPROPID2.VSHPROPID_IsUpgradeRequired,
        IntellisenseUnknown = __VSHPROPID2.VSHPROPID_IntellisenseUnknown,
        SupportsProjectDesigner = __VSHPROPID2.VSHPROPID_SupportsProjectDesigner,
        KeepAliveDocument = __VSHPROPID2.VSHPROPID_KeepAliveDocument,
        IsLinkFile = __VSHPROPID2.VSHPROPID_IsLinkFile,
        DebuggeeProcessId = __VSHPROPID2.VSHPROPID_DebuggeeProcessId,
        StatusBarClientText = __VSHPROPID2.VSHPROPID_StatusBarClientText,
        ChildrenEnumerated = __VSHPROPID2.VSHPROPID_ChildrenEnumerated,
        AddItemTemplatesGuid = __VSHPROPID2.VSHPROPID_AddItemTemplatesGuid,
        CfgBrowseObjectCATID = __VSHPROPID2.VSHPROPID_CfgBrowseObjectCATID,
        BrowseObjectCATID = __VSHPROPID2.VSHPROPID_BrowseObjectCATID,
        ExtObjectCATID = __VSHPROPID2.VSHPROPID_ExtObjectCATID,
        CfgPropertyPagesCLSIDList = __VSHPROPID2.VSHPROPID_CfgPropertyPagesCLSIDList,
        PropertyPagesCLSIDList = __VSHPROPID2.VSHPROPID_PropertyPagesCLSIDList,
        First3 = __VSHPROPID3.VSHPROPID_FIRST3,
        IsDefaultNamespaceRefactorNotify = __VSHPROPID3.VSHPROPID_IsDefaultNamespaceRefactorNotify,
        RefactorExtensions = __VSHPROPID3.VSHPROPID_RefactorExtensions,
        ProductBrandName = __VSHPROPID3.VSHPROPID_ProductBrandName,
        SupportsLinqOverDataSet = __VSHPROPID3.VSHPROPID_SupportsLinqOverDataSet,
        SupportsNTierDesigner = __VSHPROPID3.VSHPROPID_SupportsNTierDesigner,
        SupportsHierarchicalUpdate = __VSHPROPID3.VSHPROPID_SupportsHierarchicalUpdate,
        ServiceReferenceSupported = __VSHPROPID3.VSHPROPID_ServiceReferenceSupported,
        WebReferenceSupported = __VSHPROPID3.VSHPROPID_WebReferenceSupported,
        TargetFrameworkVersion = __VSHPROPID3.VSHPROPID_TargetFrameworkVersion
    }
}
