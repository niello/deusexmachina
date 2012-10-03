using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Linq;
using System.Xml.Linq;

namespace DialogLogic
{
    public class DialogInfo:ICharacterContainer
    {
        public event EventHandler DialogInfoChanged;

        private readonly Dialogs _dialogs;
        public Dialogs Dialogs{get { return _dialogs; }}
    
        public ObservableCollection<DialogCharacter> DialogCharacters { get; private set; }

        private string _name;
        public string Name
        {
            get { return _name; }
            set
            {
                if (_name != value)
                {
                    _name = value;
                    OnDialogInfoChanged();
                }
            }
        }

        private readonly DialogGraph _graph=new DialogGraph();
        public DialogGraph Graph { get { return _graph;}}

        private readonly CharacterAttributeCollection _attributeCollection;
        public CharacterAttributeCollection AttributeCollection
        {
            get { return _attributeCollection; }
        }

        public ICharacterContainer ParentContainer
        {
            get { return _dialogs; }
        }

        public string ScriptFile { get; set; }

        public DialogInfo(Dialogs dialogs)
        {
            _dialogs = dialogs;

            DialogCharacters = new ObservableCollection<DialogCharacter>();
            DialogCharacters.CollectionChanged += CharactersCollectionChanged;
            _graph.GraphChanged += OnGraphChanged;

            _attributeCollection = new CharacterAttributeCollection(this);
            _attributeCollection.AddProperty("IsPlayer", typeof (bool));
        }

        internal void ReadXml(XElement xDialog, Func<int,DialogCharacter> characterSelector)
        {
            string name;
            if(xDialog.TryGetAttribute("name",out name))
                Name = name;

            XElement characterProperties = xDialog.Element("characterProperties"),
                     graph = xDialog.Element("graph"), chatactersNode = xDialog.Element("characters");

            Dictionary<int, string> propertyMap;
            if (characterProperties != null)
                propertyMap = _attributeCollection.ReadXml(characterProperties, new List<string> { "IsPlayer" });
            else
                propertyMap = new Dictionary<int, string>();

            var tmpCharacters = new Dictionary<int, DialogCharacter>();
            if(chatactersNode!=null)
            {
                foreach(var ch in chatactersNode.Elements("character"))
                {
                    int id;
                    if(!ch.TryGetAttribute("id",out id))
                        continue;

                    var baseChar = characterSelector(id);
                    var character = new DialogCharacter(baseChar, this);

                    character.ReadProperties(ch, propId => propertyMap[propId]);

                    tmpCharacters.Add(character.Id, character);
                    DialogCharacters.Add(character);
                }
            }

            if (graph != null)
            {
                XElement xNodes = graph.Element("nodes"), xLinks = graph.Element("links");
                var nodeDefaultLinks = new Dictionary<int, int>();

                if (xNodes != null)
                {
                    foreach (var xNode in xNodes.Elements())
                    {
                        int id;
                        if (!xNode.TryGetAttribute("id", out id))
                            continue;

                        int defaultLink;
                        if (xNode.TryGetAttribute("defaultLink", out defaultLink))
                            nodeDefaultLinks.Add(id, defaultLink);

                        DialogGraphNodeBase node = null;
                        switch (xNode.Name.LocalName)
                        {
                            case "node":
                                var n = new PhraseDialogGraphNode();
                                int charId;
                                if (xNode.TryGetAttribute("characterId", out charId))
                                    n.Character = tmpCharacters[charId].Name;

                                n.Phrase = xNode.Value;
                                node = n;
                                break;
                            case "empty":
                                node = new EmptyDialogGraphNode();
                                break;
                            case "answer":
                                node = new AnswerCollectionDialogGraphNode();
                                break;
                        }

                        if (node != null)
                        {
                            node.Id = id;
                            _graph.ForceInsertNode(node);
                        }
                    }
                }

                if(xLinks!=null)
                {
                    foreach(var xLink in xLinks.Elements("link"))
                    {
                        int from, to;
                        if (!xLink.TryGetAttribute("from", out from) || !xLink.TryGetAttribute("to", out to))
                            continue;

                        var link = Graph.Link(from, to);
                        int fromNodeDefault;
                        if(nodeDefaultLinks.TryGetValue(to,out fromNodeDefault) && fromNodeDefault==from)
                        {
                            var node = _graph.GetNode(to);
                            node.DefaultLinkHere = link;
                        }
                    }
                }
            }
        }

        public List<DialogCharacter> GetCharacters()
        {
            return new List<DialogCharacter>(DialogCharacters);
        }

        public bool TryGetCharacter(string name, out DialogCharacter character)
        {
            character = DialogCharacters.FirstOrDefault(c => c.Name == name);
            return character != null;
        }

        public bool ContainsCharacter(string name)
        {
            return DialogCharacters.Any(c => c.Name == name);
        }

        public DialogCharacter AddCharacter(string name)
        {
            DialogCharacter baseCh;
            if (!ParentContainer.TryGetCharacter(name, out baseCh))
                baseCh = ParentContainer.AddCharacter(name);

            var ch = new DialogCharacter(baseCh, this);
            DialogCharacters.Add(ch);
            return ch;
        }

        public bool IsActiveCharacter(DialogCharacter ch)
        {
            return Graph.AnyNode<PhraseDialogGraphNode>(node => node.Character == ch.Name);
        }

        public void LoadFromFile(string fPath, IDialogUserProperties mdReader)
        {
            _graph.LoadFromFile(fPath, mdReader);
        }

        public void SaveToFile(string filePath, IDialogUserProperties pref)
        {
            _graph.SaveToFile(filePath, pref);
        }

        private void OnDialogInfoChanged()
        {
            var eh = DialogInfoChanged;
            if (eh != null)
                eh(this, EventArgs.Empty);
        }

        private void CharactersCollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            if (e.NewItems != null)
            {
                foreach (DialogCharacter item in e.NewItems)
                    item.PropertyChanged += OnCharacterPropertyChanged;
            }
            if (e.OldItems != null)
            {
                foreach (DialogCharacter item in e.OldItems)
                    item.PropertyChanged -= OnCharacterPropertyChanged;
            }

            OnDialogInfoChanged();
        }

        private void OnCharacterPropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            OnDialogInfoChanged();
        }

        private void OnGraphChanged(object sender, EventArgs e)
        {
            OnDialogInfoChanged();
        }
    }
}
