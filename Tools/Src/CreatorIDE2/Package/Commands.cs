using System;

namespace CreatorIDE.Package
{
    public static class Commands
    {
        public static readonly Guid
            LevelsNodeMenuGuid = new Guid(LevelsNodeMenuGuidString);

        public const string
            LevelsNodeMenuGuidString = "E96304E9-FE54-4ac7-9AC2-13EA54F29E82";

        public const int
            LevelsNodeMenuRoot = 0x4200,
            LevelsNodeLink = 0x4201;
    }
}
