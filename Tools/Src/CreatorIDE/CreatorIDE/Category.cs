using System.Collections.Generic;
using System.Windows.Forms;
using CreatorIDE.EngineAPI;

namespace CreatorIDE
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

	public class Category
	{
        private readonly List<AttrID> _attrIDs;

	    public string Name { get; set; }
	    public TreeNode TplNode { get; set; }
	    public TreeNode InstNode { get; set; }

        public List<AttrID> AttrIDs
        {
            get { return _attrIDs; }
        }

		public Category()
		{
			_attrIDs = new List<AttrID>();
		}
	}
}
