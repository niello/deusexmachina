namespace CreatorIDE.Engine
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
            return string.Format("{0} ({1})", Name, ID);
        }
    }
}