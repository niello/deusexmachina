namespace CreatorIDE.Engine
{
	public struct AttrID
	{
	    private readonly string _name;
        public string Name { get { return _name; } }

	    private readonly int _id;
        public int ID { get { return _id; } }

	    private readonly bool _isReadWrite;
        public bool IsReadWrite { get { return _isReadWrite; } }

	    private readonly EDataType _type;
        public EDataType Type { get { return _type; } }

        public AttrID(int id, string name, EDataType type, bool isReadWrite)
        {
            _id = id;
            _name = name;
            _type = type;
            _isReadWrite = isReadWrite;
        }
	}
}
