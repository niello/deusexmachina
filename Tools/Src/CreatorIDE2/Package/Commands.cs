using System;

namespace CreatorIDE.Package
{
    public static class Commands
    {
        public static readonly Guid
            LevelsNodeMenuGuid = new Guid(LevelsNodeMenuGuidString),
            LevelEditorCmdGuid = new Guid(LevelEditorCmdGuidString),
            LevelToolbarGuid = new Guid(LevelToolbarGuidString);

        public const string
            LevelsNodeMenuGuidString = "E96304E9-FE54-4ac7-9AC2-13EA54F29E82",
            LevelEditorCmdGuidString = "D6552A71-C621-42BD-B61C-C84EA444F9A0",
            LevelToolbarGuidString = "85875C5F-1ED7-4559-B63D-D16BF7678DB8";

        public const int
            LevelsNodeMenuRoot = 0x4200,
            LevelsNodeLink = 0x4201,

            LevelToolbar = 0x1000,
            LevelToolbarObjectBrowser = 0x1001,
            LevelToolbarGroup = 0x1050;
    }
}
