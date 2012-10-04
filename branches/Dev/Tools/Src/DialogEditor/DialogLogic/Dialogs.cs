using System.Collections.Generic;
using System.Linq;

namespace DialogLogic
{
    public partial class Dialogs: ICharacterContainer
    {
        private int _identity = 1;

        private readonly SortedDictionary<string, DialogCharacter> _characters = new SortedDictionary<string, DialogCharacter>();
        public SortedDictionary<string, DialogCharacter> Characters{get { return _characters; }}

        private bool _hasChanges;
        public bool HasChanges { get { return _hasChanges; } set { _hasChanges = value; }}

        #region Implementation of ICharacterContainer

        private readonly CharacterAttributeCollection _attributeCollection;
        public CharacterAttributeCollection AttributeCollection
        {
            get { return _attributeCollection; }
        }

        ICharacterContainer ICharacterContainer.ParentContainer { get { return null; } }

        #endregion

        public Dialogs()
        {
            _attributeCollection = new CharacterAttributeCollection(this);
            _attributeCollection.AddProperty("DisplayName",typeof(string));
            _attributeCollection.AddProperty("Name", typeof (string));
            _attributeCollection.AddProperty("Id", typeof (int));
        }

        //public XmlSchema GetSchema()
        //{
        //    return null;
        //}

        //public void ReadXml(XmlReader reader)
        //{
        //    var dialogs = XElement.Load(reader);

        //    XElement xCharProperties = dialogs.Element("characterProperties");
        //    Dictionary<int, string> propertyMap;
        //    if (xCharProperties != null)
        //    {
        //        propertyMap = _attributeCollection.ReadXml(xCharProperties,
        //                                                   new List<string> { "DisplayName", "Name", "Id" });
        //    }
        //    else
        //        propertyMap = new Dictionary<int, string>();


        //    XElement charactersNode = dialogs.Element("characters"), dialogsNode = dialogs.Element("dialogs");

        //    var tmpCharacters = new Dictionary<int, DialogCharacter>();
        //    if(charactersNode!=null)
        //    {
        //        foreach(var chNode in charactersNode.Elements("character"))
        //        {
        //            var character = new DialogCharacter(this);
        //            character.ReadProperties(chNode, id => propertyMap[id]);

        //            tmpCharacters.Add(character.Id,character);
        //            _characters.Add(character.Name, character);
        //        }
        //    }

        //    if (dialogsNode != null)
        //    {
        //        foreach (var dlg in dialogsNode.Elements("dialog"))
        //        {
        //            var dialog = new DialogInfo(this);
        //            dialog.ReadXml(dlg, id => tmpCharacters[id]);
        //            Add(dialog);
        //        }
        //    }
        //}

        //public void WriteXml(XmlWriter writer)
        //{
        //    var element = new XElement("characterProperties");
        //    var propertyMap = _attributeCollection.WriteXml(element);

        //    element.WriteTo(writer);

        //    element = new XElement("characters");
        //    foreach(var c in _characters.Values)
        //    {
        //        var xChar = new XElement("character");
        //        c.WriteProperties(xChar, name => propertyMap[name]);
        //        element.Add(xChar);
        //    }

        //    element.WriteTo(writer);

        //    //element = new XElement("dialogs");

        //    //foreach(var dialog in this)
        //    //{
        //    //    var xDialog = new XElement("dialog");

        //    //    dialog.WriteXml(xDialog);

        //    //    element.Add(xDialog);
        //    //}

        //    //element.WriteTo(writer);
        //}

        public List<DialogCharacter> GetCharacters()
        {
            return _characters.Values.ToList();
        }

        public bool TryGetCharacter(string name, out DialogCharacter character)
        {
            return _characters.TryGetValue(name, out character);
        }

        public bool ContainsCharacter(string name)
        {
            return _characters.ContainsKey(name);
        }

        public DialogCharacter AddCharacter(string name)
        {
            var ch = new DialogCharacter(this) { Name = name, Id = _identity++ };

            _characters.Add(name, ch);

            _hasChanges = true;
            return ch;
        }
    }
}
