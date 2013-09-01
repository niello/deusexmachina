using System;

namespace CreatorIDE.Package
{
    public static class Commands
    {
        public static readonly Guid
            ItemNodeMenuGuid = new Guid(ItemNodeMenuGuidString),
            LevelEditorCmdGuid = new Guid(LevelEditorCmdGuidString),
            LevelToolbarGuid = new Guid(LevelToolbarGuidString),
            FileNodeMenuGuid = new Guid(FileNodeMenuGuidString);

        public const string
            ItemNodeMenuGuidString = "E96304E9-FE54-4ac7-9AC2-13EA54F29E82",
            LevelEditorCmdGuidString = "D6552A71-C621-42BD-B61C-C84EA444F9A0",
            LevelToolbarGuidString = "85875C5F-1ED7-4559-B63D-D16BF7678DB8",
            FileNodeMenuGuidString = "C3C10873-F69A-452a-B7CC-A4B177DAA39B";

        public const int
            ItemNodeMenuRoot = 0x4200,
            ItemNodeBrowseFolder = 0x4202,

            FileNodeMenuRoot = 0x4300,
            FileNodeLink = 0x4301,

            LevelToolbar = 0x1000,
            LevelToolbarObjectBrowser = 0x1001,
            LevelToolbarGroup = 0x1050;
    }
}
