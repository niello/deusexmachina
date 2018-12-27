using System.Text;
using System.Runtime.InteropServices;

namespace CreatorIDE.EngineAPI
{
    public class LevelRecord
    {
        public string ID { get; set; }
        public string Name { get; set; }

        public LevelRecord()
        {}

        public LevelRecord(string id, string name)
        {
            ID = id;
            Name = name;
        }

        public override string ToString()
        {
            return Name + " (" + ID + ")";
        }
    }

    public static class Levels
    {
        [DllImport(Engine.DllName, EntryPoint = "Levels_GetCount")]
        public static extern int GetCount();

        [DllImport(Engine.DllName, EntryPoint = "Levels_GetIDName")]
        private static extern void _GetIDName(int idx, StringBuilder id, StringBuilder name);
        public static LevelRecord GetIDName(int idx)
        {
            StringBuilder sb = new StringBuilder(256),
                          sb2 = new StringBuilder(256);
            _GetIDName(idx, sb, sb2);
            return new LevelRecord(sb.ToString(), sb2.ToString());
        }

        [DllImport(Engine.DllName, EntryPoint = "Levels_LoadLevel")]
        public static extern void LoadLevel(string levelID);

        [DllImport(Engine.DllName, EntryPoint = "Levels_CreateNew")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool CreateNew(string id, string name, float[] center, float[] extents,
                                            string navMesh);

        [DllImport(Engine.DllName, EntryPoint = "Levels_RestoreDB")]
        public static extern void RestoreDB(string startLevelID);

        [DllImport(Engine.DllName, EntryPoint = "Levels_SaveDB")]
        public static extern void SaveDB();

        [DllImport(Engine.DllName, EntryPoint = "Levels_BuildNavMesh")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool BuildNavMesh(string RsrcName, float AgentRadius, float AgentHeight, float MaxClimb);

        [DllImport(Engine.DllName, EntryPoint = "Levels_GetNavPolyUnderMouse")]
        public static extern int GetNavPolyUnderMouse();
    }
}